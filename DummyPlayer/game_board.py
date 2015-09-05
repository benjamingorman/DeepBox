import const

class GameBoardADT:

    def __init__(self):
        # Vertices are numbered 0-44 working left to right, top to bottom.
        # Edges are stored as ordered pairs of vertices.
        self.Edges = ((0,1),(1,2),(2,3),(3,4),(4,5),(5,6),(6,7),(7,8),
                      (0,9),(1,10),(2,11),(3,12),(4,13),(5,14),(6,15),(7,16),(8,17),
                      (9,10),(10,11),(11,12),(12,13),(13,14),(14,15),(15,16),(16,17),
                      (9,18),(10,19),(11,20),(12,21),(13,22),(14,23),(15,24),(16,25),(17,26),
                      (18,19),(19,20),(20,21),(21,22),(22,23),(23,24),(24,25),(25,26),
                      (19,27),(20,28),(21,29),(23,30),(24,31),(25,32),
                      (27,28),(28,29),(30,31),(31,32),
                      (27,33),(28,34),(29,35),(30,36),(31,37),(32,38),
                      (33,34),(34,35),(36,37),(37,38),
                      (33,39),(34,40),(35,41),(36,42),(37,43),(38,44),
                      (39,40),(40,41),(42,43),(43,44))

        # Vertex positions given with unit spacing
        # Board occupies rectangle 9 wide and 5 high before scaling
        self.VertexPositions = ((0,5),(1,5),(2,5),(3,5),(4,5),(5,5),(6,5),(7,5),(8,5),
                                (0,4),(1,4),(2,4),(3,4),(4,4),(5,4),(6,4),(7,4),(8,4),
                                (0,3),(1,3),(2,3),(3,3),(4,3),(5,3),(6,3),(7,3),(8,3),
                                      (1,2),(2,2),(3,2),      (5,2),(6,2),(7,2),
                                      (1,1),(2,1),(3,1),      (5,1),(6,1),(7,1),
                                      (1,0),(2,0),(3,0),      (5,0),(6,0),(7,0))

        # For each square (indexed by the vertex at the top right), this stores the four edges that surround it
        # Note: squares are referred to by the vertex number on their top left. This means that the square indexes are not continuous. For this reason, they are stored in a dictionary indexed by the appropriate vertex number.
        self.Squares = {0:(0,8,9,17), 1:(1,9,10,18), 2:(2,10,11,19), 3:(3,11,12,20), 4:(4,12,13,21),
                            5:(5,13,14,22), 6:(6,14,15,23), 7:(7,15,16,24),
                        9:(17,25,26,34), 10:(18,26,27,35), 11:(19,27,28,36), 12:(20,28,29,37), 13:(21,29,30,38),
                            14:(22,30,31,39), 15:(23,31,32,40), 16:(24,32,33,41),
                        19:(35,42,43,48), 20:(36,43,44,49), 23:(39,45,46,50), 24:(40,46,47,51),
                        27:(48,52,53,58), 28:(49,53,54,59), 30:(50,55,56,60), 31:(51,56,57,61),
                        33:(58,62,63,68), 34:(59,63,64,69), 36:(60,65,66,70), 37:(61,66,67,71)}

        # For each edge, this stores the one or two squares adjacent to the edge
        self.Edge2Squares = (0,1,2,3,4,5,6,7,
                             0,(0,1),(1,2),(2,3),(3,4),(4,5),(5,6),(6,7),7,
                             (0,9),(1,10),(2,11),(3,12),(4,13),(5,14),(6,15),(7,16),
                             9,(9,10),(10,11),(11,12),(12,13),(13,14),(14,15),(15,16),16,
                             9,(10,19),(11,20),12,13,(14,23),(15,24),16,
                             19,(19,20),20,23,(23,24),24,
                             (19,27),(20,28),(23,30),(24,31),
                             27,(27,28),28,30,(30,31),31,
                             (27,33),(28,34),(30,36),(31,37),
                             33,(33,34),34,36,(36,37),37,
                             33,34,36,37)

        # Set up variables for holding game state
        # Initially no edges have been played and no squares captured
        self.EdgeStates = [const.UNPLAYED]*72
        self.SquareStates  = dict.fromkeys([0,1,2,3,4,5,6,7,9,10,11,12,13,14,15,16,19,20,23,24,27,28,30,31,33,34,36,37],const.UNCAPTURED)

    def testCaptured(self,square):
        '''Check whether the given square is now captured. Returns boolean.'''
        e1,e2,e3,e4=self.Squares[square]
        if self.EdgeStates[e1]!=const.UNCAPTURED and self.EdgeStates[e2]!=const.UNCAPTURED and self.EdgeStates[e3]!=const.UNCAPTURED and self.EdgeStates[e4]!=const.UNCAPTURED:
            return True
        else:
            return False

    def setEdgeState(self,edge,state):
        '''Update the state of an edge'''
        self.EdgeStates[edge]=state

    def getEdgeState(self,edge):
        return self.EdgeStates[edge]

    def setSquareState(self,square,state):
        '''Update the state of a square'''
        self.SquareStates[square]=state
    
    def getSquareState(self,square):
        return self.SquareStates[square]

    def getUnplayedEdges(self):
        unplayed = []
        for i in range(72):
            if self.EdgeStates[i]==const.UNPLAYED:
                unplayed.append(i)
        return unplayed

    def findCaptured(self,edge):
        squares = self.Edge2Squares[edge]
        captured = []
        if type(squares)==tuple:
            # There are two squares adjacent to the given edge
            if self.testCaptured(squares[0])==True:
                captured = [squares[0]]
            if self.testCaptured(squares[1])==True:
                captured.append(squares[1])
        else:
            # There is only one square adjacent to the given edge
            if self.testCaptured(squares)==True:
                captured = [squares]
        return captured

    def getGameState(self):
        '''Return a string encoding the current game state'''
        state = ""
        for i in range(len(self.EdgeStates)):
            state = state + ("0" if self.EdgeStates[i]==const.UNPLAYED else
                             ("1" if self.EdgeStates[i]==const.PLAYER1 else "2"))
        for square in self.SquareStates:
            state = state + ("0" if self.SquareStates[square]==const.UNCAPTURED else
                             ("1" if self.SquareStates[square]==const.PLAYER1 else "2"))
        return state

    def setGameState(self,state):
        '''Used by players to update their board when sent the current game state'''
        for i in range(len(self.EdgeStates)):
            self.EdgeStates[i] = int(state[i])
        count = 0
        for square in self.SquareStates:
            self.SquareStates[square]=int(state[count+len(self.EdgeStates)])
            count+=1