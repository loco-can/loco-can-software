## Internal CAN bus communication

CAN message are received from the can interface. Each function in a module can create messages, that must be distributed to all internal function and sent to the external bus in the end. 

The class has a fifo buffer to achieve this goal. All messages, both from external and the internal function are added to this buffer with an integer value that stores the function id.

With each program loop the top most message is extracted from the buffer and transferred to the functions, one after another. Each function can add its own messages to the buffer when it is called. All messages with a function id that is not zero will be sent to the external bus after all function are executed.