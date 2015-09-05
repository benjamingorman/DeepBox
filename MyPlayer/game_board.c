#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <wchar.h>
#include <stdbool.h>
#include "game_board.h"

GameBoard * newGameBoard() {
    GameBoard * board = (GameBoard *)malloc(sizeof(GameBoard));

    for (int i=0; i<NUM_EDGES; i++) {
        board->edges[i] = AVAILABLE;
    }

    for (int i=0; i<NUM_SQUARES; i++) {
        board->squares[i] = AVAILABLE;
    }

    return board;
}

void printGameBoard(GameBoard * gameBoard, bool printColours) {
    wchar_t vertical = L'|';
    wchar_t horizontal = L'―';
    wchar_t dot = L'•';
    wchar_t cross = L'✕';
    wchar_t empty = L' ';

    printf("%lc", dot);
    int ei = 0; // edge index
    int si = 0; // square index

    // First 2 rows of squares:
    for(; ei<NUM_EDGES; ei++) {
        if ((ei >=0  && ei <=7)  ||
            (ei >=17 && ei <=24) ||
            (ei >=34 && ei <=41) ||
            (ei >=48 && ei <=51) ||
            (ei >=58 && ei <=61) ||
            (ei >=68 && ei <=71)) {
            // Rows with horizontal edges:

            switch(gameBoard->edges[ei]) {
                case AVAILABLE:
                    printf("%lc", empty);
                    break;
                case TAKEN_P1:
                    if (printColours) { 
                        printf(P1_COLOUR "%lc" COLOUR_RESET, horizontal);
                    }
                    else {
                        printf("%lc", horizontal);
                    }
                    break;
                case TAKEN_P2:
                    if (printColours) {
                        printf(P2_COLOUR "%lc" COLOUR_RESET, horizontal);
                    }
                    else {
                        printf("%lc", horizontal);
                    }
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
            
            switch(gameBoard->edges[ei]) {
                case AVAILABLE:
                    printf("%lc", empty);
                    break;
                case TAKEN_P1:
                    if (printColours) {
                        printf(P1_COLOUR "%lc" COLOUR_RESET, vertical);
                    }
                    else {
                        printf("%lc", vertical);
                    }
                    break;
                case TAKEN_P2:
                    if (printColours) {
                        printf(P2_COLOUR "%lc" COLOUR_RESET, vertical);
                    }
                    else {
                        printf("%lc", vertical);
                    }
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
                switch(gameBoard->squares[si]) {
                    case AVAILABLE:
                        printf("%lc", empty);
                        break;
                    case TAKEN_P1:
                        if (printColours) {
                            printf(P1_COLOUR "%lc" COLOUR_RESET, cross);
                        }
                        else {
                            printf("%lc", cross);
                        }
                        break;
                    case TAKEN_P2:
                        if (printColours) {
                            printf(P2_COLOUR "%lc" COLOUR_RESET, cross);
                        }
                        else {
                            printf("%lc", cross);
                        }
                        break;
                }
                si++;
            }
        }
    }
    printf("\n");
}

const int square2edgesTable[NUM_SQUARES][4] = {
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


const int * square2edges(int squareIndex) {
    return square2edgesTable[squareIndex];
}

void run_game_board_tests() {
    printf("Running tests...\n");

    // Test square2edges:
    const int * edges = square2edges(0);
    assert(edges[0] == 0);
    assert(edges[1] == 8);
    assert(edges[2] == 9);
    assert(edges[3] == 17);

    edges = square2edges(5);
    assert(edges[0] == 5);
    assert(edges[1] == 13);
    assert(edges[2] == 14);
    assert(edges[3] == 22);
    
    // Test print game board
    printf("Printing empty game board...\n");
    GameBoard * gameBoard = newGameBoard();
    gameBoard->edges[2] = TAKEN_P1;
    gameBoard->edges[10] = TAKEN_P1;
    gameBoard->edges[11] = TAKEN_P1;
    gameBoard->edges[19] = TAKEN_P1;
    gameBoard->squares[2] = TAKEN_P1;

    gameBoard->edges[23] = TAKEN_P2;
    gameBoard->edges[31] = TAKEN_P2;
    gameBoard->edges[32] = TAKEN_P2;
    gameBoard->edges[40] = TAKEN_P2;
    gameBoard->squares[14] = TAKEN_P2;
    printGameBoard(gameBoard, true);
    free(gameBoard);

    printf("Tests completed succesfully.\n");
}
