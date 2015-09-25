#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <jansson.h>
#include "game_board.h"
#include "util.h"
#include "alphabeta.h"

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

static short doAlphaBeta(ABNode * node, UnscoredState * state, short depth, int * nodesVisitedCount, int * branchesPrunedCount) {
    log_debug("doAlphaBeta called for node at %p with move %d and depth %d\n", (void *)node, node->move, depth);
    *nodesVisitedCount += 1;

    // TODO: Clean this up
    // Sort the potential moves in order with box completing moves first.
    // Filter out corresponding corner pairs since only one needs to be visited.
    // Use a new scope to save memory.
    Edge potentialMoves[NUM_EDGES];
    short numPotentialMoves = 0;
    {
        BTree * excludedCornerSet = initBTree(NO_EDGE); 

        short numBoxCompletingMoves = 0;
        Edge boxCompletingMoves[NUM_EDGES];
        short numNonBoxCompletingMoves = 0;
        Edge nonBoxCompletingMoves[NUM_EDGES];

        for (Edge e=0; e < NUM_EDGES; e++) {
            if (state->edges[e] == FREE && !doesBTreeContain(excludedCornerSet, e)) {
                if (isBoxCompletingMove(state, e)) {
                    boxCompletingMoves[numBoxCompletingMoves++] = e;

                    // The edge may not be a corner, in which case NO_EDGE is inserted into the tree.
                    // This is not a problem since it was initialized on that value anyway.
                    insertBTree(excludedCornerSet, getCorrespondingCornerEdge(e));
                }
                else {
                    nonBoxCompletingMoves[numNonBoxCompletingMoves++] = e;
                    insertBTree(excludedCornerSet, getCorrespondingCornerEdge(e));
                }
            }
        }

        freeBTree(excludedCornerSet);
        
        // Read the box completing moves and then the non-box completing moves into potentialMoves.
        for(short i=0; i<numBoxCompletingMoves; i++) {
            potentialMoves[i] = boxCompletingMoves[i];
        }

        for(short i=0; i<numNonBoxCompletingMoves; i++) {
            potentialMoves[numBoxCompletingMoves+i] = nonBoxCompletingMoves[i];
        }

        numPotentialMoves = numBoxCompletingMoves + numNonBoxCompletingMoves;
    }

    if (numPotentialMoves == 0 || depth == 0) { // TODO: add extra heuristics for when depth is 0
        log_debug("Node is terminal or depth is 0...\n");
        // Count up the tree the number of boxes that have been taken.
        short totalNumBoxes = 0;

        do {
            if (node->isMaximizer)
                totalNumBoxes += node->numBoxesTakenByMove;
            node = node->parent;
        } while (node != NULL);

        log_debug("totalNumBoxes counted: %d\n", totalNumBoxes);
        return totalNumBoxes;
    }
    else {
        for(short i=0; i < numPotentialMoves; i++) {
            Edge untriedMove = potentialMoves[i];
            log_debug("Checking move %d (%d of %d)...\n", untriedMove, i+1, numPotentialMoves);

            ABNode * child = createABNodeAndAddToParent(node, untriedMove, state);

            setEdgeTaken(state, untriedMove); // now it's a postMoveState
            double v = doAlphaBeta(child, state, depth-1, nodesVisitedCount, branchesPrunedCount);
            setEdgeFree(state, untriedMove); // reset the state

            log_debug("Node v is %G\n", v);

            if (node->isMaximizer) {
                node->value = max(node->value, v);
                node->alpha = max(node->alpha, v);
                if (node->beta <= node->alpha) {
                    *branchesPrunedCount += 1;
                    log_debug("Pruned branch with beta cutoff!\n"); 
                    break; // beta cutoff
                }
            }
            else { // node is minimizer
                node->value = min(node->value, v);
                node->beta = min(node->beta, v);

                if (node->beta <= node->alpha) {
                    *branchesPrunedCount += 1;
                    log_debug("Pruned branch with alpha cutoff!\n"); 
                    break; // alpha cutoff
                }
            }
        }
    }

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

    short v = doAlphaBeta(rootNode, &rootState, maxDepth, &nodesVisitedCount, &branchesPrunedCount);
    log_log("getABMove: best v is %d\n", v);

    // Find the child that v came from:
    // Perhaps doAlphaBeta could return a node instead? This would save a bit of computation.
    // It would also make things a little bit more complicated during the actual algorithm.
    
    ABNode * child;
    Edge bestMove;

    child = rootNode->child;
    do {
        if (child->value == v) {
            bestMove = child->move;
            break;
        }
    } while ((child = child->sibling) != NULL);

    log_log("getABMove: best move is %d\n", bestMove);

    if (saveJSON) {
        log_log("Saving root node JSON to abRootNode.json\n");
        saveABNodeJSON(rootNode, rootState, "abRootNode.json");
    }

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

    /*
    char potentialMovesBuffer[512];
    char * p = potentialMovesBuffer;
    for(short i=0; i < node->numPotentialMoves; i++) {
        sprintf(p, "%02d,", node->potentialMoves[i]);
        p += 3;
    }
    */

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
            //"potentialMoves", potentialMovesBuffer,
            //"numPotentialMoves", node->numPotentialMoves,
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
