#include <stdio.h>
#include "player_strategy.h"
#include "game_board.h"

int chooseMove(UnscoredState state) {
    for(int i=0; i < NUM_EDGES; i++) {
        if (state.edges[i] == FREE) {
            return i;
            printf("chooseMove: Chose move %d\n", i);
        }
    }

    return -1; // Avoid compiler warnings.
}
