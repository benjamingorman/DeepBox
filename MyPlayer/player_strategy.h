#ifndef PLAYER_STRATEGY_H
#define PLAYER_STRATEGY_H

typedef enum {
    RANDOM_MOVE,
    FIRST_BOX_COMPLETING_MOVE
} Strategy;

int randomInRange(unsigned int, unsigned int);
Edge getRandomMove(UnscoredState *);
Edge getFirstBoxCompletingMove(UnscoredState *);
Edge chooseMove(UnscoredState, Strategy);
void runPlayerStrategyTests();

#endif
