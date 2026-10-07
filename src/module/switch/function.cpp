/*
 * Evaluate a switch output mapping against the latest CAN bits.
 */

#include "function.h"


void switch_bus_clear(SWITCH_BUS &bus) {

	bus.light = 0;
	bus.signal = 0;
	bus.drive = 0;
	bus.forward = true;
	bus.alive = false;
}


void switch_bus_apply(SWITCH_BUS &bus, const CAN_MESSAGE &message) {

	if (message.id == CAN_ID_EMERGENCY) {
		bus.light = 0;
		bus.signal = 0;
		bus.drive = 0;
		bus.forward = true;
		bus.alive = false;
		return;
	}

	if (message.size < 1) {
		return;
	}

	switch (message.id) {

		case CAN_ID_LIGHT:
			bus.light = message.data[0];
			bus.alive = true;
			break;

		case CAN_ID_SIGNAL:
			bus.signal = message.data[0];
			bus.alive = true;
			break;

		case CAN_ID_DRIVE:
			bus.drive = message.data[0];
			bus.forward = (message.data[0] & (uint8_t)(1 << CONTROL_DIR_FLAG)) == 0;
			bus.alive = true;
			break;

		default:
			break;
	}
}


bool switch_output_level(uint8_t map, const SWITCH_BUS &bus) {

	uint8_t message;
	uint8_t dir;
	uint8_t data;

	if (!bus.alive || !switch_map_valid(map)) {
		return false;
	}

	message = switch_map_message(map);
	dir = switch_map_dir(map);

	if (message == SWITCH_MSG_NONE) {
		return false;
	}

	if (dir == SWITCH_DIR_FORWARD && !bus.forward) {
		return false;
	}
	if (dir == SWITCH_DIR_REVERSE && bus.forward) {
		return false;
	}

	data = 0;
	if (message == SWITCH_MSG_LIGHT) {
		data = bus.light;
	}
	else if (message == SWITCH_MSG_SIGNAL) {
		data = bus.signal;
	}
	else if (message == SWITCH_MSG_DRIVE) {
		data = bus.drive;
	}

	return (data & (uint8_t)(1 << switch_map_bit(map))) != 0;
}


uint8_t switch_output_mask(const uint8_t *maps, uint8_t count, const SWITCH_BUS &bus) {

	uint8_t mask = 0;

	if (maps == 0) {
		return 0;
	}

	if (count > SWITCH_OUTPUT_COUNT) {
		count = SWITCH_OUTPUT_COUNT;
	}

	for (uint8_t i = 0; i < count; i++) {
		if (switch_output_level(maps[i], bus)) {
			mask |= (uint8_t)(1 << i);
		}
	}

	return mask;
}
