#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <wchar.h>
#include <stdbool.h>

#include "game_board.h"

static const Edge getBoxEdgesTable[NUM_BOXES][4] = {
    {0,8,9,17},    {1,9,10,18},   {2,10,11,19},
    {3,11,12,20},  {4,12,13,21},  {5,13,14,22},
    {6,14,15,23},  {7,15,16,24},  {17,25,26,34},
    {18,26,27,35}, {19,27,28,36}, {20,28,29,37},
    {21,29,30,38}, {22,30,31,39}, {23,31,32,40},
    {24,32,33,41}, {35,42,43,48}, {36,43,44,49},
    {39,45,46,50}, {40,46,47,51}, {48,52,53,58},
    {49,53,54,59}, {50,55,56,60}, {51,56,57,61},
    {58,62,63,68}, {59,63,64,69}, {60,65,66,70},
    {61,66,67,71}
};

static const Box getEdgeBoxesTable[NUM_EDGES][2] = {
    { 0,NO_BOX}, { 1,NO_BOX}, { 2,NO_BOX}, { 3,NO_BOX}, { 4,NO_BOX}, { 5,NO_BOX}, { 6,NO_BOX}, { 7,NO_BOX}, 
    { 0,NO_BOX}, { 0, 1}, { 1, 2}, { 2, 3}, { 3, 4}, { 4, 5}, { 5, 6}, { 6, 7}, { 7,NO_BOX},
    { 0, 8}, { 1, 9}, { 2,10}, { 3,11}, { 4,12}, { 5,13}, { 6,14}, { 7,15},
    { 8,NO_BOX}, { 8, 9}, { 9,10}, {10,11}, {11,12}, {12,13}, {13,14}, {14,15}, {15,NO_BOX},
    { 8,NO_BOX}, { 9,16}, {10,17}, {11,NO_BOX}, {12,NO_BOX}, {13,18}, {14,19}, {15,NO_BOX},
    {16,NO_BOX}, {16,17}, {17,NO_BOX}, {18,NO_BOX}, {18,19}, {19,NO_BOX},
    {16,20}, {17,21}, {18,22}, {19,23},
    {20,NO_BOX}, {20,21}, {21,NO_BOX}, {22,NO_BOX}, {22,23}, {23,NO_BOX},
    {20,24}, {21,25}, {22,26}, {23,27},
    {24,NO_BOX}, {24,25}, {25,NO_BOX}, {26,NO_BOX}, {26,27}, {27,NO_BOX},
    {24,NO_BOX}, {25,NO_BOX}, {26,NO_BOX}, {27,NO_BOX} 
};

void initUnscoredState(UnscoredState * state) {
    for (Edge e=0; e<NUM_EDGES; e++) {
        setEdgeFree(state, e);
    }
}

void stringToUnscoredState(UnscoredState * state, const char * edge_data) {
    for (short i=0; i<NUM_EDGES; i++) {
        if (edge_data[i] == '0') {
            setEdgeFree(state, i);
        }
        else {
            setEdgeTaken(state, i);
        }
    }
}

void setEdgeTaken(UnscoredState * state, Edge e) {
    state->edges[e] = TAKEN;
}

void setEdgeFree(UnscoredState * state, Edge e) {
    state->edges[e] = FREE;
}

short getFreeEdges(UnscoredState * state, Edge * freeEdgesBuffer) {
    short numFreeEdges = 0;

    for(Edge e=0; e<NUM_EDGES; e++) {
        if (!isEdgeTaken(state, e))
            freeEdgesBuffer[numFreeEdges++] = e;
    }

    return numFreeEdges;
}

short getNumFreeEdges(UnscoredState * state) {
    short freeEdges=0;

    for(short i=0; i<NUM_EDGES; i++) {
        if (state->edges[i] == FREE)
            freeEdges++;
    }

    return freeEdges;
}

short getNumBoxesLeft(UnscoredState * state) {
    short numBoxesLeft = NUM_BOXES;

    for(short i=0; i<NUM_BOXES; i++) {
        if(isBoxTaken(state, i))
            numBoxesLeft--;
    }

    return numBoxesLeft;
}

const Edge * getBoxEdges(Box b) {
    return getBoxEdgesTable[b];
}

const Box * getEdgeBoxes(Edge e) {
    return getEdgeBoxesTable[e];
}

short getBoxNumTakenEdges(UnscoredState * state, Box b) {
    const Edge * boxEdges = getBoxEdges(b);
    short numTakenEdges = 0;

    for(short i=0; i<4; i++) {
        if (isEdgeTaken(state, boxEdges[i]))
            numTakenEdges++;
    }

    return numTakenEdges;
}

bool isEdgeTaken(UnscoredState * state, Edge e) {
    return state->edges[e] == TAKEN;
}

bool isBoxTaken(UnscoredState * state, Box b) {
    const Edge * boxEdges = getBoxEdges(b);

    for (short i=0; i<4; i++) {
        if (!isEdgeTaken(state, boxEdges[i]))
            return false;
    }
    return true;
}

short howManyBoxesDoesMoveComplete(UnscoredState * state, Edge edge) {
    short numCompleted = 0;
    const Box * boxes = getEdgeBoxes(edge);

    for(short i=0; i<2; i++) {
        Box b = boxes[i];
        if(b != NO_BOX) {
            if(getBoxNumTakenEdges(state, b) == 3) // the edge must complete the box
                numCompleted++;
        }
    }

    return numCompleted;
}

void printUnscoredState(UnscoredState * state) {
    wchar_t vertical = L'|';
    wchar_t horizontal = L'―';
    wchar_t dot = L'•';
    wchar_t cross = L'✕';
    wchar_t empty = L' ';

    printf("%lc", dot);
    short ei = 0; // edge index
    short si = 0; // square index

    // First 2 rows of squares:
    for(; ei<NUM_EDGES; ei++) {
        if ((ei >=0  && ei <=7)  ||
            (ei >=17 && ei <=24) ||
            (ei >=34 && ei <=41) ||
            (ei >=48 && ei <=51) ||
            (ei >=58 && ei <=61) ||
            (ei >=68 && ei <=71)) {
            // Rows with horizontal edges:

            switch(state->edges[ei]) {
                case FREE:
                    printf("%lc", empty);
                    break;
                case TAKEN:
                    printf("%lc", horizontal);
                    break;
            }
            printf("%lc", dot);

            if (ei==7 || ei==24) {
                printf("\n");
            }
            else if (ei==41) {
                printf("\n  ");
            }
            else if (ei==49 || ei==59 || ei==69) {
                printf("   %lc", dot);
            }
            else if (ei==51 || ei==61) {
                printf("  \n  ");
            }
        }
        else {
            // Rows with vertical edges
            // e.g. ei=8
            
            switch(state->edges[ei]) {
                case FREE:
                    printf("%lc", empty);
                    break;
                case TAKEN:
                    printf("%lc", vertical);
                    break;
            }

            if (ei==16 || ei==33) {
                printf("\n%lc", dot);
            }
            else if (ei==44 || ei==54 || ei==64) {
                printf("   ");
            }
            else if (ei==47 || ei==57 || ei==67) {
                printf("  \n  %lc", dot);
            }
            else {
                if (isBoxTaken(state, si)) {
                    printf("%lc", cross);
                }
                else {
                    printf("%lc", empty);
                }
                si++;
            }
        }
    }
    printf("\n");
}

void runGameBoardTests() {
    printf("RUNNING GAME_BOARD TESTS\n");
    
    puts("Testing UnscoredState...");
    UnscoredState state;
    initUnscoredState(&state);
    stringToUnscoredState(&state, "1100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    assert(state.edges[0] == TAKEN);
    assert(state.edges[1] == TAKEN);
    for(short i=2; i<NUM_EDGES; i++) {
        assert(state.edges[i] == FREE);
    }

    printf("Testing getFreeEdges...\n");
    stringToUnscoredState(&state, "101010101111111111111111111111111111111111111111111111111111111111111111");
    Edge freeEdges[NUM_EDGES];
    short numFreeEdges = getFreeEdges(&state, freeEdges);
    assert(numFreeEdges == 4);
    assert(freeEdges[0] == 1);
    assert(freeEdges[1] == 3);
    assert(freeEdges[2] == 5);
    assert(freeEdges[3] == 7);

    printf("Testing getBoxEdges...\n");
    const Edge * edges = getBoxEdges(0);
    assert(edges[0] == 0);
    assert(edges[1] == 8);
    assert(edges[2] == 9);
    assert(edges[3] == 17);

    edges = getBoxEdges(5);
    assert(edges[0] == 5);
    assert(edges[1] == 13);
    assert(edges[2] == 14);
    assert(edges[3] == 22);

    printf("Testing getEdgeBoxes...\n");
    const Box * boxes = getEdgeBoxes(18);
    assert(boxes[0] == 1);
    assert(boxes[1] == 9);

    boxes = getEdgeBoxes(46);
    assert(boxes[0] == 18);
    assert(boxes[1] == 19);

    boxes = getEdgeBoxes(16);
    assert(boxes[0] == 7);
    assert(boxes[1] == NO_BOX);

    boxes = getEdgeBoxes(68);
    assert(boxes[0] == 24);
    assert(boxes[1] == NO_BOX);

    printf("Testing getBoxNumTakenEdges...\n");
    stringToUnscoredState(&state, "102000001110000001010000000000000000000000000000010000000001000000000000");
    short numTakenEdges = getBoxNumTakenEdges(&state, 0);
    assert(numTakenEdges == 4);

    numTakenEdges = getBoxNumTakenEdges(&state, 2);
    assert(numTakenEdges == 3);

    numTakenEdges = getBoxNumTakenEdges(&state, 21);
    assert(numTakenEdges == 2);


    printf("Printing example unscored state...\n");
    initUnscoredState(&state);
    state.edges[2] = TAKEN;
    state.edges[10] = TAKEN;
    state.edges[11] = TAKEN;
    state.edges[19] = TAKEN;

    state.edges[23] = TAKEN;
    state.edges[31] = TAKEN;
    state.edges[32] = TAKEN;
    state.edges[40] = TAKEN;
    state.edges[14] = TAKEN;
    printUnscoredState(&state);

    printf("Testing isBoxTaken...\n");
    for(Box i=0; i<NUM_BOXES; i++) {
        if (i==2 || i==14)
            assert(isBoxTaken(&state, i) == true);
        else
            assert(isBoxTaken(&state, i) == false);
    }

    puts("Testing howManyBoxesDoesMoveComplete...");
    stringToUnscoredState(&state, "000000000000000000000000000000000000000000000000000000000000000000000000");
    assert(howManyBoxesDoesMoveComplete(&state, 0) == 0);

    stringToUnscoredState(&state, "100000001000000001000000000000000000000000000000000000000000000000000000");
    assert(howManyBoxesDoesMoveComplete(&state, 9) == 1);

    stringToUnscoredState(&state, "110000001010000001100000000000000000000000000000000000000000000000000000");
    assert(howManyBoxesDoesMoveComplete(&state, 9) == 2);

    puts("Testing getNumBoxesLeft...");
    stringToUnscoredState(&state, "000000000000000000000000000000000000000000000000000000000000000000000000");
    assert(getNumBoxesLeft(&state) == NUM_BOXES);

    stringToUnscoredState(&state, "111111111111111111111111111111111111111111111111111111111111111111111111");
    assert(getNumBoxesLeft(&state) == 0);

    stringToUnscoredState(&state, "100000001100000001000000000000000000000000000000000000000000000000000000");
    assert(getNumBoxesLeft(&state) == NUM_BOXES - 1);


    puts("GAME_BOARD TESTS COMPLETED\n");
}
