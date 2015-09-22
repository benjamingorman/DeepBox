#!/bin/bash

games=$1
strategy1=$2
iterations1=$3
strategy2=$4
iterations2=$5

WORK="/home/ben/Projects/PiSquare"
cd $WORK
mkdir logs

log_basepath="$WORK/logs/$(date +%d%m_%H%M%S)"

(
    cd MyGameServer
    python2 pisquare_game_server.py $games > "$log_basepath".server &
    echo "Server: $!"
) 

(
    sleep 1
    cd MyPlayer
    bin/client -s $strategy1 -i $iterations1 > "$log_basepath".player1 &
    echo "Player 1: $!"
)

(
    sleep 1
    cd MyPlayer
    bin/client -s $strategy2 -i $iterations2 &
    echo "Player 2: $!"
)
