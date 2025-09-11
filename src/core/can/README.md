## CAN bus communication

CAN message are received from the can interface. Each function in a module can create messages, that must be distributed to all internal function and sent to the bus in the end. 

The class has a fifo buffer to achieve this goal. All messages, both from external and the internal function are added to this buffer. 

Each loop the top most message is extracted from the bus and transferred to the first function.