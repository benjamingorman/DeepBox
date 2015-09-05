import const
from game_board import GameBoardADT
from pisquare_gui import PiSquareGraphics
from player_socket import PlayerSocket
import socket # Import socket module
import time
import random

def drawBoard(gui,board):
    # Draw dots
    for i in range(len(board.VertexPositions)):
        x,y = board.VertexPositions[i]
        gui._fillCircle(x, y, const.DOTRADIUS)
    # Draw board outline
    offset = const.BOARDSCALE/2
    x1,y1 = board.VertexPositions[0]
    x2,y2 = board.VertexPositions[8]
    gui._drawLine(x1-offset,y1+offset,x2+offset,y2+offset,"blue",5)
    x3,y3 = board.VertexPositions[18]
    gui._drawLine(x1-offset,y1+offset,x3-offset,y3-offset,"blue",5)
    x4,y4 = board.VertexPositions[19]
    gui._drawLine(x4-offset,y4-offset,x3-offset,y3-offset,"blue",5)
    x5,y5 = board.VertexPositions[39]
    gui._drawLine(x4-offset,y4-offset,x5-offset,y5-offset,"blue",5)
    x6,y6 = board.VertexPositions[41]
    gui._drawLine(x6+offset,y6-offset,x5-offset,y5-offset,"blue",5)
    x7,y7 = board.VertexPositions[21]
    gui._drawLine(x6+offset,y6-offset,x7+offset,y7-offset,"blue",5)
    x8,y8 = board.VertexPositions[23]
    gui._drawLine(x8-offset,y8-offset,x7+offset,y7-offset,"blue",5)
    x9,y9 = board.VertexPositions[42]
    gui._drawLine(x8-offset,y8-offset,x9-offset,y9-offset,"blue",5)
    x10,y10 = board.VertexPositions[44]
    gui._drawLine(x10+offset,y10-offset,x9-offset,y9-offset,"blue",5)
    x11,y11 = board.VertexPositions[25]
    gui._drawLine(x10+offset,y10-offset,x11+offset,y11-offset,"blue",5)
    x12,y12 = board.VertexPositions[26]
    gui._drawLine(x12+offset,y12-offset,x11+offset,y11-offset,"blue",5)
    gui._drawLine(x12+offset,y12-offset,x2+offset,y2+offset,"blue",5)

def isValidEdge(board,edge):
    if (edge<=71) and (edge>=0) and board.getEdgeState(edge)==const.UNPLAYED:
        return True
    else:
        return False

def drawEdge(gui,board,edge):
    v1,v2 = board.Edges[edge]
    x1,y1 = board.VertexPositions[v1]
    x2,y2 = board.VertexPositions[v2]
    
    gui._drawLine(x1, y1, x2, y2, "black", 3)

def fillSquare(gui,board,square,turn):
    e1,e2,e3,e4=board.Squares[square]
    x,y=board.VertexPositions[board.Edges[e1][0]]
    fillcolor = "red" if turn==const.PLAYER1 else "green"
    gui._fillRectangle(x, y, const.BOARDSCALE, const.BOARDSCALE, fillcolor)
    # Redraw black dots
    x,y = board.VertexPositions[board.Edges[e1][0]]
    gui._fillCircle(x, y, const.DOTRADIUS)
    x,y = board.VertexPositions[board.Edges[e1][1]]
    gui._fillCircle(x, y, const.DOTRADIUS)
    x,y = board.VertexPositions[board.Edges[e4][0]]
    gui._fillCircle(x, y, const.DOTRADIUS)
    x,y = board.VertexPositions[board.Edges[e4][1]]
    gui._fillCircle(x, y, const.DOTRADIUS)
    x,y = board.VertexPositions[square]
    gui.drawPie(x+const.BOARDSCALE/2, y-const.BOARDSCALE/1.9, fillcolor)

def playGame(gui,player1name,player2name,player1,player2,playsfirst,scorePlayer1,scorePlayer2):
    # Setup and draw board
    board = GameBoardADT()
    drawBoard(gui,board)
    gui.drawNames(player1name,player2name,scorePlayer1,scorePlayer2)
    scores = [None,0,0]
    
    turn = playsfirst

    # For every edge on the board, take a turn. If a square is not captured, switch players. If a square is captured, update scores and states and request another move from the same player.
    for i in range(72):
        gui.drawCurrentPlayer(turn)
        #        print board.getGameState()
        #edge = getEdge(board)
        # Request the current game state
        state = board.getGameState()
        # Send game state to current player and request move
        edge = player1.chooseMove(state) if turn==const.PLAYER1 else player2.chooseMove(state)
        if isValidEdge(board,edge)==False:
            # Current player has made an invalid move and forfeits the game
            scores[turn]=-1
            break
        # Draw the new edge on the board and update the game state
        drawEdge(gui,board,edge)
        board.setEdgeState(edge,const.PLAYED)
        # Check whether any squares have been captured and update scores accordingly
        captured = board.findCaptured(edge)
        if len(captured)==0:
            # No squares captured so switch turns
            turn = const.PLAYER1 if turn==const.PLAYER2 else const.PLAYER2
        elif len(captured)==1:
            # One square captured
            scores[turn]+=1
            fillSquare(gui,board,captured[0],turn)
            board.setSquareState(captured[0],turn)
            gui.addPie(turn,scores[turn])
        elif len(captured)==2:
            # Two squares captured
            scores[turn]+=2
            fillSquare(gui,board,captured[0],turn)
            board.setSquareState(captured[0],turn)
            gui.addPie(turn,scores[turn]-1)
            fillSquare(gui,board,captured[1],turn)
            board.setSquareState(captured[1],turn)
            gui.addPie(turn,scores[turn])
    # Game is over. Return match score (one-nil win or draw)
    if scores[const.PLAYER1]>scores[const.PLAYER2]:
        return (1,0)
    elif scores[const.PLAYER2]>scores[const.PLAYER1]:
        return (0,1)
    else:
        return (0.5,0.5)

def playMatch(gui, rounds, player1name,player2name,player1,player2):
    """
        function handling a match between two players. Each match may be composed of several games/rounds.
        @param <BattleshipsGraphics> gui, the graphic interface displaying the match.
        @param <PlayerSocket> firstPlayer, secondPlayer: The player objects
        """
    scorePlayer1 = scorePlayer2 = 0
    # Randomly choose who plays first in the first game
    playsfirst = random.choice([const.PLAYER1,const.PLAYER2])
    for game in range(rounds):
        gui.turtle.clear()
        (score1,score2)=playGame(gui,player1name,player2name,player1,player2,playsfirst,scorePlayer1,scorePlayer2)
        scorePlayer1+=score1
        scorePlayer2+=score2
        if scorePlayer1>rounds/2.0:
            # Player 1 wins match
            player1.close("Win")
            player2.close("Lose")
            winnername = player1name
            break
        if scorePlayer2>rounds/2.0:
            # Player 2 wins match
            player2.close("Win")
            player1.close("Lose")
            winnername = player2name
            break
        playsfirst = const.PLAYER1 if playsfirst==const.PLAYER2 else const.PLAYER2
        player1.newGame()
        player2.newGame()
    if scorePlayer1==scorePlayer2:
        gui.reportDraw()
    else:
        gui.finalScore(winnername,max(scorePlayer1,scorePlayer2),min(scorePlayer1,scorePlayer2))

# Main

print "Waiting for connection..."
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)    # Create a socket object
sock.bind((const.GAME_SERVER_ADDR, const.GAME_SERVER_PORT)) # The same port as used by the server & clients
sock.listen(2)                                              # Now wait for client connection (max of 2 client at a time).

client1, addr1 = sock.accept()                          # Establish connection with first client.
print 'Got connection from', addr1
player1 = PlayerSocket(client1, addr1)                  # Create first player with that connection
player1.acknowledgeConnection()
player1name = player1.getName(const.PLAYER1)
print "player",player1name,"is connected..."

client2, addr2 = sock.accept()                          # Establish connection with second client.
print 'Got connection from', addr2
player2 = PlayerSocket(client2, addr2)                  # Create second player with second connection
player2.acknowledgeConnection()
player2name = player2.getName(const.PLAYER2)
print "player",player2name,"is connected..."

gui = PiSquareGraphics()

playMatch(gui,const.ROUNDS,player1name,player2name,player1,player2)

print "Click game board to exit"
## Must be the last line of code
gui.screen.exitonclick()

