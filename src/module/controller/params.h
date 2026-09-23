/*
 * Controller parameters stored in EEPROM and edited over the CAN setup
 * protocol (0x7FF request, 0x780 reply, 0x700-0x77F write).
 *
 * Byte 0 is the module version. The following bytes are the analog-switch
 * positions (count, then little-endian ADC points, index 0 = lowest
 * voltage) and the drive/brake lever positions.
 *
 *   0      version
 *   1      mains count
 *   2..7   mains points (3 x uint16)
 *   8      direction count
 *   9..14  direction points (3 x uint16)
 *   15     light count
 *   16..25 light points (5 x uint16)
 *   26     second light count
 *   27..36 second light points (5 x uint16)
 *   37..38 drive zero
 *   39..40 drive full
 *   41..42 brake zero
 *   43..44 brake full
 *
 * CAN id 0x700 writes the module name. Ids 0x701.. write parameter bytes.
 */
#pragma once

#ifndef CONTROLLER_PARAMS_H
#define CONTROLLER_PARAMS_H

#include <stdint.h>

#include "../../core/can/can_message.h"

#define CONTROLLER_TYPE_ID 0x10

#define CONTROLLER_PARAM_VERSION 0
#define CONTROLLER_PARAM_MAINS_COUNT 1
#define CONTROLLER_PARAM_MAINS_POINT 2
#define CONTROLLER_PARAM_MAINS_POINTS 3

#define CONTROLLER_PARAM_DIR_COUNT 8
#define CONTROLLER_PARAM_DIR_POINT 9
#define CONTROLLER_PARAM_DIR_POINTS 3

#define CONTROLLER_PARAM_LIGHT_COUNT 15
#define CONTROLLER_PARAM_LIGHT_POINT 16
#define CONTROLLER_PARAM_LIGHT_POINTS 5

#define CONTROLLER_PARAM_LIGHT2_COUNT 26
#define CONTROLLER_PARAM_LIGHT2_POINT 27
#define CONTROLLER_PARAM_LIGHT2_POINTS 5

#define CONTROLLER_PARAM_DRIVE_ZERO 37
#define CONTROLLER_PARAM_DRIVE_FULL 39
#define CONTROLLER_PARAM_BRAKE_ZERO 41
#define CONTROLLER_PARAM_BRAKE_FULL 43

#define CONTROLLER_PARAM_BYTES 45
#define CONTROLLER_PARAM_NAME_LEN 15

#define CONTROLLER_PARAM_MAGIC 0xC3
#define CONTROLLER_PARAM_SCHEMA 1

struct CONTROLLER_PARAMS {
	uint8_t bytes[CONTROLLER_PARAM_BYTES];
	char name[CONTROLLER_PARAM_NAME_LEN + 1];
};

struct CONTROLLER_PARAM_RESULT {
	uint8_t reply_count;
	CAN_MESSAGE reply[2];
	bool changed;
};

void controller_params_defaults(CONTROLLER_PARAMS &params, uint16_t resolution, bool dual_brake, uint8_t module_version);

uint16_t controller_params_get16(const CONTROLLER_PARAMS &params, uint8_t index);
void controller_params_put16(CONTROLLER_PARAMS &params, uint8_t index, uint16_t value);

/* copy up to max points; returns the stored count */
uint8_t controller_params_points(const CONTROLLER_PARAMS &params, uint8_t count_index, uint8_t max_points, uint16_t *out);

CONTROLLER_PARAM_RESULT controller_params_on_can(
	CONTROLLER_PARAMS &params,
	const CAN_MESSAGE &message,
	uint16_t self_uuid,
	uint8_t module_version,
	uint8_t module_type
);

#endif
