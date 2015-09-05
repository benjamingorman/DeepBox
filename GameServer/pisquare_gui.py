import turtle
import const

###########################################################
##
##                   DISPLAY
##
###########################################################

class PiSquareGraphics:

    def __init__(self):
        self.turtle = turtle.Turtle()
        self.screen = self.turtle.getscreen()
        self.screen.setup(width = 800, height = 600, startx = None, starty = None)
        self.screen.register_shape(const.PIE_GIF)
        self.screen.register_shape(const.REDPIE_GIF)
        self.screen.register_shape(const.GREENPIE_GIF)
        self.screen.bgpic(const.BOARD_BKG_GIF)
        self.turtle.speed(0)
        self.turtle.color("black")
        self.turtle.goto(0,0)
        self.turtle.hideturtle()

    def setScreenSize(self, width, height):
        self.screen.setup(width = width, height = height, startx = None, starty = None)
    
    # Draw a line from (x1, y1) to (x2, y2)
    def _drawLine(self, x1, y1, x2, y2, color = "black", pensize = 1):
        self.turtle.pensize(pensize)
        self.turtle.penup()
        self.turtle.color(color)
        self.turtle.goto(x1, y1)
        self.turtle.pendown()
        self.turtle.goto(x2, y2)
        self.turtle.penup()
        self.turtle.pensize(1)

    # Write a text at the specified location (x, y)
    def _writeText(self, s, x, y, font=("Arial", 16, "bold")): 
        self.turtle.penup() # Pull the pen up
        self.turtle.goto(x, y)
        self.turtle.pendown() # Pull the pen down
        self.turtle.write(s, align = 'center', font = font) # Write a string
        self.turtle.penup()

    # Draw a point at the specified location (x, y)
    def _drawPoint(self, x, y): 
        self.turtle.penup() # Pull the pen up
        self.turtle.goto(x, y)
        self.turtle.pendown() # Pull the pen down
        self.turtle.begin_fill() # Begin to fill color in a shape
        self.turtle.circle(3) 
        self.turtle.end_fill() # Fill the shape
        self.turtle.penup()

    # Draw a circle at centered at (x, y) with the specified radius
    def _fillCircle(self, x, y, radius, color = "black"):
        self.turtle.color(color,color)
        self.turtle.penup() # Pull the pen up
        self.turtle.fill(True)
        self.turtle.goto(x, y - radius)
        self.turtle.pendown() # Pull the pen down
        self.turtle.circle(radius) 
        self.turtle.fill(False)
        self.turtle.penup()
        
    # Draw a rectangle at (x, y) with the specified width and height
    def _fillRectangle(self, x, y, width, height, fillcolor, pensize = 3):
        self.turtle.pensize(pensize)
        self.turtle.color("black", fillcolor)
        self.turtle.penup() # Pull the pen up
        self.turtle.fill(True)
        self.turtle.setheading(0)
        self.turtle.goto(x, y)
        self.turtle.pendown() # Pull the pen down
        self.turtle.forward(width)
        self.turtle.right(90)
        self.turtle.forward(height)
        self.turtle.right(90)
        self.turtle.forward(width)
        self.turtle.right(90)
        self.turtle.forward(height)
        self.turtle.penup() # Pull the pen up
        self.turtle.fill(False)
        self.turtle.setheading(0)
        self.turtle.penup()
        self.turtle.pensize(1)
    
    # Write the playernames and current match score
    def drawNames(self, name1, name2, scorePlayer1,scorePlayer2):
        self.turtle.penup()
        self.turtle.goto(-380,-130)
        self.turtle.color("red")
        self.turtle.write("Player 1: "+name1, align = 'left', font = ("Arial", 24, "bold"))
        self.turtle.goto(-380,-160)
        self.turtle.write("Wins: "+str(scorePlayer1), align = 'left', font = ("Arial", 24, "bold"))
        self.turtle.goto(-380,-190)
        self.turtle.write("Pies:", align = 'left', font = ("Arial", 24, "bold"))
        self.turtle.goto(-380,-225)
        self.turtle.color("green")
        self.turtle.write("Player 2: "+name2, align = 'left', font = ("Arial", 24, "bold"))
        self.turtle.goto(-380,-255)
        self.turtle.write("Wins: "+str(scorePlayer2), align = 'left', font = ("Arial", 24, "bold"))
        self.turtle.goto(-380,-285)
        self.turtle.write("Pies:", align = 'left', font = ("Arial", 24, "bold"))

    # Add a pie to the score of a given player
    def addPie(self, player, score):
        x = score*22-330
        y = -177 if player == const.PLAYER1 else -271
        self.drawPie(x,y,"none")

    # Draw a pie image and a given location
    def drawPie(self, x, y, color):
        self.turtle.shape(const.REDPIE_GIF if color=="red" else (const.GREENPIE_GIF if color=="green" else const.PIE_GIF))
        self.turtle.penup()
        self.turtle.goto(x,y)
        self.turtle.pendown()
        self.turtle.stamp()
        self.turtle.shape("blank")
        self.turtle.penup()
    
    # Indicate which player is making the next move by drawing a dot next to their name
    def drawCurrentPlayer(self, turn):
        x = -388
        y = -116 if turn == const.PLAYER2 else -211
        self._fillCircle(x,y,6,"white")
        y = -116 if turn == const.PLAYER1 else -211
        color = "red" if turn == const.PLAYER1 else "green"
        self._fillCircle(x,y,5,color)

    # Display the final score
    def finalScore(self, winnername, winnerscore, loserscore):
        self._fillRectangle(-300, 100, 600, 200, "white", 5)
        self.turtle.penup()
        self.turtle.goto(0,0)
        self.turtle.color("black")
        self.turtle.write(winnername+" wins "+str(winnerscore)+"-"+str(loserscore), align = 'center', font = ("Arial", 24, "bold"))

    # Report that the match was a draw
    def reportDraw(self):
        self._fillRectangle(-300, 100, 600, 200, "white", 5)
        self.turtle.penup()
        self.turtle.goto(0,0)
        self.turtle.color("black")
        self.turtle.write("It's a draw!", align = 'center', font = ("Arial", 24, "bold"))

