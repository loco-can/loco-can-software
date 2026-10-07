/*
 * Switch parameters stored in EEPROM and edited over the CAN setup
 * protocol (0x7FF request, 0x780 reply, 0x700-0x77F write).
 *
 *   0      module version
 *   1..6   output function map, one byte per output
 *   7..8   max_current, little-endian milliamps
 *
 * CAN id 0x700 writes the module name. Ids 0x701.. write parameter bytes.
 * Each map byte is a function: a CAN message, a bit, and a direction gate.
 * See function.h. Unknown map bytes are rejected.
 */
#pragma once

#ifndef SWITCH_PARAMS_H
#define SWITCH_PARAMS_H

#include <stdint.h>

#include "current.h"
#include "function.h"

#define SWITCH_TYPE_ID 0x20

#define SWITCH_PARAM_VERSION 0
#define SWITCH_PARAM_MAP 1
#define SWITCH_PARAM_MAX_CURRENT (SWITCH_PARAM_MAP + SWITCH_OUTPUT_COUNT)
#define SWITCH_PARAM_BYTES (SWITCH_PARAM_MAX_CURRENT + 2)
#define SWITCH_PARAM_NAME_LEN 15

#define SWITCH_PARAM_MAGIC 0x53
#define SWITCH_PARAM_SCHEMA 2

struct SWITCH_PARAMS {
	uint8_t bytes[SWITCH_PARAM_BYTES];
	char name[SWITCH_PARAM_NAME_LEN + 1];
};

struct SWITCH_PARAM_RESULT {
	uint8_t reply_count;
	CAN_MESSAGE reply[2];
	bool changed;
};

void switch_params_defaults(SWITCH_PARAMS &params, uint8_t module_version);

uint8_t switch_params_map(const SWITCH_PARAMS &params, uint8_t output);

uint16_t switch_params_max_current(const SWITCH_PARAMS &params);
void switch_params_set_max_current(SWITCH_PARAMS &params, uint16_t milliamp);

SWITCH_PARAM_RESULT switch_params_on_can(
	SWITCH_PARAMS &params,
	const CAN_MESSAGE &message,
	uint16_t self_uuid,
	uint8_t module_version,
	uint8_t module_type
);

#endif
