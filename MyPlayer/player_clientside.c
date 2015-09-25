#include <assert.h>
#include <locale.h>
#include <wchar.h>
#include <unistd.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <regex.h>
#include <jansson.h>
#include "game_board.h"
#include "player_clientside.h"
#include "player_strategy.h"
#include "mcts.h"
#include "alphabeta.h"
#include "util.h"

#define ACKNOWLEDGED "ACK"
#define BUFSIZE 1025

static const char * PLAYER_NAME = "DeepBox";
int LOG_LEVEL = LOG_LEVEL_LOG;

bool parseServerMsg(
        const char * server_msg,
        char * cmd_buf,
        char * data_buf) {
    const char * msg_regex_pattern = "\\('(\\w+)', '?(\\w+)'?\\)";
    regex_t regex;

    log_debug("msg_regex_pattern: %s\n", msg_regex_pattern);
    log_debug("server_msg: %s\n", server_msg);
    
    // Compile regex
    int reti = regcomp(&regex, msg_regex_pattern, REG_EXTENDED);
    if (reti) {
        log_warn("parseServerMsg: Could not compile regex.");
        exit(1);
    }

    regmatch_t matches[3];
    reti = regexec(&regex, server_msg, 3, matches, 0);

    if (!reti) {
        // Group 1 (command)
        int cmd_so = (int)matches[1].rm_so;
        int cmd_eo = (int)matches[1].rm_eo;
        int cmd_len = cmd_eo - cmd_so;
        strncpy(cmd_buf, server_msg + cmd_so, cmd_len);
        cmd_buf[cmd_len] = '\0';

        // Group 2 (data)
        int data_so = (int)matches[2].rm_so;
        int data_eo = (int)matches[2].rm_eo;
        int data_len = data_eo - data_so;
        strncpy(data_buf, server_msg + data_so, data_len);
        data_buf[data_len] = '\0';
        return true;
    }
    else if (reti == REG_NOMATCH) {
        log_warn("parseServerMsg: No match!\n");
        return false;
    }
    else {
        char err_buf[200];
        regerror(reti, &regex, err_buf, sizeof(err_buf));
        log_warn("Regex match failed: %s\n", err_buf);
        exit(1);
    }
}

void runPlayerClientsideTests() {
    log_log("RUNNING PLAYER_CLIENTSIDE TESTS\n");
    char msg_buf[BUFSIZE];
    char cmd_buf[200];
    char data_buf[1025];

    sprintf(msg_buf, "('newGame', None)");
    log_log("Testing parsing of: %s\n", msg_buf);
    bool success = parseServerMsg(msg_buf, cmd_buf, data_buf);
    assert(success == true);
    assert(strcmp(cmd_buf, "newGame") == 0);
    assert(strcmp(data_buf, "None") == 0);

    sprintf(msg_buf, "('getName', '1')");
    log_log("Testing parsing of: %s\n", msg_buf);
    success = parseServerMsg(msg_buf, cmd_buf, data_buf);
    assert(success == true);
    assert(strcmp(cmd_buf, "getName") == 0);
    assert(strcmp(data_buf, "1") == 0);
    log_log("PLAYER_CLIENTSIDE TESTS COMPLETED\n\n");
}

int main(int argc, char ** argv) {
    setlocale(LC_ALL, ""); // for unicode printing
    LOG_LEVEL = LOG_LEVEL_LOG; // defined in util.h
    char * server_address = "localhost";
    char * server_port = "12345";
    bool run_tests = false;
    char * strategyName = "random_move";
    Strategy strategy = RANDOM_MOVE;
    int turnTimeMillis = 1000;
    bool runningExamplePosition = false; // if -x flag is given then a position is expected on standard input.

    int option;
    while((option = getopt(argc, argv, "l:a:p:ts:i:x")) != -1) {
        switch(option) {
            case 'l':
                if(strcmp("debug", optarg) == 0)
                    LOG_LEVEL = LOG_LEVEL_DEBUG;
                else if(strcmp("log", optarg) == 0)
                    LOG_LEVEL = LOG_LEVEL_LOG;
                else if(strcmp("warn", optarg) == 0)
                    LOG_LEVEL = LOG_LEVEL_WARN;
                else if(strcmp("error", optarg) == 0)
                    LOG_LEVEL = LOG_LEVEL_ERROR;
                else
                    fprintf(stderr, "Unrecognised log level: %s. Available options are {debug, log, warn, error}.\n", optarg);
                break;
            case 'a':
                server_address = optarg;
                break;
            case 'p':
                server_port = optarg;
                break;
            case 't':
                run_tests = true;
                break;
            case 's':
                strategyName = optarg;

                if(strcmp("random_move", strategyName) == 0)
                    strategy = RANDOM_MOVE;
                else if(strcmp("first_box_completing_move", strategyName) == 0)
                    strategy = FIRST_BOX_COMPLETING_MOVE;
                else if(strcmp("monte_carlo", strategyName) == 0)
                    strategy = MONTE_CARLO;
                else if(strcmp("alpha_beta", strategyName) == 0)
                    strategy = ALPHA_BETA;
                else
                    fprintf(stderr, "Unrecognised strategy name: %s. Available options are {random_move, first_box_completing_move, monte_carlo, alpha_beta}.\n", strategyName);
                break;
            case 'i':
                turnTimeMillis = atoi(optarg);
                break;
            case 'x':
                runningExamplePosition = true;
                break;
        }
    }

    log_log("Server address: %s\n", server_address);
    log_log("Server port: %s\n", server_port);
    log_log("Using strategy: %s\n", strategyName);
    log_log("Time per turn (millis): %d\n", turnTimeMillis);

    if (run_tests) {
        runGameBoardTests();
        runPlayerClientsideTests();
        runPlayerStrategyTests();
        runUtilTests();
        //runMCTSTests();
        runAlphaBetaTests();
        
        exit(0);
    }
    else if (runningExamplePosition) {
        char stateBuf[NUM_EDGES];
        if(fgets(stateBuf, sizeof(char) * (NUM_EDGES+1), stdin) == NULL) {
            fprintf(stderr, "Error opening example file. Exiting.\n");
            exit(1);
        }

        UnscoredState state;
        stringToUnscoredState(&state, stateBuf);
        Edge move = chooseMove(state, strategy, turnTimeMillis);

        log_log("CHOSE MOVE: %d\n", move);

        exit(0);
    }
    // else connect to the server and proceed into a game

    // Create socket
    int sockfd, portno, n;

    struct sockaddr_in serv_addr;
    struct hostent *server;

    log_debug("Creating socket...\n");
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        log_warn("[WARN] error opening socket.\n");
        exit(1);
    }

    log_debug("Getting host...\n");
    server = gethostbyname(server_address);
    if (server == NULL) {
        log_warn("[WARN], no such host: %s\n", server_address);
        exit(1);
    }

    portno = atoi(server_port);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr_list[0],
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);
    serv_addr.sin_port = htons(portno);

    log_debug("Connecting...\n");
    if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
        log_error("[ERROR] connecting to server.\n");
        exit(1);
    }

    log_log("Connected to server successfully.\n");

    char recv_buf[BUFSIZE]; // For msg from server
    char send_buf[BUFSIZE]; // For msg to server

    char cmd_buf[100]; // For parsing server msg
    char data_buf[BUFSIZE]; 

    // Usual game loop is:
    // 1.  SERVER: connected  ME: ack
    // 2.  SERVER: getName    ME: bob
    // 3.  SERVER: chooseMove ME: 13 # until game completed
    // 4a. SERVER: newGame    ME: ack # then back to 3 
    // 4b. SERVER: gameOver   ME: ack # then exit
    bool game_over = false;
    while(!game_over) {
        bzero(recv_buf, sizeof(char)*BUFSIZE);

        log_debug("Waiting for message from server...\n");
        n = read(sockfd, recv_buf, BUFSIZE);
        if (n < 0) {
            log_error("[ERROR] reading from socket.\n");
            exit(1);
        }
        log_debug("Server says: %s\n", recv_buf);

        // Check what the server wants us to do and respond accordingly:
        bzero(send_buf, sizeof(char)*BUFSIZE);
        if (strcmp(recv_buf, "connected") == 0) {
            log_log("Recognized message 'connected'. Sending ack.\n");
            sprintf(send_buf, ACKNOWLEDGED);
        }
        else if (parseServerMsg(recv_buf, cmd_buf, data_buf)) {
            log_log("Server command: %s\nServer data: %s\n", cmd_buf, data_buf);
            if (strcmp(cmd_buf, "getName") == 0) {
                log_log("Recognized message 'getName'. My name is %s %s %d\n", PLAYER_NAME, strategyName, turnTimeMillis);
                sprintf(send_buf, "%s %s %d", PLAYER_NAME, strategyName, turnTimeMillis);
            }
            else if (strcmp(cmd_buf, "newGame") == 0) {
                log_log("Recognized message 'newGame'. Sending ack.\n");
                sprintf(send_buf, ACKNOWLEDGED);
                // TODO: Init newGame object
            }
            else if (strcmp(cmd_buf, "chooseMove") == 0) {
                log_log("Recognized message 'chooseMove'.\n");
                UnscoredState state;
                stringToUnscoredState(&state, data_buf);
                Edge move = chooseMove(state, strategy, turnTimeMillis);
                sprintf(send_buf, "%d", move);
            }
            else if (strcmp(cmd_buf, "gameOver") == 0) {
                log_log("GAME IS OVER. Result: %s\n", data_buf);
                sprintf(send_buf, ACKNOWLEDGED);
                game_over = true;
            }
            else {
                log_error("[ERROR] Parsed message from server successfully bit did not recognize cmd: %s\n", cmd_buf);
                exit(1);
            }
        }
        else {
            log_error("[ERROR] Server sent an unrecognized command. Exiting.\n");
            exit(1);
        }

        n = write(sockfd, send_buf, strlen(send_buf));
        if (n < 0) {
            log_error("[ERROR] Writing to socket.\n");
            exit(1);
        }
    }
}
