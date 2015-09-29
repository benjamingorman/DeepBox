#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <jansson.h>
#include "game_board.h"
#include "player_strategy.h"
#include "mcts.h"
#include "graphs.h"
#include "alphabeta.h"
#include "util.h"

Edge getRandomMove(UnscoredState * state) {
    Edge freeEdges[NUM_EDGES];
    short numFreeEdges = getFreeEdges(state, freeEdges);

    return freeEdges[randomInRange(0, numFreeEdges-1)];
}

Edge getRandomMoveFromList(Edge * edges, short numEdges) {
    return edges[randomInRange(0, numEdges-1)];
}

Edge getFirstBoxCompletingMove(UnscoredState * state) {
    const Edge * boxEdges;

    for(Box b=0; b < NUM_BOXES; b++) {
        if(getBoxNumTakenEdges(state, b) == 3) {
            // Find the first free edge
            boxEdges = getBoxEdges(b);

            for(short i=0; i<4; i++) {
                if (!isEdgeTaken(state, boxEdges[i]))
                    return boxEdges[i];
            }
        }
    }

    return NO_EDGE;
}

Edge getMoveAlways4Never3(UnscoredState * state) {
    // Returns a box completing move if one exists.
    // Else returns a move which does not form the 3rd edge of a box.
    // Else returns a random move.
    
    Edge move = getFirstBoxCompletingMove(state);
    if (move != NO_EDGE) {
        log_debug("always4never3: return a box completing move\n");
        return move;
    }
    else {
        Edge potentialMoves[NUM_EDGES];
        short numPotentialMoves = getFreeEdges(state, potentialMoves);
        log_debug("always4never3: numPotentialMoves is %d\n", numPotentialMoves);

        for (short i=0; i < numPotentialMoves; i++) {
            Edge e = potentialMoves[i];
            const Box * edgeBoxes = getEdgeBoxes(e);
            Box b0 = edgeBoxes[0];
            Box b1 = edgeBoxes[1];

            short b0takenEdges = 0;
            if (b0 != NO_BOX)
                b0takenEdges = getBoxNumTakenEdges(state, b0);

            short b1takenEdges = 0;
            if (b1 != NO_BOX)
                b1takenEdges = getBoxNumTakenEdges(state, b1);

            if (b0takenEdges <= 1 && b1takenEdges <= 1) {
                log_debug("always4never3: return a not-3 move\n");
                return e;
            }
        }

        // If at this point no edge has been found, then just return a random one.
        log_debug("always4never3: returning a random move\n");
        return getRandomMoveFromList(potentialMoves, numPotentialMoves);
    }
}

Edge getDeepBox1Move(UnscoredState * state, int turnTimeMillis) {
    short numEdgesLeft = getNumFreeEdges(state);

    if (numEdgesLeft <= 14) {
        log_log("Using alpha-beta strategy...\n");
        return getABMove(state, 14, false);
    }
    else if (numEdgesLeft <= 40) {
        log_log("Using monte-carlo strategy...\n");
        return getMCTSMove(state, turnTimeMillis, false);
    }
    else {
        log_log("Using always4never3 strategy...\n");
        return getMoveAlways4Never3(state);
    }
}

Edge getGraphsMove(UnscoredState * state) {
    SCGraph graph;
    unscoredStateToSCGraph(&graph, state);

    Edge potentialMoves[NUM_EDGES];
    short numPotentialMoves = getGraphsPotentialMoves(&graph, potentialMoves);

    // In the graph representation, corner edges are the same.
    // This means that a 'potential move' returned might actually be a corner edge that is already taken.
    // In this scenario it must be converted to its equivalent corner edge.
    for(short i=0; i < numPotentialMoves; i++) {
        Edge move = potentialMoves[i];
        if (isEdgeTaken(state, move)) {
            log_log("getGraphsMove: Converting taken corner edge %d to %d.\n", move, getCorrespondingCornerEdge(move));
            potentialMoves[i] = getCorrespondingCornerEdge(move);
        }
    }

    log_log("getGraphsMove: getGraphsPotentialMoves found %d moves. Choosing the first move which is %d.\n", numPotentialMoves, potentialMoves[0]);

    return potentialMoves[0];
}

Edge chooseMove(UnscoredState state, Strategy strategy, int turnTimeMillis) {
    assert(turnTimeMillis > 0);

    Edge moveChoice = NO_EDGE;

    unsigned long long startTime = getTimeMillis();

    switch(strategy) {
        case RANDOM_MOVE:
            moveChoice =  getRandomMove(&state);
            break;
        case FIRST_BOX_COMPLETING_MOVE:
            {
            Edge move = getFirstBoxCompletingMove(&state);

            if (move != NO_EDGE)
                moveChoice = move;
            else
                moveChoice = getRandomMove(&state);

            }
            break;
        case MONTE_CARLO:
            moveChoice = getMCTSMove(&state, turnTimeMillis, false);
            break;
        case ALPHA_BETA:
            moveChoice = getABMove(&state, 20, false);
            break;
        case GRAPHS:
            moveChoice = getGraphsMove(&state);
            break;
        case DEEPBOX1:
            moveChoice = getDeepBox1Move(&state, turnTimeMillis);
            break;
    }

    unsigned long long endTime = getTimeMillis();
    unsigned long long timeTaken = endTime - startTime;
    log_log("chooseMove: Chose %d. Time taken to choose: %llu.\n", moveChoice, timeTaken);

    return moveChoice; // Avoid compiler warnings.
}

void runPlayerStrategyTests() {
    log_log("RUNNING PLAYER_STRATEGY TESTS\n");

    log_log("Testing getRandomMove...\n");
    UnscoredState state;
    stringToUnscoredState(&state, "111101111111111111111111111111111111111111111111111111111111111111111111"); 
    Edge edge = getRandomMove(&state);
    log_log("getRandomMove returned %d\n", edge);
    assert(edge == 4);

    stringToUnscoredState(&state, "100111111111111111111111111111111111111111111111111111111111111111111111"); 
    edge = getRandomMove(&state);
    assert(edge == 1 || edge == 2);

    log_log("Testing getFirstBoxCompletingMove...\n");
    stringToUnscoredState(&state, "010100000101000000101000000000000000000000000000000000000000000000000000"); 
    edge = getFirstBoxCompletingMove(&state);
    log_log("getFirstBoxCompletingMove returned %d\n", edge);
    assert(edge == 10);

    stringToUnscoredState(&state, "000000000000000000000000000000000000000000000000000000000000000000000000"); 
    edge = getFirstBoxCompletingMove(&state);
    log_log("getFirstBoxCompletingMove returned %d\n", edge);
    assert(edge == NO_EDGE);

    log_log("PLAYER_STRATEGY TESTS COMPLETED\n\n");
}
