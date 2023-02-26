#ifndef MODULE_H
#define MODULE_H

#include "config.h"
#include "motor.h"
#include "can_com.h"

#define SERVO_COUNT 4

// message structure
// binary: if true, 8 bits of a message byte can be used to set the motor to discrete positions
// mask: CAN mask
// filter: CAN filter
// offset: start byte to be used from the max 8 bytes of a message
// bytes: number of bytes to be used for an analog value (1, 2)
struct MESSAGE {
	bool binary;
	uint16_t mask;
	uint16_t filter;
	uint8_t offset;
	uint8_t bytes;
};


class MODULE {

	public:
		void begin(void);
		void clear_message(void);
		void register_message(uint16_t mask, uint16_t filter, uint8_t offset, uint8_t bytes);

		void update(void);

	private:
		MOTOR _servo[SERVO_COUNT];
		MESSAGE _message[SERVO_COUNT];
		uint8_t _message_count;
};

#endif