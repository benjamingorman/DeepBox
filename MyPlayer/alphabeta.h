#ifndef ALPHABETA_H
#define ALPHABETA_H

typedef struct ABNode {
    double alpha;
    double beta;

    Edge move;
    short numBoxesTakenByMove;
    Edge potentialMoves[NUM_EDGES];
    short numPotentialMoves;

    struct ABNode * parent;
    struct ABNode * child;
    struct ABNode * sibling;
    short numChildren;

    bool isMaximizer;
    double value;
} ABNode;

Edge getABMove(const UnscoredState * state, short maxDepth, bool saveJSON);
void runAlphaBetaTests();

#endif
