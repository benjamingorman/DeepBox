#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <wchar.h>
#include <stdbool.h>

#include "game_board.h"
#include "util.h"

// PiSquare board:
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

// corner pairs: {0,8} {7,16} {25,34} {33,41} {62,68} {64,69} {65,70} {67,71} 
// sorted: 0,8,7,16, 25 ,33,34,41,  62,  64,65,68, 67, 69,70,71
Edge getCorrespondingCornerEdge(Edge e) {
    // Because the set of corner edges is so small, the most efficient, alebit ugly thing to do
    // is a hard-coded binary search.
    // Returns NO_EDGE if e is not a corner edge.
    if (e == 62)
        return 68;
    else if (e < 62) {
        if (e == 25)
            return 34;
        else if (e < 25) {
            switch(e) {
                case 0: return 8; break;
                case 8: return 0; break;
                case 7: return 16; break;
                case 16: return 7; break;
            }
        }
        else { // e > 25
            switch (e) {
                case 33: return 41; break;
                case 34: return 25; break;
                case 41: return 33; break;
            }
        }
    }
    else { // e > 62
        if (e == 67)
            return 71;
        else if (e < 67) {
            switch (e) {
                case 64: return 69; break;
                case 65: return 70; break;
                case 68: return 62; break;
            }
        }
        else {
            switch (e) {
                case 69: return 64; break;
                case 70: return 65; break;
                case 71: return 67; break;
            }
        }
    }

    return NO_EDGE;
}

/*
// 3x3 board:
static const Edge getBoxEdgesTable[NUM_BOXES][4] = {
    {0,3,4,7}, {1,4,5,8}, {2,5,6,9},
    {7,10,11,14}, {8,11,12,15}, {9,12,13,16},
    {14,17,18,21}, {15,18,19,22}, {16,19,20,23}
};

static const Box getEdgeBoxesTable[NUM_EDGES][2] = {
    {0,NO_BOX}, {1,NO_BOX}, {2,NO_BOX},
    {0,NO_BOX}, {0,1}, {1,2}, {2,NO_BOX},
    {0,3}, {1,4}, {2,5},
    {3,NO_BOX}, {3,4}, {4,5}, {5,NO_BOX},
    {3,6}, {4,7}, {5,8},
    {6,NO_BOX}, {6,7}, {7,8}, {8,NO_BOX},
    {6,NO_BOX}, {7, NO_BOX}, {8,NO_BOX}
};

// 3x3 grid corners: 0,3 2,6 17,21 20,23
// sorted: 0,2,3,6,17,20,21,23

Edge getCorrespondingCornerEdge(Edge e) {
    if (e == 17)
        return 21;
    else if (e < 17) {
        if (e == 0)
            return 3;
        else if (e == 2)
            return 6;
        else if (e == 3)
            return 0;
        else if (e == 6)
            return 2;
    }
    else { // e > 17
        if (e == 20)
            return 23;
        else if (e == 21)
            return 17;
        else if (e == 23)
            return 20;
    }

    return NO_EDGE;
}
*/

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

short getFreeEdges(const UnscoredState * state, Edge * freeEdgesBuffer) {
    short numFreeEdges = 0;

    for(Edge e=0; e<NUM_EDGES; e++) {
        if (!isEdgeTaken(state, e))
            freeEdgesBuffer[numFreeEdges++] = e;
    }

    return numFreeEdges;
}

short getNumFreeEdges(const UnscoredState * state) {
    short freeEdges=0;

    for(short i=0; i<NUM_EDGES; i++) {
        if (state->edges[i] == FREE)
            freeEdges++;
    }

    return freeEdges;
}

short getRemainingBoxes(const UnscoredState * state, Box * boxBuffer) {
    short numBoxesLeft = 0;

    for(Box b=0; b < NUM_BOXES; b++) {
        if (!isBoxTaken(state, b))
            boxBuffer[numBoxesLeft++] = b;
    }

    return numBoxesLeft;
}

short getNumBoxesLeft(const UnscoredState * state) {
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

short getBoxNumTakenEdges(const UnscoredState * state, Box b) {
    const Edge * boxEdges = getBoxEdges(b);
    short numTakenEdges = 0;

    for(short i=0; i<4; i++) {
        if (isEdgeTaken(state, boxEdges[i]))
            numTakenEdges++;
    }

    return numTakenEdges;
}

bool isEdgeTaken(const UnscoredState * state, Edge e) {
    return state->edges[e] == TAKEN;
}

bool isBoxTaken(const UnscoredState * state, Box b) {
    const Edge * boxEdges = getBoxEdges(b);

    for (short i=0; i<4; i++) {
        if (!isEdgeTaken(state, boxEdges[i]))
            return false;
    }
    return true;
}

short howManyBoxesDoesMoveComplete(const UnscoredState * state, Edge edge) {
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

bool isBoxCompletingMove(const UnscoredState * state, Edge move) {
    return howManyBoxesDoesMoveComplete(state, move) > 0;
}

void printUnscoredState(const UnscoredState * state) {
    wchar_t vertical = L'|';
    wchar_t horizontal = L'―';
    wchar_t dot = L'•';
    wchar_t cross = L'✕'; wchar_t empty = L' ';

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
    log_log("RUNNING GAME_BOARD TESTS\n");
    
    log_log("Testing UnscoredState...\n");
    UnscoredState state;
    initUnscoredState(&state);
    stringToUnscoredState(&state, "1100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    assert(state.edges[0] == TAKEN);
    assert(state.edges[1] == TAKEN);
    for(short i=2; i<NUM_EDGES; i++) {
        assert(state.edges[i] == FREE);
    }

    log_log("Testing getFreeEdges...\n");
    stringToUnscoredState(&state, "101010101111111111111111111111111111111111111111111111111111111111111111");
    Edge freeEdges[NUM_EDGES];
    short numFreeEdges = getFreeEdges(&state, freeEdges);
    assert(numFreeEdges == 4);
    assert(freeEdges[0] == 1);
    assert(freeEdges[1] == 3);
    assert(freeEdges[2] == 5);
    assert(freeEdges[3] == 7);

    log_log("Testing getBoxEdges...\n");
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

    log_log("Testing getEdgeBoxes...\n");
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

    log_log("Testing getBoxNumTakenEdges...\n");
    stringToUnscoredState(&state, "102000001110000001010000000000000000000000000000010000000001000000000000");
    short numTakenEdges = getBoxNumTakenEdges(&state, 0);
    assert(numTakenEdges == 4);

    numTakenEdges = getBoxNumTakenEdges(&state, 2);
    assert(numTakenEdges == 3);

    numTakenEdges = getBoxNumTakenEdges(&state, 21);
    assert(numTakenEdges == 2);


    log_log("Printing example unscored state...\n");
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

    log_log("Testing isBoxTaken...\n");
    for(Box i=0; i<NUM_BOXES; i++) {
        if (i==2 || i==14)
            assert(isBoxTaken(&state, i) == true);
        else
            assert(isBoxTaken(&state, i) == false);
    }

    log_log("Testing howManyBoxesDoesMoveComplete...\n");
    stringToUnscoredState(&state, "000000000000000000000000000000000000000000000000000000000000000000000000");
    assert(howManyBoxesDoesMoveComplete(&state, 0) == 0);

    stringToUnscoredState(&state, "100000001000000001000000000000000000000000000000000000000000000000000000");
    assert(howManyBoxesDoesMoveComplete(&state, 9) == 1);

    stringToUnscoredState(&state, "110000001010000001100000000000000000000000000000000000000000000000000000");
    assert(howManyBoxesDoesMoveComplete(&state, 9) == 2);

    log_log("Testing getNumBoxesLeft...\n");
    stringToUnscoredState(&state, "000000000000000000000000000000000000000000000000000000000000000000000000");
    assert(getNumBoxesLeft(&state) == NUM_BOXES);

    stringToUnscoredState(&state, "111111111111111111111111111111111111111111111111111111111111111111111111");
    assert(getNumBoxesLeft(&state) == 0);

    stringToUnscoredState(&state, "100000001100000001000000000000000000000000000000000000000000000000000000");
    assert(getNumBoxesLeft(&state) == NUM_BOXES - 1);


    log_log("GAME_BOARD TESTS COMPLETED\n\n");
}
