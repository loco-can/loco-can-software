/*
 * Servo output mapping.
 *
 * Each output is stored as one EEPROM byte that selects which analog
 * field of which CAN message drives that servo.
 *
 *   bits 0-3  source (see SERVO_SRC_*)
 *   bit  7    invert: 1023 - value
 *
 * Analog frames (current, voltage, speed, tacho) carry a little-endian
 * 16-bit value in bytes 0-1 and an optional full-scale in bytes 2-3.
 * Drive frames carry 10-bit throttle, power and brake plus the
 * direction flag.
 */
#pragma once

#ifndef SERVO_MAPPING_H
#define SERVO_MAPPING_H

#include <stdint.h>

#include "../../can_protocol.h"
#include "../../core/can/can_message.h"

#define SERVO_OUTPUT_COUNT 4
#define SERVO_ANALOG_MAX 1023

#define SERVO_SRC_NONE 0
#define SERVO_SRC_DRIVE 1
#define SERVO_SRC_BRAKE 2
#define SERVO_SRC_POWER 3
#define SERVO_SRC_DIR 4
#define SERVO_SRC_SPEED 5
#define SERVO_SRC_TACHO 6
#define SERVO_SRC_MODULE_CURRENT 7
#define SERVO_SRC_MOTOR_CURRENT 8
#define SERVO_SRC_BATT_CURRENT 9
#define SERVO_SRC_VOLTAGE 10
#define SERVO_SRC_MOTOR_VOLTAGE 11
#define SERVO_SRC_BATT_VOLTAGE 12
#define SERVO_SRC_MAX SERVO_SRC_BATT_VOLTAGE

#define SERVO_MAP_SRC_MASK 0x0F
#define SERVO_MAP_INVERT 0x80

constexpr uint8_t servo_map_make(uint8_t source, bool invert) {
	return (uint8_t)((source & SERVO_MAP_SRC_MASK) | (invert ? SERVO_MAP_INVERT : 0));
}

constexpr uint8_t SERVO_FUNC_NONE = servo_map_make(SERVO_SRC_NONE, false);
constexpr uint8_t SERVO_FUNC_DRIVE = servo_map_make(SERVO_SRC_DRIVE, false);
constexpr uint8_t SERVO_FUNC_BRAKE = servo_map_make(SERVO_SRC_BRAKE, false);
constexpr uint8_t SERVO_FUNC_POWER = servo_map_make(SERVO_SRC_POWER, false);

constexpr uint8_t SERVO_DEFAULT_MAP[SERVO_OUTPUT_COUNT] = {
	SERVO_FUNC_DRIVE,
	SERVO_FUNC_BRAKE,
	SERVO_FUNC_POWER,
	SERVO_FUNC_NONE
};

inline uint8_t servo_map_source(uint8_t map) {
	return (uint8_t)(map & SERVO_MAP_SRC_MASK);
}

inline bool servo_map_invert(uint8_t map) {
	return (map & SERVO_MAP_INVERT) != 0;
}

inline bool servo_map_valid(uint8_t map) {
	return servo_map_source(map) <= SERVO_SRC_MAX;
}

struct SERVO_BUS {
	uint16_t drive;
	uint16_t brake;
	uint16_t power;
	uint16_t dir;
	uint16_t speed;
	uint16_t tacho;
	uint16_t module_current;
	uint16_t motor_current;
	uint16_t batt_current;
	uint16_t voltage;
	uint16_t motor_voltage;
	uint16_t batt_voltage;
	bool alive;
};

void servo_bus_clear(SERVO_BUS &bus);
void servo_bus_apply(SERVO_BUS &bus, const CAN_MESSAGE &message);

uint16_t servo_analog_scale(uint16_t value, uint16_t full_scale);
uint16_t servo_output_value(uint8_t map, const SERVO_BUS &bus);

#endif
