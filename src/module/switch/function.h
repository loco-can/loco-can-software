/*
 * Switch output functions.
 *
 * Each output is stored as one byte that selects a bit of a CAN
 * message and, for the end lamps, which direction lights it.
 *
 *   bits 0-2  bit in the message data byte
 *   bits 3-4  message: 0 none, 1 light, 2 signal, 3 drive
 *   bits 5-7  direction gate: 0 any, 1 forward, 2 reverse
 *
 * light_low_* follows LIGHT_LOW on the leading end.
 * light_back_* follows LIGHT_BACK on the trailing end.
 * horn_low / horn_high follow SIGNAL_LOW / SIGNAL_HIGH.
 *
 * Direction is CONTROL_DIR_FLAG in the drive frame (0 = forward).
 */
#pragma once

#ifndef SWITCH_FUNCTION_H
#define SWITCH_FUNCTION_H

#include <stdint.h>

#include "../../can_protocol.h"
#include "../../core/can/can_message.h"

#define SWITCH_OUTPUT_COUNT 6

#define SWITCH_MSG_NONE 0
#define SWITCH_MSG_LIGHT 1
#define SWITCH_MSG_SIGNAL 2
#define SWITCH_MSG_DRIVE 3

#define SWITCH_DIR_ANY 0
#define SWITCH_DIR_FORWARD 1
#define SWITCH_DIR_REVERSE 2

#define SWITCH_MAP_BIT_MASK 0x07
#define SWITCH_MAP_MSG_SHIFT 3
#define SWITCH_MAP_DIR_SHIFT 5

constexpr uint8_t switch_map_make(uint8_t message, uint8_t bit, uint8_t dir) {
	return (uint8_t)(((dir & 0x07) << SWITCH_MAP_DIR_SHIFT)
		| ((message & 0x03) << SWITCH_MAP_MSG_SHIFT)
		| (bit & SWITCH_MAP_BIT_MASK));
}

constexpr uint8_t SWITCH_FUNC_NONE = switch_map_make(SWITCH_MSG_NONE, 0, SWITCH_DIR_ANY);

constexpr uint8_t SWITCH_FUNC_LIGHT_LOW_FRONT = switch_map_make(SWITCH_MSG_LIGHT, LIGHT_LOW, SWITCH_DIR_FORWARD);
constexpr uint8_t SWITCH_FUNC_LIGHT_LOW_BACK = switch_map_make(SWITCH_MSG_LIGHT, LIGHT_LOW, SWITCH_DIR_REVERSE);
constexpr uint8_t SWITCH_FUNC_LIGHT_BACK_FRONT = switch_map_make(SWITCH_MSG_LIGHT, LIGHT_BACK, SWITCH_DIR_REVERSE);
constexpr uint8_t SWITCH_FUNC_LIGHT_BACK_BACK = switch_map_make(SWITCH_MSG_LIGHT, LIGHT_BACK, SWITCH_DIR_FORWARD);

constexpr uint8_t SWITCH_FUNC_LIGHT_HIGH_FRONT = switch_map_make(SWITCH_MSG_LIGHT, LIGHT_HIGH, SWITCH_DIR_FORWARD);
constexpr uint8_t SWITCH_FUNC_LIGHT_HIGH_BACK = switch_map_make(SWITCH_MSG_LIGHT, LIGHT_HIGH, SWITCH_DIR_REVERSE);
constexpr uint8_t SWITCH_FUNC_LIGHT_POSIT_FRONT = switch_map_make(SWITCH_MSG_LIGHT, LIGHT_POSIT, SWITCH_DIR_FORWARD);
constexpr uint8_t SWITCH_FUNC_LIGHT_POSIT_BACK = switch_map_make(SWITCH_MSG_LIGHT, LIGHT_POSIT, SWITCH_DIR_REVERSE);

constexpr uint8_t SWITCH_FUNC_HORN_LOW = switch_map_make(SWITCH_MSG_SIGNAL, SIGNAL_LOW, SWITCH_DIR_ANY);
constexpr uint8_t SWITCH_FUNC_HORN_HIGH = switch_map_make(SWITCH_MSG_SIGNAL, SIGNAL_HIGH, SWITCH_DIR_ANY);
constexpr uint8_t SWITCH_FUNC_SIGNAL_BELL = switch_map_make(SWITCH_MSG_SIGNAL, SIGNAL_BELL, SWITCH_DIR_ANY);

constexpr uint8_t SWITCH_DEFAULT_FUNCTION[SWITCH_OUTPUT_COUNT] = {
	SWITCH_FUNC_LIGHT_LOW_FRONT,
	SWITCH_FUNC_LIGHT_LOW_BACK,
	SWITCH_FUNC_LIGHT_BACK_FRONT,
	SWITCH_FUNC_LIGHT_BACK_BACK,
	SWITCH_FUNC_HORN_LOW,
	SWITCH_FUNC_HORN_HIGH
};

inline uint8_t switch_map_message(uint8_t map) {
	return (uint8_t)((map >> SWITCH_MAP_MSG_SHIFT) & 0x03);
}

inline uint8_t switch_map_bit(uint8_t map) {
	return (uint8_t)(map & SWITCH_MAP_BIT_MASK);
}

inline uint8_t switch_map_dir(uint8_t map) {
	return (uint8_t)((map >> SWITCH_MAP_DIR_SHIFT) & 0x07);
}

inline bool switch_map_valid(uint8_t map) {
	return switch_map_message(map) <= SWITCH_MSG_DRIVE && switch_map_dir(map) <= SWITCH_DIR_REVERSE;
}

struct SWITCH_BUS {
	uint8_t light;
	uint8_t signal;
	uint8_t drive;
	bool forward;
	bool alive;
};

void switch_bus_clear(SWITCH_BUS &bus);
void switch_bus_apply(SWITCH_BUS &bus, const CAN_MESSAGE &message);

/* true when this mapped output should be driven */
bool switch_output_level(uint8_t map, const SWITCH_BUS &bus);

/* bit 0 is output 0 */
uint8_t switch_output_mask(const uint8_t *maps, uint8_t count, const SWITCH_BUS &bus);

#endif
