#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
// #include <jansson.h>
#include <sys/time.h>
#include "game_board.h"
#include "mcts.h"
#include "player_strategy.h"
#include "util.h"

static void initMCTSNode(MCTSNode * node, MCTSNode * parent, UnscoredState state, PlayerNum playerJustMoved, Edge move);
static void freeMCTSNode(MCTSNode * node);
static MCTSNode * applyTreePolicy(MCTSNode * root, UnscoredState * rootState);
static MCTSNode * getBestChildUCB1(const MCTSNode * node);
static MCTSNode * expandMCTSNode(MCTSNode * node);
static short getUntriedMovesMCTSNode(MCTSNode * node, Edge * untriedMovesBuffer);
static MCTSNode * addChildToMCTSNode(MCTSNode * parentNode, Edge move);
static double applyDefaultPolicy(const MCTSNode * leafNode, short rootNumBoxesLeft);
static void backpropagateResult(MCTSNode * node, PlayerNum scoreFirstPlayer, double score);
static MCTSNode * getMostVisitedChild(MCTSNode * node);
//static void saveMCTSNodeJSON(const MCTSNode * node, const char * filePath);
//static json_t * MCTSNodeToJSON(const MCTSNode * node);


static void initMCTSNode(MCTSNode * node, MCTSNode * parent, UnscoredState state, PlayerNum playerJustMoved, Edge move) {
    node->parent = parent;
    node->state = state;
    node->playerJustMoved = playerJustMoved;

    short numBoxesTakenByMove = 0;
    if (move != NO_EDGE) {
        UnscoredState preMoveState = state;
        setEdgeFree(&preMoveState, move);
        numBoxesTakenByMove = howManyBoxesDoesMoveComplete(&preMoveState, move);
    }

    if (playerJustMoved == NO_PLAYER)
        node->nextPlayerToMove = 1;
    else if (numBoxesTakenByMove > 0)
        node->nextPlayerToMove = playerJustMoved;
    else
        node->nextPlayerToMove = 3 - playerJustMoved;

    node->move = move;
    
    node->numBoxesTakenByMove = numBoxesTakenByMove;
    node->visits = 0;
    node->totalScore = 0.0; // from perspective of playerJustMoved
    node->numPotentialMoves = getNumFreeEdges(&state);
    node->numChildren = 0;
    node->child = NULL;
    node->sibling = NULL;
}

static void freeMCTSNode(MCTSNode * node) {
    if (node->numChildren != 0) { // recursively free it's children
        MCTSNode * currentChild = node->child;
        MCTSNode * nextChild;

        do {
            nextChild = currentChild->sibling;
            freeMCTSNode(currentChild);
            currentChild = nextChild;
        } while (currentChild != NULL);
    }

    free(node);
}

// SELECT
static MCTSNode * applyTreePolicy(MCTSNode * root, UnscoredState * rootState) {
    // Selects a node from the tree and expands it, modifying rootState.

    MCTSNode * node = root;

    while(node->numPotentialMoves > 0) { // node is non-terminal
        log_debug("applyTreePolicy: Node at %p is non-terminal.\n", (void *)node);
        bool fullyExpanded = node->numPotentialMoves == node->numChildren;

        if(!fullyExpanded) {
            log_debug("applyTreePolicy: Node at %p is not fully expanded. Expanding it and returning child...\n", (void *)node);

            node = expandMCTSNode(node);
            setEdgeTaken(rootState, node->move);
            return node;
        }
        else {
            log_debug("applyTreePolicy: Node at %p is fully expanded. Choosing best child with UCT formula...\n", (void *)node);
            node = getBestChildUCB1(node);
            setEdgeTaken(rootState, node->move);
        }
    }

    return node;
}

static MCTSNode * getBestChildUCB1(const MCTSNode * node) {
    // Constant UCTK can be varied to change amount of exploration vs exploitation
    // Uses score = (c.totalScore / c.visits)/numBoxesLeft + UCTK * sqrt(2*log(n.visits)/c.visits)
    // where c represents a child and n the node.
    double UCTK = 0.7;

    MCTSNode * bestChild;
    double bestChildScore = -1.0;

    MCTSNode * c;
    double cScore;

    assert(node->child != NULL);

    c = node->child;
    while (c != NULL) {
        cScore = c->totalScore / (double)c->visits + UCTK * sqrt(2*log((double)node->visits)/(double)c->visits);

        if (cScore > bestChildScore) {
            bestChildScore = cScore;
            bestChild = c;
        }

        c = c->sibling;
    }

    log_debug("getBestChildUCB1: best child is: %p, UCT score: %G, move %d\n", (void *)bestChild, bestChildScore, bestChild->move);
    return bestChild;
}

static short getNumBoxesTakenUpTree(const MCTSNode * node, PlayerNum targetPlayer) {
    short numBoxes = 0;

    do {
        if (node->playerJustMoved == targetPlayer)
            numBoxes += node->numBoxesTakenByMove;
        node = node->parent;
    } while (node != NULL);

    return numBoxes;
}

// EXPAND
static MCTSNode * expandMCTSNode(MCTSNode * node) {
    log_debug("expandMCTSNode: Expanding node at %p...\n", (void *)node);

    Edge untriedMoves[node->numPotentialMoves - node->numChildren];
    short numUntriedMoves = getUntriedMovesMCTSNode(node, untriedMoves);

    Edge move = untriedMoves[randomInRange(0, numUntriedMoves-1)];
    node = addChildToMCTSNode(node, move);

    log_debug("Chose random (untried) move %d. Added child at %p. Move completes %d boxes.\n", move, (void *)node, node->numBoxesTakenByMove);
    return node;
}

static short getUntriedMovesMCTSNode(MCTSNode * node, Edge * untriedMovesBuffer) {
    MCTSNode * child = node->child;
    if (child == NULL)
        return getFreeEdges(&(node->state), untriedMovesBuffer);
    else {
        // Build a binary search tree with the already-tried child moves.
        // This is far more efficient than using a list.
        BTree * triedMoves = newBTree(child->move);

        while((child = child->sibling) != NULL) {
            insertBTree(triedMoves, child->move);
        }

        Edge freeEdges[node->numPotentialMoves];
        short numFreeEdges = getFreeEdges(&(node->state), freeEdges);
        short numUntriedMoves = 0;
        for(short i=0; i < numFreeEdges; i++) {
            if (!doesBTreeContain(triedMoves, freeEdges[i]))
                untriedMovesBuffer[numUntriedMoves++] = freeEdges[i];
        }

        return numUntriedMoves;
    }
}

static MCTSNode * addChildToMCTSNode(MCTSNode * parentNode, Edge move) {
    // Init child
    MCTSNode * newChild = (MCTSNode *)malloc( sizeof(MCTSNode) );
    UnscoredState newChildState = parentNode->state;
    setEdgeTaken(&newChildState, move);

    initMCTSNode(newChild, parentNode, newChildState, parentNode->nextPlayerToMove, move);

    // Add child to node
    if (parentNode->child == NULL)
        parentNode->child = newChild;
    else {
        MCTSNode * otherChild = parentNode->child;

        // Walk to the end of the linked list
        while(otherChild->sibling != NULL)
            otherChild = otherChild->sibling;

        otherChild->sibling = newChild;
    }

    parentNode->numChildren += 1;
    return newChild;
}

// SIMULATE
static double applyDefaultPolicy(const MCTSNode * leafNode, short rootNumBoxesLeft) {
    // Returns a value between 0.0 and 1.0 which is the proportion of boxes
    // taken by the first player to move from the given leafNode.
    if (rootNumBoxesLeft == 0)
        return 0.0;

    UnscoredState tmpState = leafNode->state;

    short boxesTaken = 0;
    short currentPlayer = leafNode->nextPlayerToMove;
    short numPotentialMoves = getNumFreeEdges(&tmpState);

    for(short i=0; i<numPotentialMoves; i++) {
        Edge move = getRandomMove(&tmpState);

        short boxesTakenByMove = howManyBoxesDoesMoveComplete(&tmpState, move);
        if(boxesTakenByMove == 0) {
            // It's the other player's turn
            currentPlayer = 3 - currentPlayer;
        }
        else if (currentPlayer == 1) {
            boxesTaken += boxesTakenByMove;
        }

        setEdgeTaken(&tmpState, move);
    }

    short boxesTakenUpTree = getNumBoxesTakenUpTree(leafNode, leafNode->nextPlayerToMove);
    return ((double)boxesTaken + (double)boxesTakenUpTree) / (double)rootNumBoxesLeft;
}

// BACKPROPAGATE
static void backpropagateResult(MCTSNode * node, PlayerNum scoreFirstPlayer, double score) {
    while(node != NULL) {
        if (node->nextPlayerToMove == scoreFirstPlayer)
            node->totalScore += score;
        else
            node->totalScore += 1.0 - score;

        node->visits += 1;

        log_debug("Updated node at %p. totalScore: %G, visits: %d\n", (void *)node, node->totalScore, node->visits);

        node = node->parent;
    }
}

static MCTSNode * getMostVisitedChild(MCTSNode * node) {
    MCTSNode * mostVisited;
    int mostVisits = -1;

    assert(node->child != NULL);
    MCTSNode * child = node->child;

    while(child != NULL) {
        if(child->visits > mostVisits) {
            mostVisited = child;
            mostVisits = child->visits;
        }
        child = child->sibling;
    }

    return mostVisited;
}

Edge getMCTSMove(UnscoredState * rootState, int runTimeMillis, bool saveTreeJSON) {
    MCTSNode * rootNode = (MCTSNode *)malloc( sizeof(MCTSNode) );
    initMCTSNode(rootNode, NULL, *rootState, NO_PLAYER, NO_EDGE);

    log_log("\ngetMCTSMove: STARTING. Root node at %p, numPotentialMoves: %d\n", (void *)rootNode, rootNode->numPotentialMoves);

    assert(runTimeMillis > 0);
    unsigned long long startTimeMillis = getTimeMillis();
    unsigned long long endTimeMillis = startTimeMillis + (unsigned long long)runTimeMillis;
    int iterationCount = 0;

    short rootNumBoxesLeft = getNumBoxesLeft(rootState);

    MCTSNode * node;
    UnscoredState nodeState;

    while(getTimeMillis() < endTimeMillis) {
        iterationCount++;
        log_debug("getMCTSMove: Iteration %d\n", iterationCount);

        node = rootNode;
        nodeState = *rootState;

        // Select & expand (apply tree policy)
        log_debug("getMCTSMove: Applying tree policy...\n");
        node = applyTreePolicy(node, &nodeState);

        // Simulate (apply default policy)
        log_debug("getMCTSMove: Applying default policy...\n");
        double score = applyDefaultPolicy(node, rootNumBoxesLeft);

        // Backpropagate
        log_debug("getMCTSMove: Backpropagating...\n");
        backpropagateResult(node, node->nextPlayerToMove, score);
    }

    log_log("Simulation complete! Ran for %d iterations. Average iteration duration (millis): %G.\n", iterationCount, (double)runTimeMillis/(double)iterationCount);
    log_log("Returning child with most visits...\n");
Edge bestMove = getMostVisitedChild(rootNode)->move;

    /*
    if(saveTreeJSON)
        saveMCTSNodeJSON(rootNode, "rootNode.json");
    */

    freeMCTSNode(rootNode);

    return bestMove;
}

/*
static void saveMCTSNodeJSON(const MCTSNode * node, const char * filePath) {
    json_t * j = MCTSNodeToJSON(node);
    json_dump_file(j, filePath, 0);
    json_decref(j);
}

static json_t * MCTSNodeToJSON(const MCTSNode * node) {
    json_t * j;

    char parentPointerString[32];
    sprintf(parentPointerString, "%p", (void *)node->parent);

    char nodePointerString[32];
    sprintf(nodePointerString, "%p", (void *)node);

    char stateString[NUM_EDGES+1];
    for(short i=0; i<NUM_EDGES; i++) {
        if(isEdgeTaken(&(node->state), i))
            stateString[i] = '1';
        else
            stateString[i] = '0';
    }
    stateString[NUM_EDGES] = '\0';

    json_t * children_array = json_array();
    MCTSNode * child = node->child;
    while(child != NULL) {
        json_array_append_new(children_array, MCTSNodeToJSON(child));
        child = child->sibling;
    }

    j = json_pack("{s:s, s:s, s:s, s:i, s:i, s:i, s:i, s:f, s:i, s:i, s:i, s:o}",
            "address", nodePointerString,
            "parent", parentPointerString,
            "state", stateString,
            "playerJustMoved", (int)node->playerJustMoved,
            "nextPlayerToMove", (int)node->nextPlayerToMove,
            "move", (int)node->move,
            "numBoxesTakenByMove", (int)node->numBoxesTakenByMove,
            "totalScore", node->totalScore,
            "visits", node->visits,
            "numPotentialMoves", (int)node->numPotentialMoves,
            "numChildren", (int)node->numChildren,
            "children", children_array);
    return j;
}
*/

void runMCTSTests() {
    log_log("RUNNING MCTS TESTS\n");

    log_log("\nTesting initMCTSNode...\n");
    UnscoredState rootState;
    stringToUnscoredState(&rootState, "000000000000000000000000000000000000000000000000000000000000000000000000");

    log_debug("It should behave correctly for the root node.\n");
    MCTSNode * rootNode = (MCTSNode *)malloc( sizeof(MCTSNode) );
    initMCTSNode(rootNode, NULL, rootState, NO_PLAYER, NO_EDGE); 
    assert(rootNode->parent == NULL);
    assert(rootNode->child == NULL);
    assert(rootNode->sibling == NULL);
    assert(rootNode->move = NO_EDGE);
    assert(rootNode->numBoxesTakenByMove == 0);
    assert(rootNode->playerJustMoved == NO_PLAYER);
    assert(rootNode->nextPlayerToMove == 1);
    assert(rootNode->totalScore == 0.0);
    assert(rootNode->visits == 0);
    assert(rootNode->numPotentialMoves == NUM_EDGES);
    assert(rootNode->numChildren == 0);

    log_log("\nTesting addChildToMCTSNode...\n");
    log_debug("It should behave correctly for a child node.\n");
    MCTSNode * firstChild = addChildToMCTSNode(rootNode, 1);
    assert(rootNode->child == firstChild);
    assert(rootNode->numChildren == 1);
    assert(firstChild->parent == rootNode);
    assert(firstChild->child == NULL);
    assert(firstChild->sibling == NULL);
    assert(firstChild->move = 1);
    assert(firstChild->numBoxesTakenByMove == 0);
    assert(firstChild->playerJustMoved == 1);
    assert(firstChild->nextPlayerToMove == 2);
    assert(firstChild->totalScore == 0.0);
    assert(firstChild->visits == 0);
    assert(firstChild->numPotentialMoves == NUM_EDGES - 1);
    assert(firstChild->numChildren == 0);

    log_debug("It should behave correctly for a second child node.\n");
    MCTSNode * secondChild = addChildToMCTSNode(rootNode, 2);
    assert(rootNode->child == firstChild);
    assert(rootNode->numChildren == 2);
    assert(firstChild->sibling == secondChild);
    assert(secondChild->parent == rootNode);
    assert(secondChild->child == NULL);
    assert(secondChild->sibling == NULL);
    assert(secondChild->move = 2);
    assert(secondChild->numBoxesTakenByMove == 0);
    assert(secondChild->playerJustMoved == 1);
    assert(secondChild->nextPlayerToMove == 2);
    assert(secondChild->totalScore == 0.0);
    assert(secondChild->visits == 0);
    assert(secondChild->numPotentialMoves == NUM_EDGES - 1);
    assert(secondChild->numChildren == 0);

    log_log("\nTesting applyTreePolicy...\n");
    log_debug("It should create a new node if the given node is not fully expanded.\n");
    MCTSNode * newNode = applyTreePolicy(rootNode, &rootState);
    assert(newNode != firstChild && newNode != secondChild);
    assert(rootNode->numChildren == 3);

    log_debug("It should modify the given state with the move of the node it picks.\n");
    assert(isEdgeTaken(&rootState, newNode->move) == true);

    log_debug("It should be invariant for terminal nodes.\n");
    UnscoredState terminalState;
    stringToUnscoredState(&terminalState, "111111111111111111111111111111111111111111111111111111111111111111111111");

    MCTSNode * terminalNode = (MCTSNode *)malloc( sizeof(MCTSNode) );
    initMCTSNode(terminalNode, NULL, terminalState, NO_PLAYER, NO_EDGE); 
    assert(terminalNode == applyTreePolicy(terminalNode, &terminalState));

    log_log("\nTesting getUntriedMovesMCTSNode...\n");
    log_debug("It should return no moves for a terminal node.\n");
    Edge untriedMoves[NUM_EDGES];
    short numUntriedMoves = getUntriedMovesMCTSNode(terminalNode, untriedMoves);
    assert(numUntriedMoves == 0);

    log_debug("It should return the correct number of moves for a node with some children.\n");
    numUntriedMoves = getUntriedMovesMCTSNode(rootNode, untriedMoves);
    assert(numUntriedMoves == rootNode->numPotentialMoves - rootNode->numChildren);

    log_log("\nTesting getBestChildUCB1...\n");
    log_debug("It should return the child with the highest UCB1 score...\n");
    rootNode->totalScore = 1.0;
    rootNode->visits = 2;

    firstChild->totalScore = 0.25;
    firstChild->visits = 1;

    secondChild->totalScore = 0.75;
    secondChild->visits = 1;

    assert(getBestChildUCB1(rootNode) == secondChild);

    log_log("\nTesting applyDefaultPolicy...\n");
    log_debug("It should return 0.0 for a terminal node.\n");
    double score = applyDefaultPolicy(terminalNode, 0);
    assert(score == 0.0);

    log_debug("It should return a sensible value for a non-terminal state.\n");
    MCTSNode * firstChildChild = addChildToMCTSNode(firstChild, 3);
    score = applyDefaultPolicy(firstChildChild, NUM_BOXES);
    log_debug("Score: %G\n", score);
    assert(score >= 0.0 && score <= 1.0);

    log_log("\nTesting backpropagateResult...\n");
    log_debug("It should increase the visits and total score of the leaf node.\n");
    backpropagateResult(firstChildChild, firstChildChild->nextPlayerToMove, score);
    assert(firstChildChild->visits == 1);
    assert(firstChildChild->totalScore == score);
    log_debug("firstChildChild->totalScore = %G\n", firstChildChild->totalScore);

    log_debug("It should increase the visits of the parent node.\n");
    assert(firstChild->visits == 2);

    log_debug("If the parent node's move was played by the other player, it's totalScore should increase by 1.0 - the backpropagated score.\n");
    // 0.25 is assigned above
    assert(firstChild->totalScore == 0.25 + 1.0 - score);

    log_log("\nTesting getMostVisitedChild...\n");
    log_debug("It should return the most visited child.\n");
    assert(getMostVisitedChild(rootNode) == firstChild);

    log_log("\nSaving test tree JSON...\n");
    // saveMCTSNodeJSON(rootNode, "testTree.json");

    log_log("\nTesting getMCTSMove...\n");
    log_debug("It should return a sensible result. (Saving JSON)\n");
    UnscoredState emptyState;
    initUnscoredState(&emptyState);
    Edge move = getMCTSMove(&emptyState, 1000, true);
    assert(move >= 0 && move < NUM_EDGES);

    freeMCTSNode(terminalNode);
    freeMCTSNode(rootNode);

    log_log("MCTS TESTS COMPLETED\n\n");
}
