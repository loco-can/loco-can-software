#ifndef USB_H
#define USB_H

// send can message to serial

#include "can_com.h"


class USB {

	public:
		static void send(CAN_MESSAGE message);

};

#endif