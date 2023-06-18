import os
import serial
import serial.tools.list_ports


class SerialCAN:


	def __init__(self):
		self.serialPort = False

		pass


	def connect(self):

		USBport = False;
		USBname = False;

		ports = serial.tools.list_ports.comports(True)

		if (os.name == 'posix'):
			os_filter = "FT232"
		else:
			os_filter = "USB"


		# myports = [tuple(p) for p in list(serial.tools.list_ports.comports())]
		# print(myports)
		# exit()

		# get USB ports with FT232 description
		for port, desc, hwid in sorted(ports):

			# print(port, ' ', desc)

			if desc.find(os_filter) != -1:

				USBport = port
				USBname = desc
				break


		p = "/dev/ttyS6"

		print("connect to " + p)

		try:

			self.serialPort = serial.Serial(
				port=p, baudrate=115200, bytesize=8, timeout=2, stopbits=serial.STOPBITS_ONE
			)

		except:
			print("connection error")
			exit(0)


		# connect to serial adapter
		# if USBport != False:

		#     print("connected to ", USBport, USBname)
		#     return serial.Serial(port=USBport, baudrate=115200, timeout=.2)

		# else:
		#     print("no active FT232 found");
		#     # exit()
		#     return False

		return True


	# message on serial is available
	# return count
	def available(self):
		if self.serialPort.in_waiting > 0:
			return self.serialPort.in_waiting

		else:
			return False


	# read line and return message object
	def read(self):
		data = self.serialPort.readline()

		if data:
			 return self.parse(data)

		else:
			return False


	# parse string line to message object
	def parse(self, data):

		start_bit = 255
		start_bit_count = 2
		end_string = "LOCO"

		ret = False
		valid = True;
		data_list = list(data);


		# check for valid format
		# [0,1] == 0xFF
		# [end] == LOCO\r\n
		start = data_list[0:2]
		end = data_list[-1*(len(end_string)+2):-2]

		# check start
		for i in start:
			if (i != start_bit):
				valid = False

		# check end
		for k, i in enumerate(end):
			if i != end[k]:
				valid = False


		# is valid
		if valid:

			ret = {}

			# get identifiers
			ret["can_id"] = int.from_bytes(bytes(data_list[3:7]), byteorder="big")
			ret["uuid"] = int.from_bytes(bytes(data_list[7:9]), byteorder="big")


			# get data
			ret["size"] = data_list[2]

			ret["data"] = [];
			for d in data_list[9:9+ret["size"]]:
				ret["data"].append(d)


			# get checksum
			ret["checksum"] = data_list[len(data_list) - len(end_string) - 3]


		return ret
		pass