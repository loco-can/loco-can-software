/*
 * Servo parameters stored in EEPROM and edited over the CAN setup
 * protocol (0x7FF request, 0x780 reply, 0x700-0x77F write).
 *
 *   0      version code, major*10+minor (2.1.12 is stored as 21)
 *   1..4   output map, one byte per servo (see mapping.h)
 *
 * CAN id 0x700 writes the module name. Ids 0x701.. write parameter bytes.
 */
#pragma once

#ifndef SERVO_PARAMS_H
#define SERVO_PARAMS_H

#include <stdint.h>

#include "mapping.h"

#include "../../can_protocol.h"

#define SERVO_PARAM_VERSION 0
#define SERVO_PARAM_MAP 1
#define SERVO_PARAM_BYTES (SERVO_PARAM_MAP + SERVO_OUTPUT_COUNT)
#define SERVO_PARAM_NAME_LEN 15

#define SERVO_PARAM_MAGIC 0x5E
#define SERVO_PARAM_SCHEMA 1

struct SERVO_PARAMS {
	uint8_t bytes[SERVO_PARAM_BYTES];
	char name[SERVO_PARAM_NAME_LEN + 1];
};

struct SERVO_PARAM_RESULT {
	uint8_t reply_count;
	CAN_MESSAGE reply[CAN_SETUP_REPLY_MAX];
	bool changed;
};

void servo_params_defaults(SERVO_PARAMS &params, uint32_t module_version);

uint8_t servo_params_map(const SERVO_PARAMS &params, uint8_t output);

SERVO_PARAM_RESULT servo_params_on_can(
	SERVO_PARAMS &params,
	const CAN_MESSAGE &message,
	uint16_t self_uuid,
	uint32_t module_version,
	uint8_t module_type
);

#endif
