#include "can_message.h"

extern CAN_MESSAGE can_message;

// return can_message struct from data
CAN_MESSAGE data2message(uint32_t id, uint16_t uuid, uint8_t* data, uint8_t size) {

	can_message.id = id;
	can_message.uuid = uuid;

	for (uint8_t i = 0; i < size; i++) {
		can_message.data[i] = data[i];
	}

	can_message.size = size;

	return can_message;
}