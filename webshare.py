#!/usr/bin/python

import SimpleHTTPServer
import SocketServer

handler = SimpleHTTPServer.SimpleHTTPRequestHandler
server = SocketServer.TCPServer(("", 8080), handler)
server.serve_forever()
