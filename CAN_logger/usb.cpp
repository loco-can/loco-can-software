#include "usb.h"


void USB::send(CAN_MESSAGE message) {

	uint8_t id, i;
	uint8_t data[8];


	Serial.print("SIZE:");		
	Serial.print(message.size);
	Serial.print(";");		

	Serial.print("ID:");		
	Serial.print(message.id, HEX);
	Serial.print(";");		

	Serial.print("UUID:");		
	Serial.print(message.uuid, HEX);

	for (i = 0; i < message.size; i++) {
		Serial.print(";");
		Serial.print("D");		
		Serial.print(i);		
		Serial.print(":");		

		Serial.print(message.data[i], BIN);
	}

	Serial.println("");
}