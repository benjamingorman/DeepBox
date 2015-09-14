#ifndef PLAYER_CLIENTSIDE_H
#define PLAYER_CLIENTSIDE_H

#include <stdbool.h>

bool parse_server_msg(const char *, char *, char *);
void run_player_clientside_tests();

#endif
