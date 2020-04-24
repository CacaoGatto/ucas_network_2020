import SimpleHTTPServer
import SocketServer

PORT = 80
Protocol = "HTTP/1.1"

Handler = SimpleHTTPServer.SimpleHTTPRequestHandler
Handler.protocol_version = Protocol
httpd = SocketServer.TCPServer(("", PORT), Handler)

print "Serving at port", PORT
httpd.serve_forever()
