#include "can_message.h"


// return can_message struct from data
CAN_MESSAGE data2message(uint32_t id, uint16_t uuid, uint8_t* data, uint8_t size) {

	CAN_MESSAGE message;

	message.id = id;
	message.uuid = uuid;

	for (uint8_t i = 0; i < size; i++) {
		message.data[i] = data[i];
	}

	message.size = size;
}