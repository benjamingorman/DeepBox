#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "game_board.h"
#include "player_strategy.h"

int randomInRange(unsigned int min, unsigned int max) {
    // Credit: http://stackoverflow.com/questions/2509679/how-to-generate-a-random-number-from-within-a-range
    int r;

    const unsigned int range = 1 + max - min;
    const unsigned int buckets = RAND_MAX / range;
    const unsigned int limit = buckets * range;

    /* Create equal size buckets all in a row, then fire randomly towards
     * the buckets until you land in one of them. All buckets are equally
     * likely. If you land off the end of the line of buckets, try again. */
    do {
        r = rand();
    } while (r >= limit);

    return min + (r / buckets);
}

Edge getRandomMove(UnscoredState * state) {
    Edge freeEdges[NUM_EDGES];
    short numFreeEdges = 0;
    for(short i=0; i<NUM_EDGES; i++) {
        if (state->edges[i] == FREE)
            freeEdges[numFreeEdges++] = i;
    }

    return freeEdges[randomInRange(0, numFreeEdges-1)];
}

Edge getFirstBoxCompletingMove(UnscoredState * state) {
    const Edge * boxEdges;
    Edge edge;
    short boxScore; // How many of the box's edges are taken.

    for(Box b=0; b < NUM_BOXES; b++) {
        boxEdges = getBoxEdges(b);
        boxScore = 0;

        for(short i=0; i<4; i++) {
            edge = boxEdges[i];
            if (isEdgeTaken(state, edge))
                boxScore++;
        }

        if(boxScore == 3) { // A box completing move is available
            // Find the free edge:
            for(short i=0; i<4; i++) {
                if (!isEdgeTaken(state, boxEdges[i]))
                    return boxEdges[i];
            }
        }
    }

    return -1;
}

Edge chooseMove(UnscoredState state, Strategy strategy) {
    switch(strategy) {
        case RANDOM_MOVE:
            return getRandomMove(&state);
            break;
        case FIRST_BOX_COMPLETING_MOVE:
            {
            Edge move = getFirstBoxCompletingMove(&state);

            if (move != -1)
                return move;
            else
                return getRandomMove(&state);

            }
            break;
    }

    return -1; // Avoid compiler warnings.
}

void runPlayerStrategyTests() {
    puts("RUNNING PLAYER_STRATEGY TESTS\n");

    puts("Testing getRandomMove...");
    UnscoredState state;
    stringToUnscoredState(&state, "111101111111111111111111111111111111111111111111111111111111111111111111"); 
    Edge edge = getRandomMove(&state);
    printf("getRandomMove returned %d\n", edge);
    assert(edge == 4);

    stringToUnscoredState(&state, "100111111111111111111111111111111111111111111111111111111111111111111111"); 
    edge = getRandomMove(&state);
    assert(edge == 1 || edge == 2);

    puts("Testing getFirstBoxCompletingMove...");
    stringToUnscoredState(&state, "010100000101000000101000000000000000000000000000000000000000000000000000"); 
    edge = getFirstBoxCompletingMove(&state);
    printf("getFirstBoxCompletingMove returned %d\n", edge);
    assert(edge == 10);

    stringToUnscoredState(&state, "000000000000000000000000000000000000000000000000000000000000000000000000"); 
    edge = getFirstBoxCompletingMove(&state);
    printf("getFirstBoxCompletingMove returned %d\n", edge);
    assert(edge == -1);

    puts("PLAYER_STRATEGY TESTS COMPLETED\n");
}
