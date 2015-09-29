#ifndef GAME_BOARD_H
#define GAME_BOARD_H

#include <stdbool.h>

#define P1_COLOUR "\x1b[31m"
#define P2_COLOUR "\x1b[34m"
#define COLOUR_RESET "\x1b[0m"

// PiSquare board:
#define NUM_BOXES 28
#define NUM_EDGES 72

// 3x3 board:
// #define NUM_BOXES 9
// #define NUM_EDGES 24

#define NO_BOX 100
#define NO_EDGE 100
#define NO_PLAYER 100

typedef short Edge;
typedef short Box;
typedef short PlayerNum;

typedef enum {
    FREE,
    TAKEN
} CaptureState;

typedef struct {
    CaptureState edges[NUM_EDGES];
} UnscoredState;

typedef struct {
    CaptureState edges[NUM_EDGES];
    short score_p1;
    short score_p2;
} ScoredState;

typedef struct {
    Edge moves[NUM_EDGES];
} Game;

Edge getCorrespondingCornerEdge(Edge e);
void initUnscoredState(UnscoredState *);
void stringToUnscoredState(UnscoredState *, const char *);
void setEdgeTaken(UnscoredState *, Edge);
void setEdgeFree(UnscoredState *, Edge);
short getNumFreeEdges(const UnscoredState * state);
short getRemainingBoxes(const UnscoredState * state, Box * boxBuffer);
short getNumBoxesLeft(const UnscoredState * state);
short getFreeEdges(const UnscoredState *, Edge *);
const Edge * getBoxEdges(Box);
const Box * getEdgeBoxes(Edge);
short getBoxNumTakenEdges(const UnscoredState *, Box);
bool isEdgeTaken(const UnscoredState *, Edge);
bool isBoxTaken(const UnscoredState *, Box);
short howManyBoxesDoesMoveComplete(const UnscoredState *, Edge);
bool isBoxCompletingMove(const UnscoredState * state, Edge move);
Edge boxPairToEdge(Box b1, Box b2);
void printUnscoredState(const UnscoredState *);
void runGameBoardTests();

#endif
