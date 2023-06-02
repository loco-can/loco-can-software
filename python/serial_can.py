import serial
import time

# ser = serial.Serial('/dev/ttyS3', 115200, timeout=1)

serialPort = serial.Serial(
    port="/dev/ttyS6", baudrate=115200, bytesize=8, timeout=2, stopbits=serial.STOPBITS_ONE
)

serialString = ""  # Used to hold data coming over UART

start_bit = 255
start_bit_count = 2
end_string = "LOCO"


def parse(data):

    ret = {}
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


while 1:

    # Wait until there is data waiting in the serial buffer
    if serialPort.in_waiting > 0:

        # Read data out of the buffer until a carraige return / new line is found
        data = serialPort.readline()


        # end of line
        if (data):

            print(parse(data))

