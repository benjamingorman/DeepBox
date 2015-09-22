CC=gcc
SRCDIR=src
BUILDDIR=build
CFLAGS=-std=c99 -pedantic -Wall -I. -lm -ljansson

DEPS=game_board.h player_clientside.h player_strategy.h mcts.h util.h
OBJECTS=build/game_board.o build/player_clientside.o build/player_strategy.o build/mcts.o build/util.o

build/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

client: $(OBJECTS) 
	$(CC) -o bin/client $(OBJECTS) $(CFLAGS)
	
clean:
	rm -f build/*
	rm -f bin/client
