#include <Arduino.h>
#include "serialcan.h"


SERIALCAN s_can;
CAN_MESSAGE message;

void setup(void) {

	s_can.begin(115200);
}


void loop(void) {

	// read from serial
	if (s_can.available()) {

		s_can.read(&message);
		// s_can.send(&message);
	}

}