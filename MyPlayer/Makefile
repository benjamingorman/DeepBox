CC=gcc
SRCDIR=src
BUILDDIR=build
# -lstdc++ because libbliss requires c++ standard libraries linked in
CFLAGS=-std=c99 -pedantic -Wall -I. -lm -lbliss -g -lstdc++

DEPS=game_board.h player_clientside.h player_strategy.h mcts.h util.h alphabeta.h graphs.h
OBJECTS=build/game_board.o build/player_clientside.o build/player_strategy.o build/mcts.o build/util.o build/alphabeta.o build/graphs.o

build/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: $(OBJECTS) 
	$(CC) -o bin/client $(OBJECTS) $(CFLAGS)
	
clean:
	rm -f build/*
	rm -f bin/client
