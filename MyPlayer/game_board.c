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
    { 0,-1}, { 1,-1}, { 2,-1}, { 3,-1}, { 4,-1}, { 5,-1}, { 6,-1}, { 7,-1}, 
    { 0,-1}, { 0, 1}, { 1, 2}, { 2, 3}, { 3, 4}, { 4, 5}, { 5, 6}, { 6, 7}, { 7,-1},
    { 0, 8}, { 1, 9}, { 2,10}, { 3,11}, { 4,12}, { 5,13}, { 6,14}, { 7,15},
    { 8,-1}, { 8, 9}, { 9,10}, {10,11}, {11,12}, {12,13}, {13,14}, {14,15}, {15,-1},
    { 8,-1}, { 9,16}, {10,17}, {11,-1}, {12,-1}, {13,18}, {14,19}, {15,-1},
    {16,-1}, {16,17}, {17,-1}, {18,-1}, {18,19}, {19,-1},
    {16,20}, {17,21}, {18,22}, {19,23},
    {20,-1}, {20,21}, {21,-1}, {22,-1}, {22,23}, {23,-1},
    {20,24}, {21,25}, {22,26}, {23,27},
    {24,-1}, {24,25}, {25,-1}, {26,-1}, {26,27}, {27,-1},
    {24,-1}, {25,-1}, {26,-1}, {27,-1} 
};

void initUnscoredState(UnscoredState * state) {
    for (short i=0; i<NUM_EDGES; i++) {
        state->edges[i] = FREE;
    }
}

void stringToUnscoredState(UnscoredState * state, const char * edge_data) {
    for (short i=0; i<NUM_EDGES; i++) {
        if (edge_data[i] == '0') {
            state->edges[i] = FREE;
        }
        else {
            state->edges[i] = TAKEN;
        }
    }
}

const Edge * getBoxEdges(Box b) {
    return getBoxEdgesTable[b];
}

const Box * getEdgeBoxes(Edge e) {
    return getEdgeBoxesTable[e];
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
    assert(boxes[1] == -1); // Edges with only 1 box have -1 as the index of the second box

    boxes = getEdgeBoxes(68);
    assert(boxes[0] == 24);
    assert(boxes[1] == -1);
    
    printf("Printing test unscored state...\n");
    UnscoredState state;
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

    puts("Testing UnscoredState...");
    initUnscoredState(&state);
    stringToUnscoredState(&state, "1100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    assert(state.edges[0] == TAKEN);
    assert(state.edges[1] == TAKEN);
    for(short i=2; i<NUM_EDGES; i++) {
        assert(state.edges[i] == FREE);
    }

    puts("GAME_BOARD TESTS COMPLETED\n");
}
