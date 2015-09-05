'''
This module handle the client side of the client/server communication of the game.
It calls the functions defined in the Player_strategy module and send the output/input 
information needed by the server.

Created on 7 Aug 2014

@author: Lilian
'''

import socket # Import socket module
import player_strategy as player # import the module created by the student containing the strategy
import const
import re

def main():
    game_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM) # Create a socket object
    if const.GAME_SERVER_ADDR == '':        
        host = socket.gethostname() # Get local machine name
        game_server.connect((host, const.GAME_SERVER_PORT))
    else:
        game_server.connect((const.GAME_SERVER_ADDR, const.GAME_SERVER_PORT))
        
    status = game_server.recv(1024)                     # used for client/server synchronisation purpose
    game_server.send(const.ACKNOWLEDGED)    # used for client/server synchronisation purpose    
    print status
    
    if status == "connected":
        while True:
            request = game_server.recv(1024)
            print "dummy side rqt:", request, "|"

            regex_match = re.match("\((.+), (.+)\)", request)
            if not regex_match:
                # Request from server is malformed
                print("[ERROR] Could not parse request from server: " + request)
                game_server.send(const.ACKNOWLEDGED)
                break
            else:
                command = regex_match.group(1).strip("'")
                data = regex_match.group(2).strip("'")

                print("Received command: " + command)
                print("Received data: " + data)


            if command == "getName":
                player.myPlayerNumber = int(data)
                print "I am player "+str(player.myPlayerNumber)
                game_server.send(str(player.playerName))

            elif command == "chooseMove":
                game_server.send(str(player.chooseMove(data)))
            
            elif command == "newGame":
                player.newGame()
                game_server.send(const.ACKNOWLEDGED)
            
            elif command == "gameOver":
                print "I won!" if data=="Win" else "I lost!"
                break
                    
            else:
                # unknown commmand so must be either end of game or a fatal error. Exit the game loop
                print("[ERROR] Unknown command received in request: " + request)
                game_server.send(const.ACKNOWLEDGED)
                break
            

    game_server.close # Close the socket when done

        
main()
