#include <locale.h>
#include <wchar.h>
#include "game_board.h"
#include "player_clientside.h"

int main(int argc, char ** argv) {
    setlocale(LC_ALL, ""); // for unicode printing
    run_game_board_tests();

    return 0;
}
