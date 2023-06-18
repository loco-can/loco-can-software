import http.server
import socketserver

class HTTP:

	def __init__(self):
		pass


	def start(self, port):

	    print("start http server")

	    # http server
	    PORT = 34567
	    Handler = http.server.SimpleHTTPRequestHandler

	    with socketserver.TCPServer(("", PORT), Handler) as httpd:
	        print("serving at port", PORT)
	        httpd.serve_forever()

	    print("end http server")


	def on_GET():
	    print("get");
