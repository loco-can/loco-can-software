#include "module.h"


// init extern classes
extern CAN_COM can_com;

CAN_MESSAGE message;


void MODULE::begin(void) {
	
	_servo[0].begin(SERVO_1);
	_servo[1].begin(SERVO_2);
	_servo[2].begin(SERVO_3);
	_servo[3].begin(SERVO_4);

	clear_message();
}

void MODULE::clear_message(void) {
	_message_count = 0;
}


// register a message to get the value for a motor
// mask and filter for CAN message
// bytes => number of bytes to be used for the value
void MODULE::register_message(uint16_t mask, uint16_t filter, uint8_t offset, uint8_t bytes) {

	if (_message_count < SERVO_COUNT) {

		_message[_message_count].mask = mask;
		_message[_message_count].filter = filter;
		_message[_message_count].offset = offset;
		_message[_message_count].bytes = bytes;

		_message_count++;
	}
}

void MODULE::update(void) {

	uint8_t i;
	uint16_t filter;


	// =================================================0
	// data section
	// check for message
	// set motors
	if (filter = can_com.read(&message)) {

		while (i < SERVO_COUNT) {

			// matches registered filter
			if (filter = _message[i].filter) {

				// 
			}

			i++;
		}
	}
}