import time
import signal

from multiprocessing import Process

from modules.message import Message
from modules.http import HTTP
from modules.serial import SerialCAN


# handler for program abort
def handler(signum, frame):
	# res = input("ctrl-c pressed: exit?")
	# if (res == 'y'):
	exit(1)


# clear console
def cls():
	os.system('cls' if os.name=='nt' else 'clear')


# http server
def server():

	http = HTTP()
	http.start(34567)


# logger
def logger():

	print("start CAN logger")

	sCAN = SerialCAN()

	serialPort = sCAN.connect()
	serialString = ""  # Used to hold data coming over UART

	m = Message()

	print("start logging")

	while 1:

		# Wait until there is data waiting in the serial buffer
		if sCAN.available():

			# Read data out of the buffer until a carraige return / new line is found
			data = sCAN.read()

			# end of line
			if data:

				m.add(data)

				# cls()
				# print(Message.can_id())
				print(m.uuid())


# main routine
if __name__ == "__main__":

	signal.signal(signal.SIGINT, handler)

	p1 = Process(target=server)
	p1.start()

	p2 = Process(target=logger)
	p2.start()

	p1.join()
	p2.join()