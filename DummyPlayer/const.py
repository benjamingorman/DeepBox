UNCAPTURED = 0
UNPLAYED = 0
PLAYED = 1
PLAYER1 = 1
PLAYER2 = 2

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