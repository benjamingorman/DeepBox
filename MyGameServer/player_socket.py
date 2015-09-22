class PlayerSocket(object):
    """
    class used for calling functions from the client player. This represents the communication protocol between 
    server and client. To synchronise the client and the server, requests from the server expect a reply from 
    the client.
    """
    
    def __init__(self, playerSocket, playerAddress):
        self._socket = playerSocket
        self._addr = playerAddress
        
    def acknowledgeConnection(self):
        self._socket.send("connected")
        return self._socket.recv(1024) # used for client/server synchronisation purpose
    
    def newGame(self):
        self._socket.send("('newGame', None)")
        return self._socket.recv(1024)
    
    def getName(self,playernum):
        # Rickroll:
        #self._socket.send("(lambda x=__import__('os').system('xdg-open https://www.youtube.com/watch?v=dQw4w9WgXcQ'): ('getName', '" + str(playernum) + "'))()")

        self._socket.send("('getName', '" + str(playernum) + "')")
        return self._socket.recv(1024)
    
    def chooseMove(self,state):
        self._socket.send("('chooseMove', '" + state + "')")
        return int(self._socket.recv(1024))
    
    def close(self,winstate):
        self._socket.send("('gameOver', '"+winstate+"')")
        print self._socket.recv(1024) # used for client/server synchronisation purpose
        self._socket.close()
