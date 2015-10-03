DeepBox
=======

This project is an AI to play the pen-and-paper game [Dots & Boxes](https://en.wikipedia.org/wiki/Dots_and_Boxes). Depth limited alpha-beta minimax search is employed during the midgame and endgame in order to find the best move. 

It was the winning entry to the [York University Raspberry Pi Challenge](https://www.cs.york.ac.uk/undergraduate/challenge/) in 2015.

Screenshots
-------

*Human client:*

![Human client](http://imgur.com/A60cXau)

*Server gui:*

![Server gui](http://imgur.com/GiaZQ84)

*Interactive alpha-beta tree visualization:*

![Alpha-beta](http://imgur.com/K7m0gfs)

Usage
-------

To start a game server:
    
    cd MyGameServer
    python pisquare_game_server.py

To play as a human (requires Lua LOVE):

    cd HumanPlayer
    love .

To run the AI (requires [bliss](http://www.tcs.hut.fi/Software/bliss/) and [jansson](http://www.digip.org/jansson/)  dependencies):

    cd MyPlayer
    mkdir build bin
    make
    bin/client -s deepbox
