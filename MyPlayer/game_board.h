#ifndef GAME_BOARD_H
#define GAME_BOARD_H

#include <stdbool.h>

#define P1_COLOUR "\x1b[31m"
#define P2_COLOUR "\x1b[34m"
#define COLOUR_RESET "\x1b[0m"

#define NUM_BOXES 28
#define NUM_EDGES 72

#define NO_BOX -1
#define NO_EDGE -1
#define NO_PLAYER -1

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

void initUnscoredState(UnscoredState *);
void stringToUnscoredState(UnscoredState *, const char *);
void setEdgeTaken(UnscoredState *, Edge);
void setEdgeFree(UnscoredState *, Edge);
short getNumFreeEdges(UnscoredState * state);
short getNumBoxesLeft(UnscoredState * state);
short getFreeEdges(UnscoredState *, Edge *);
const Edge * getBoxEdges(Box);
const Box * getEdgeBoxes(Edge);
short getBoxNumTakenEdges(UnscoredState *, Box);
bool isEdgeTaken(UnscoredState *, Edge);
bool isBoxTaken(UnscoredState *, Box);
short howManyBoxesDoesMoveComplete(UnscoredState *, Edge);
void printUnscoredState(UnscoredState *);
void runGameBoardTests();

#endif
