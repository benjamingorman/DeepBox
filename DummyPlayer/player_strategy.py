'''
This module handles the game strategy (only) of a player.
'''

import random
import const
from game_board import GameBoardADT

# Enter your own player name
playerName = "Dummy Player"

# The game board
board = GameBoardADT()

# This tells you which player you are, either const.PLAYER1 or const.PLAYER2
# It is set when the connection to the game server is made
# It may be useful if you want to know who occupies which square
myPlayerNumber = None

def chooseMove(state):
    """
    Should Decide what move to make based on current state of opponent's board and return it.
    Currently the strategy is completely random. It just chooses randomly from the list of unplayed edges.
    """
    global board
    global myPlayerNumber
    # Update your board to the current state:
    board.setGameState(state)
    # YOUR IMPROVED STRATEGY SHOULD GO HERE:

    # The random player just asks for all unplayed edges and randomly chooses one
    unplayed = board.getUnplayedEdges()
    move = random.choice(unplayed)
    print move
    return move

def newGame():
    """
    This method is called when a new game is starting (new game with same player). This gives you the
    ability to update your strategy.
    Currently does nothing.
    """
    pass

