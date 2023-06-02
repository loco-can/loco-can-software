#include <Arduino.h>


#include "loco_config.h"
#include "can_protocol.h"
#include "can_com.h"
#include "intellitimeout.h"

// servo = 9
// motor = 5
#define CAN_STATUS_LED 5

#define SEND_TIMEOUT 500
#define SEND_ID CAN_ID_DRIVE_HEARTBEAT
#define DEBUG

CAN_COM can_com;
CAN_MESSAGE message;

INTELLITIMEOUT send_timeout;

int8_t msg;
uint8_t send_msg;
uint8_t msg_cnt;

void setup(void) {

	Serial.begin(115200);

	can_com.begin(CAN_BUS_SPEED, CAN_STATUS_LED);

	send_timeout.begin(SEND_TIMEOUT);

	Serial.println("input binary 8 bit value to be sent");

	send_msg = 0;
}


void loop(void) {

	uint8_t i;
	uint16_t filter;
	uint8_t buffer[8];


	msg = Serial.read();


// 	// get data from serial
// 	if (msg != -1) {
// Serial.println(msg);

// 		// not end of message
// 		if (msg != 10 && msg_cnt < 8) {

// 			switch(msg) {

// 				// add 0
// 				case 48:

// 					break;

// 				// add 1
// 				case 49:
// 					break;
// 			}
// 		}

// 		// end of message
// 		else {
// 			send_msg = msg;
// 		}
// 	}

	// =========================================
	// send message
	if (send_timeout.update()) {

		buffer[0] = 0xFF;
		buffer[1] = 0x88;
		buffer[2] = 0x00;
		buffer[3] = 0xAA;
		can_com.send(buffer, 4, CAN_ID_DRIVE);

		buffer[0] = 0x12;
		can_com.send(buffer, 1, CAN_ID_SPEED);
	}


	// =========================================
	// received message
	filter = can_com.read(&message);


	// receive message
	if (filter) {
		Serial.print("receive id 0x");
		Serial.print(filter, HEX);

		Serial.print(" data ");
		Serial.println(message.data[0], BIN);
	}


	// no message
	// else {
		// Serial.print(".");

		// i++;

		// if (i > 30) {
		// 	Serial.println();
		// 	i = 0;
		// }
	// }


}
