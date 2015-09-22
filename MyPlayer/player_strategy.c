#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <jansson.h>
#include "game_board.h"
#include "player_strategy.h"
#include "mcts.h"
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

Edge chooseMove(UnscoredState state, Strategy strategy, int iterations) {
    switch(strategy) {
        case RANDOM_MOVE:
            return getRandomMove(&state);
            break;
        case FIRST_BOX_COMPLETING_MOVE:
            {
            Edge move = getFirstBoxCompletingMove(&state);

            if (move != NO_EDGE)
                return move;
            else
                return getRandomMove(&state);

            }
            break;
        case SIMPLE_MONTE_CARLO:
            return getSimpleMCTSMove(&state, iterations);
            break;
        case MONTE_CARLO:
            return getMCTSMove(&state, iterations, false);
            break;
    }

    return NO_EDGE; // Avoid compiler warnings.
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

    log_log("PLAYER_STRATEGY TESTS COMPLETED\n");
}
