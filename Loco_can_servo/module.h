#ifndef MODULE_H
#define MODULE_H

#include "config.h"
#include "intelliservo.h"
#include "can_com.h"

#define SERVO_COUNT 4

// message structure
// binary: if true, 8 bits of a message byte can be used to set the motor to discrete positions
// mask: CAN mask
// filter: CAN filter
// offset: start byte to be used from the max 8 bytes of a message
// bytes: number of bytes to be used for an analog value (1, 2)
struct SERVO_MESSAGE {
	bool binary;
	uint16_t mask;
	uint16_t filter;
	uint8_t offset;
	uint8_t bytes;
};


// map analoge value to analog position
// map analoge value area to position, if type is 
struct SERVO_ANALOG {
	uint16_t min;
	uint16_t max;
	uint8_t pos_1;
	uint8_t pos_2;
};

// go to a position, if the analoge value is inside the value area
struct SERVO_AREA {
	uint16_t min;
	uint16_t max;
	uint8_t position;
};

// map binary to two positions
struct SERVO_BINARY {
	uint8_t pos_false;
	uint8_t pos_true;
};

// select position by a certain bit combination of the byte
// only bits, that are set to 1 in the mask will be compared
struct SERVO_BYTE {
	uint8_t byte;
	uint8_t mask;
	uint8_t position;
};

// each servo has settings determining the mapping of message values
// SERVO_MESSAGE:	the message id and mask
// type: the type of value mapping
// 		SERVO_ANALOG_MAPPING		map an analoge value to an angle area
// 		SERVO_AREA_MAPPING			map up to 16 value areas to discrete positions
// 		SERVO_BINARY_MAPPING		map a binary value to two positions
// 		SERVO_STEP_MAPPING			map up the 8 bit combinations of a byte to different positions
struct SERVO_SETTINGS {
	SERVO_MESSAGE message;
	uint8_t type;

	SERVO_ANALOG analog;
	SERVO_AREA area[16];
	SERVO_BINARY binary;
	SERVO_BYTE bytes[8];
};




class MODULE {

	public:
		void begin(void);
		void clear_message(void);
		void register_message(uint16_t mask, uint16_t filter, uint8_t offset, uint8_t bytes);

		void update(void);

	private:
		INTELLISERVO _servo[SERVO_COUNT];
		SERVO_MESSAGE _message[SERVO_COUNT];
		uint8_t _message_count;
};

#endif