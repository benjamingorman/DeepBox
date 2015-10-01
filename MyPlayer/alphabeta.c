#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <jansson.h>
#include "game_board.h"
#include "util.h"
#include "alphabeta.h"
#include "graphs.h"

static const short ALPHA_MIN = -100;
static const short BETA_MAX = 100;

static void saveABNodeJSON(const ABNode * node, UnscoredState state, const char * filePath);
static json_t * ABNodeToJSON(const ABNode * node, UnscoredState state);

static ABNode * newABRootNode(const UnscoredState * rootState) {
    ABNode * node = (ABNode *)malloc(sizeof(ABNode));
    
    node->alpha = ALPHA_MIN;
    node->beta = BETA_MAX;
    node->move = NO_EDGE;
    node->numBoxesTakenByMove = 0;
    //node->numPotentialMoves = getFreeEdges(rootState, node->potentialMoves);
    node->parent = NULL;
    node->child = NULL;
    node->sibling = NULL;
    node->numChildren = 0;

    node->isMaximizer = true;
    node->value = ALPHA_MIN;

    return node;
}

static ABNode * newABNode(ABNode * parent, Edge move, const UnscoredState * preMoveState) {
    ABNode * node = (ABNode *)malloc(sizeof(ABNode));

    node->alpha = parent->alpha;
    node->beta = parent->beta;
    node->move = move;
    node->numBoxesTakenByMove = howManyBoxesDoesMoveComplete(preMoveState, move);

    UnscoredState postMoveState = *preMoveState;
    setEdgeTaken(&postMoveState, move);
    //node->numPotentialMoves = getFreeEdges(&postMoveState, node->potentialMoves);

    node->parent = parent;
    node->child = NULL;
    node->sibling = NULL;
    node->numChildren = 0;

    if (node->numBoxesTakenByMove == 0)
        node->isMaximizer = !(parent->isMaximizer);
    else
        node->isMaximizer = parent->isMaximizer;

    if (node->isMaximizer)
        node->value = ALPHA_MIN;
    else
        node->value = BETA_MAX;

    return node;
}

static void freeABNode(ABNode * node) {
    if (node->numChildren != 0) { // recursively free it's children
        ABNode * currentChild = node->child;
        ABNode * nextChild;

        do {
            nextChild = currentChild->sibling;
            freeABNode(currentChild);
        } while ((currentChild = nextChild) != NULL);
    }

    free(node);
}

static void addChildToABNode(ABNode * parentNode, ABNode * childNode) {
    if (parentNode->child == NULL)
        parentNode->child = childNode;
    else {
        ABNode * otherChild = parentNode->child;
        // Walk to the end of the linked list
        while(otherChild->sibling != NULL)
            otherChild = otherChild->sibling;

        otherChild->sibling = childNode;
    }

    parentNode->numChildren += 1;
}

static ABNode * createABNodeAndAddToParent(ABNode * parent, Edge move, const UnscoredState * preMoveState) {
    ABNode * child = newABNode(parent, move, preMoveState);
    addChildToABNode(parent, child);
    log_debug("Created new ABNode at %p and added it to parent at %p\n", (void *)child, (void *)parent);

    return child;
}

static short doAlphaBeta(ABNode * node, UnscoredState * state, short depth, int * nodesVisitedCount, int * branchesPrunedCount, bool isRoot) {
    // If isRoot, returns the best move. Else returns a score for the node.
   
    log_debug("doAlphaBeta called for node at %p with move %d and depth %d\n", (void *)node, node->move, depth);
    *nodesVisitedCount += 1;

    short numFreeEdges = getNumFreeEdges(state);
    if (depth == 0 || numFreeEdges == 0) { // Node is terminal
        log_debug("Depth is 0...\n");
        short score = 0;

        if (depth == 0 && numFreeEdges > 0) { // Compute a heuristic value for the node
            SCGraph graph;
            unscoredStateToSCGraph(&graph, state);

            short initRemainingNodes = getNumNodesLeftToCapture(&graph);

            // While urgent moves exist, make them.
            Edge urgentMovesBuf[2];
            while(getSuperGraphUrgentMoves(&graph, urgentMovesBuf) == 1)
                removeConnectionEdge(&graph, urgentMovesBuf[0]);

            short finalRemainingNodes = getNumNodesLeftToCapture(&graph);
            short nodesTaken = initRemainingNodes - finalRemainingNodes;

            if (node->isMaximizer)
                score += nodesTaken;

            // Then assume we get half of what's left
            score += (int)finalRemainingNodes/2.0;
            freeAdjLists(&graph);
        }

        // Count up the tree the number of boxes that have been taken.
        do {
            if (node->isMaximizer)
                score += node->numBoxesTakenByMove;
            node = node->parent;
        } while (node != NULL);

        log_debug("score for node: %d\n", score);
        return score;
    }

    // Else enumerate the possible moves and try them.
    SCGraph graph;
    unscoredStateToSCGraph(&graph, state);

    Edge potentialMoves[NUM_EDGES];
    short numPotentialMoves = getGraphsPotentialMoves(&graph, potentialMoves);

    { // Order the moves so those that give away boxes are considered last. Using a new scope like this saves memory.
        Edge terribleMoves[NUM_EDGES];
        Edge badMoves[NUM_EDGES];
        Edge goodMoves[NUM_EDGES];
        short numBadMoves = 0;
        short numGoodMoves = 0;
        short numTerribleMoves = 0;
        for(short i=0; i < numPotentialMoves; i++) {
            Edge edge = potentialMoves[i];
            const Box * edgeBoxes = getEdgeBoxes(edge);

            short node1, node2;
            if(edgeBoxes[0] == NO_BOX)
                node1 = 0;
            else
                node1 = graph.boxToNode[edgeBoxes[0]];

            if(edgeBoxes[1] == NO_BOX)
                node2 = 0;
            else
                node2 = graph.boxToNode[edgeBoxes[1]];

            short badness = 0;
            if (getNodeValency(&graph, node1) == 2)
                badness++;
            if (getNodeValency(&graph, node2) == 2)
                badness++;

            switch(badness) {
                case 0:
                    goodMoves[numGoodMoves++] = edge;
                    break;
                case 1:
                    badMoves[numBadMoves++] = edge;
                    break;
                case 2:
                    terribleMoves[numTerribleMoves++] = edge;
                    break;
            }
        }

        for (short i=0; i < numGoodMoves; i++)
            potentialMoves[i] = goodMoves[i];

        for (short i=0; i < numBadMoves; i++)
            potentialMoves[numGoodMoves + i] = badMoves[i];

        for (short i=0; i < numTerribleMoves; i++)
            potentialMoves[numGoodMoves + numBadMoves + i] = terribleMoves[i];
    }

    Edge bestMove = NO_EDGE;
    for(short i=0; i < numPotentialMoves; i++) {
        Edge untriedMove = potentialMoves[i];
        // untriedMove might be a corner move in which case it may need to be converted
        if (isEdgeTaken(state, untriedMove))
            untriedMove = getCorrespondingCornerEdge(untriedMove);

        ABNode * child = createABNodeAndAddToParent(node, untriedMove, state);


        setEdgeTaken(state, untriedMove); // now it's a postMoveState
        
        short v = doAlphaBeta(child, state, depth-1, nodesVisitedCount, branchesPrunedCount, false);
        setEdgeFree(state, untriedMove); // reset the state

        if(isRoot)
            log_log("Checked untried move %d. Score is: %d\n", untriedMove, v);

        if (node->isMaximizer) {
            if (v > node->value) {
                node->value = v;
                bestMove = untriedMove;
            }
            node->alpha = max(node->alpha, v);
            if (node->beta <= node->alpha) {
                *branchesPrunedCount += 1;
                log_debug("Pruned branch with beta cutoff!\n"); 
                break; // beta cutoff
            }
        }
        else { // node is minimizer
            if (v < node->value) {
                node->value = v;
                bestMove = untriedMove;
            }
            node->beta = min(node->beta, v);

            if (node->beta <= node->alpha) {
                *branchesPrunedCount += 1;
                log_debug("Pruned branch with alpha cutoff!\n"); 
                break; // alpha cutoff
            }
        }
    }

    freeAdjLists(&graph);

    if (isRoot)
        return bestMove;
    else
        return node->value;
}

Edge getABMove(const UnscoredState * state, short maxDepth, bool saveJSON) {
    log_log("\nStarting getABMove with maxDepth %d\n", maxDepth);

    unsigned long long startTime = getTimeMillis();
    int nodesVisitedCount = 0;
    int branchesPrunedCount = 0;
    
    ABNode * rootNode = newABRootNode(state);
    UnscoredState rootState = *state;

    printUnscoredState(&rootState);

    Edge bestMove = doAlphaBeta(rootNode, &rootState, maxDepth, &nodesVisitedCount, &branchesPrunedCount, true);
    log_log("getABMove: Best move is %d.\n", bestMove);

    freeABNode(rootNode);

    unsigned long long endTime = getTimeMillis();
    long timeSpent = endTime - startTime;
    
    log_log("Time spent: %ld, Nodes visited: %d, Branches pruned: %d\n", timeSpent, nodesVisitedCount, branchesPrunedCount);

    return bestMove;
}

static json_t * ABNodeToJSON(const ABNode * node, UnscoredState state) {
    json_t * j;

    if (node->move != NO_EDGE)
        setEdgeTaken(&state, node->move);

    char stateString[NUM_EDGES+1];
    for(short i=0; i<NUM_EDGES; i++) {
        if(isEdgeTaken(&state, i))
            stateString[i] = '1';
        else
            stateString[i] = '0';
    }
    stateString[NUM_EDGES] = '\0';

    char parentPointerString[32];
    sprintf(parentPointerString, "%p", (void *)node->parent);

    char nodePointerString[32];
    sprintf(nodePointerString, "%p", (void *)node);


    json_t * children_array = json_array();
    ABNode * child = node->child;
    while(child != NULL) {
        json_array_append_new(children_array, ABNodeToJSON(child, state));
        child = child->sibling;
    }

    j = json_pack("{s:s, s:s, s:s, s:f, s:f, s:i, s:i, s:i, s:o, s:b, s:f}",
            "state", stateString,
            "address", nodePointerString,
            "parent", parentPointerString,
            "alpha", node->alpha,
            "beta", node->beta,
            "move", node->move,
            "numBoxesTakenByMove", node->numBoxesTakenByMove,
            "numChildren", node->numChildren,
            "children", children_array,
            "isMaximizer", node->isMaximizer,
            "value", node->value);

    return j;
}

static void saveABNodeJSON(const ABNode * node, UnscoredState rootState, const char * filePath) {
    json_t * j = ABNodeToJSON(node, rootState);
    json_dump_file(j, filePath, 0);
    json_decref(j);
}

void runAlphaBetaTests() {
    log_log("RUNNING ALPHA BETA TESTS\n");
    log_log("ALPHA BETA TESTS COMPLETED\n\n");
}
