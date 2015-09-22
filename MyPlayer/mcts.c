#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <jansson.h>
#include <sys/time.h>
#include "game_board.h"
#include "mcts.h"
#include "player_strategy.h"
#include "util.h"

void initMCTSNode(MCTSNode * node, MCTSNode * parent, UnscoredState state, PlayerNum playerJustMoved, Edge move) {
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
    node->totalScore = 0; // from perspective of playerJustMoved
    node->numPotentialMoves = getNumFreeEdges(&state);
    node->numChildren = 0;
    node->child = NULL;
    node->sibling = NULL;
}

void updateMCTSNode(MCTSNode * node, short gameScore) {
    node->visits += 1;
    node->totalScore += node->numBoxesTakenByMove + gameScore;
}

MCTSNode * addChildToMCTSNode(MCTSNode * parentNode, Edge move) {
    // Init child
    MCTSNode * newChild = (MCTSNode *)malloc( sizeof(MCTSNode) );
    UnscoredState newChildState = parentNode->state;
    setEdgeTaken(&newChildState, move);

    PlayerNum newChildPlayer = parentNode->nextPlayerToMove;

    initMCTSNode(newChild, parentNode, newChildState, newChildPlayer, move);

    // Add child to node
    if (parentNode->child == NULL) {
        parentNode->child = newChild;
    } 
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

void freeMCTSNode(MCTSNode * node) {
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

short doRandomGame(UnscoredState state) {
    // Returns the score for the player who's turn it is first.

    short score = 0;
    short currentPlayer = 1;
    short numPotentialMoves = getNumFreeEdges(&state);

    for(short i=0; i<numPotentialMoves; i++) {
        Edge move = getRandomMove(&state);

        short boxesCompleted = howManyBoxesDoesMoveComplete(&state, move);
        if(boxesCompleted == 0) {
            // It's the other player's turn
            currentPlayer = 3 - currentPlayer;
        }
        else if (currentPlayer == 1) {
            score += boxesCompleted;
        }

        setEdgeTaken(&state, move);
    }

    return score;
}

Edge getSimpleMCTSMove(UnscoredState * rootState, int iterations) {
    // Simple monte-carlo search. Simulate 'iterations' number of games for each potential move.
    // Return the move with the highest average score.

    MCTSNode * rootNode = (MCTSNode *)malloc( sizeof(MCTSNode) );
    initMCTSNode(rootNode, NULL, *rootState, NO_PLAYER, NO_EDGE);

    Edge potentialMoves[NUM_EDGES];
    short numPotentialMoves = getFreeEdges(rootState, potentialMoves);

    MCTSNode * bestChild;
    short bestChildScore = -100;
    for(short i=0; i<numPotentialMoves; i++) {
        MCTSNode * child = addChildToMCTSNode(rootNode, potentialMoves[i]);
        log_debug("Simulating move %d for %d iterations...\n", child->move, iterations);

        for(int j=0; j<iterations; j++) {
            short myScore;
            if (child->nextPlayerToMove == 1)
                myScore = doRandomGame(child->state);
            else
                myScore = NUM_BOXES - doRandomGame(child->state);

            child->totalScore += myScore;
            child->visits += 1;
        }
        log_log("Total score over all random games: %d\n", child->totalScore);

        if (child->totalScore > bestChildScore) {
            bestChildScore = child->totalScore;
            bestChild = child;
        }
    }

    Edge bestMove = bestChild->move;
    freeMCTSNode(rootNode);

    return bestMove;
}

MCTSNode * selectChildUCT(MCTSNode * node, short numBoxesLeft) {
    // Constant UCTK can be varied to change amount of exploration vs exploitation
    // Uses score = c.totalScore / c.visits + UCTK * sqrt(2*log(n.visits)/c.visits)
    // where c represents a child and n the node.
    log_debug("Selecting child with UCT formula...");
    double UCTK = 0.7;

    MCTSNode * bestChild;
    double bestChildScore = -1.0;

    MCTSNode * c;
    double cAverageNumBoxes;
    double cScore;

    if (node->child == NULL) {
        log_warn("[WARN] selectChildUCT called for a node with no children!");
        return NULL;
    }
    else {
        c = node->child;
        while (c != NULL) {
            cAverageNumBoxes = (double)c->totalScore / (double)c->visits;
            cScore = cAverageNumBoxes / (double)numBoxesLeft + UCTK * sqrt(2*log((double)node->visits)/(double)c->visits);

            if (cScore > bestChildScore) {
                bestChildScore = cScore;
                bestChild = c;
            }

            c = c->sibling;
        }

        log_debug("best child is: %p, UCT score: %G, move %d\n", (void *)bestChild, bestChildScore, bestChild->move);
        return bestChild;
    }
}

Edge getMCTSMove(UnscoredState * rootState, int iterations, bool saveTree) {
    MCTSNode * rootNode = (MCTSNode *)malloc( sizeof(MCTSNode) );
    initMCTSNode(rootNode, NULL, *rootState, NO_PLAYER, NO_EDGE);

    log_log("\nGetting MCTS move. Root node at %p, numPotentialMoves: %d\n", (void *)rootNode, rootNode->numPotentialMoves);
    unsigned long long startTimeMillis = getTimeMillis();
    unsigned long long endTimeMillis = startTimeMillis + 5000; // run for 5 seconds
    int iterationCount = 0;

    MCTSNode * node;
    UnscoredState nodeState;
    short numUntriedMoves;
    Edge move;
    PlayerNum randomGameFirstPlayer;
    short randomGameScore;
    
    while(getTimeMillis() < endTimeMillis) {
        iterationCount++;
        log_debug("Iteration: %d\n", iterationCount);
        node = rootNode;
        nodeState = *rootState;

        // Select
        while ((numUntriedMoves = node->numPotentialMoves - node->numChildren) == 0 && node->numChildren != 0) {
            log_debug("Node at %p is fully expanded and non-terminal.\n", (void *)node);
            short numBoxesLeft = getNumBoxesLeft(&(node->state));
            node = selectChildUCT(node, numBoxesLeft);
            setEdgeTaken(&nodeState, node->move);
        }

        // Expand
        if (node->numPotentialMoves != 0) { // node is non-terminal
            log_debug("Expanding node...");
            move = getRandomMove(&nodeState);
            setEdgeTaken(&nodeState, move);
            node = addChildToMCTSNode(node, move); // add child and descend tree
            log_debug("Chose random move %d. Move completes %d boxes.\n", move, node->numBoxesTakenByMove);
        }

        // Rollout
        log_debug("Simulating random game...");
        randomGameFirstPlayer = node->nextPlayerToMove;
        randomGameScore = doRandomGame(node->state);

        // Backpropagate
        log_debug("Backpropagating...");
        while(node != NULL) {
            if (node->playerJustMoved == randomGameFirstPlayer)
                updateMCTSNode(node, randomGameScore);
            else
                updateMCTSNode(node, getNumBoxesLeft(&nodeState) - randomGameScore);

            log_debug("Updated node at %p. totalScore: %d, visits: %d\n", (void *)node, node->totalScore, node->visits);

            node = node->parent;
        }
    }

    log_log("Simulation complete! Ran for %d iterations. Average iteration duration (millis): %G.\n", iterationCount, (double)5000/(double)iterationCount);
    log_log("Returning child with most visits...\n");
    
    MCTSNode * mostVisited;
    int mostVisits = -1;

    MCTSNode * child = rootNode->child;

    while(child != NULL) {
        if(child->visits > mostVisits) {
            log_debug("New most visited: child at %p, move: %d, totalScore: %d, visits: %d\n", (void *)child, child->move, child->totalScore, child->visits);
            mostVisited = child;
            mostVisits = child->visits;
        }
        child = child->sibling;
    }

    Edge bestMove = mostVisited->move;

    if(saveTree)
        saveMCTSNodeJSON(rootNode, "rootNode.json");

    freeMCTSNode(rootNode);

    return bestMove;
}

void saveMCTSNodeJSON(MCTSNode * node, const char * filePath) {
    json_t * j = MCTSNodeToJSON(node);
    json_dump_file(j, filePath, 0);
    json_decref(j);
}

json_t * MCTSNodeToJSON(MCTSNode * node) {
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

    j = json_pack("{s:s, s:s, s:s, s:i, s:i, s:i, s:i, s:i, s:i, s:i, s:i, s:o}", "address", nodePointerString, "parent", parentPointerString, "state", stateString, "playerJustMoved", (int)node->playerJustMoved, "nextPlayerToMove", (int)node->nextPlayerToMove, "move", (int)node->move, "numBoxesTakenByMove", (int)node->numBoxesTakenByMove, "totalScore", node->totalScore, "visits", node->visits, "numPotentialMoves", (int)node->numPotentialMoves, "numChildren", (int)node->numChildren, "children", children_array);
    return j;
}

void runMCTSTests() {
    log_log("RUNNING MCTS TESTS\n");

    log_log("Testing initMCTSNode...\n");
    UnscoredState rootState;
    stringToUnscoredState(&rootState, "000000000000000000000000000000000000000000000000000000000000000000000000");

    MCTSNode * rootNode = (MCTSNode *)malloc( sizeof(MCTSNode) );
    initMCTSNode(rootNode, NULL, rootState, NO_PLAYER, NO_EDGE); 
    assert(rootNode->numChildren == 0);
    assert(rootNode->child == NULL);
    assert(rootNode->sibling == NULL);
    assert(rootNode->nextPlayerToMove == 1);
    assert(rootNode->numBoxesTakenByMove == 0);

    log_log("Testing addChildToMCTSNode...\n");
    {
        log_log("firstChildNode...\n");
        MCTSNode * firstChildNode = addChildToMCTSNode(rootNode, 0);
        assert(rootNode->numChildren == 1);
        assert(rootNode->child == firstChildNode);
        assert(firstChildNode->playerJustMoved == 1);
        assert(firstChildNode->nextPlayerToMove == 2);
        assert(firstChildNode->sibling == NULL);
        assert(firstChildNode->state.edges[0] == TAKEN);

        log_log("secondChildNode...\n");
        MCTSNode * secondChildNode = addChildToMCTSNode(rootNode, 1);
        assert(rootNode->numChildren == 2);
        assert(rootNode->child == firstChildNode);
        assert(firstChildNode->sibling == secondChildNode);
        assert(secondChildNode->playerJustMoved == 1);
        assert(firstChildNode->nextPlayerToMove == 2);
        assert(secondChildNode->sibling == NULL);
        assert(secondChildNode->state.edges[0] == FREE);
        assert(secondChildNode->state.edges[1] == TAKEN);

        log_log("subchild of secondChildNode...\n");
        MCTSNode * subchild = addChildToMCTSNode(secondChildNode, 2);
        assert(secondChildNode->numChildren == 1);
        assert(subchild->parent == secondChildNode);
        assert(subchild->playerJustMoved == 2);
        assert(subchild->nextPlayerToMove == 1);
    }

    log_log("Testing whether doRandomGame returns 0 for full board...\n");
    stringToUnscoredState(&rootState, "111111111111111111111111111111111111111111111111111111111111111111111111");
    assert(doRandomGame(rootState) == 0);

    log_log("Testing whether doRandomGame returns 1 when one edge remaining...\n");
    stringToUnscoredState(&rootState, "101111111111111111111111111111111111111111111111111111111111111111111111");
    assert(doRandomGame(rootState) == 1);

    log_log("Testing whether getMCTSMove returns something valid...\n");
    stringToUnscoredState(&rootState, "000000000000000000000000000000000000000000000000000000000000000000000000");
    Edge move = getMCTSMove(&rootState, 200, false);
    assert(move >= 0 && move < NUM_EDGES);

    log_log("Testing whether getMCTSMove returns the only available move...\n");
    stringToUnscoredState(&rootState, "111111111011111111111111111111111111111111111111111111111111111111111111");
    move = getMCTSMove(&rootState, 10, false);
    assert(move == 9);

    log_log("Testing saving MCTSNode JSON...\n");
    stringToUnscoredState(&rootState, "101001001010010111010010110011001000100000000001111100001010001001001011");
    getMCTSMove(&rootState, 1000, true);
    
    log_log("Freeing root node...\n");
    freeMCTSNode(rootNode);

    log_log("MCTS TESTS COMPLETED\n");
}
