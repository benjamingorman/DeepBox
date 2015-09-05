UNCAPTURED = 0
UNPLAYED = 0
PLAYED = 1
PLAYER1 = 1
PLAYER2 = 2

ROUNDS = 3    ## Max number of rounds per match (best of)

BOARD_BKG_GIF = "images/background.gif"
PIE_GIF = "images/pie.gif"
REDPIE_GIF = "images/redpie.gif"
GREENPIE_GIF = "images/greenpie.gif"

BOARDSCALE = 50
BOARDX = -4*BOARDSCALE
BOARDY = -1.5*BOARDSCALE

DOTRADIUS = 5

# Vertex positions given with unit spacing
# Board occupies rectangle 9 wide and 5 high before scaling
VertexPositions = ((0,5),(1,5),(2,5),(3,5),(4,5),(5,5),(6,5),(7,5),(8,5),
                   (0,4),(1,4),(2,4),(3,4),(4,4),(5,4),(6,4),(7,4),(8,4),
                   (0,3),(1,3),(2,3),(3,3),(4,3),(5,3),(6,3),(7,3),(8,3),
                   (1,2),(2,2),(3,2),      (5,2),(6,2),(7,2),
                   (1,1),(2,1),(3,1),      (5,1),(6,1),(7,1),
                   (1,0),(2,0),(3,0),      (5,0),(6,0),(7,0))

############################################################
##
## Constants used for the client/server functionalities
##
############################################################
# Constant containing the IP address of the server
GAME_SERVER_ADDR = ''   #meaning the server is on the local host. Comment if server is elsewhere and provide
                        # an address like the one below
# GAME_SERVER_ADDR = '10.240.74.225' 
 
# Port used for the socket communication. You must ensure the same port is used by the client & the server
# Note: another port number could be used
GAME_SERVER_PORT = 12345 

# Constant used in the communication to acknowledge a received message
# Typically this constant is used to synchronise the clients and the server
ACKNOWLEDGED = 'ACK'