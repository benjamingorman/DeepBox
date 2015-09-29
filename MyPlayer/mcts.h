#ifndef MCTS_H
#define MCTS_H

#include "graphs.h"

typedef struct MCTSNode { 
    struct MCTSNode * parent;
    struct MCTSNode * child;
    struct MCTSNode * sibling;

    Edge move;
    short numBoxesTakenByMove;
    UnscoredState state; // the post-move state
    SCGraph graph;
    PlayerNum playerJustMoved;
    PlayerNum nextPlayerToMove;

    double totalScore; // from perspective of playerJustMoved
    int visits;

    Edge potentialMoves[NUM_EDGES];
    short nextUntriedMoveIndex;
    short numPotentialMoves;
    short numChildren;
} MCTSNode;

Edge getMCTSMove(UnscoredState * rootState, int runTimeMillis, bool saveTreeJSON);
void runMCTSTests();

#endif
