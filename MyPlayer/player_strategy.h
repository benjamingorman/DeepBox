#ifndef PLAYER_STRATEGY_H
#define PLAYER_STRATEGY_H

typedef enum {
    RANDOM_MOVE,
    FIRST_BOX_COMPLETING_MOVE,
    MONTE_CARLO,
    GMCTS,
    ALPHA_BETA,
    GRAPHS,
    DEEPBOX
} Strategy;

Edge getRandomMove(UnscoredState *);
Edge getRandomMoveFromList(Edge * edges, short numEdges);
Edge getFirstBoxCompletingMove(UnscoredState *);
Edge chooseMove(UnscoredState, Strategy, int);
void runPlayerStrategyTests();

#endif
