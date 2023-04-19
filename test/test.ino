#include <Arduino.h>

#include "can_com.h"
#include "can_protocol.h"

CAN_COM can;

void setup(void) {

	Serial.begin(115200);

	can.begin(500E3, 5);
}


void loop(void) {

	uint8_t data[8];

	for (uint8_t i=0; i<255; i+=10) {

		// send different messages
		data[0] = i;
		can.send(data, 2, CAN_ID_VOLTAGE);

		data[0] = 0xaa;
		can.send(data, 1, CAN_ID_DRIVE_STATUS);

		data[0] = 0x02;
		can.send(data, 1, CAN_ID_LIGHT);

		delay(300);
	}

	delay(1000);

}