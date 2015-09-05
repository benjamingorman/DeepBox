#ifndef GAME_BOARD_H
#define GAME_BOARD_H

#include <stdbool.h>

#define P1_COLOUR "\x1b[31m"
#define P2_COLOUR "\x1b[34m"
#define COLOUR_RESET "\x1b[0m"

#define NUM_SQUARES 28
#define NUM_EDGES 72

typedef enum {
    AVAILABLE,
    TAKEN_P1,
    TAKEN_P2
} CaptureState;

typedef struct {
    CaptureState edges[NUM_EDGES];
    CaptureState squares[NUM_SQUARES];
} GameBoard; 

GameBoard * newGameBoard();
void printGameBoard(GameBoard *, bool);
const int * square2edges(int);
void run_game_board_tests();

#endif
