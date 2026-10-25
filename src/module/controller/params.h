/*
 * Controller parameters stored in EEPROM and edited over the CAN setup
 * protocol (0x7FF request, 0x780 reply, 0x700-0x77F write).
 *
 * Byte 0 is the module version. The following bytes are the analog-switch
 * positions (count, then little-endian ADC points, index 0 = first
 * programmed point) and the drive/brake lever positions.
 *
 *   0      version code, major*10+minor (2.1.12 is stored as 21)
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
 *   45     battery gauge mode (0 analog PWM, 1 servo)
 *   46..47 battery gauge output min
 *   48..49 battery gauge output max
 *   50     motor-voltage gauge mode
 *   51..52 motor-voltage gauge output min
 *   53..54 motor-voltage gauge output max
 *   55     current gauge mode
 *   56..57 current gauge output min
 *   58..59 current gauge output max
 *
 * Live ADC (GET 0x7FF UUID index, not stored):
 *   60 mains, 62 dir, 64 light, 66 light2, 68 drive, 70 brake
 *
 * Live gauge preview (GET/SET, not stored). Moves the output in
 * module setup so the programmer slider can find min/max:
 *   72 battery, 74 motor voltage, 76 current
 *
 * CAN id 0x700 writes the module name. Ids 0x701.. write parameter bytes.
 */
#pragma once

#ifndef CONTROLLER_PARAMS_H
#define CONTROLLER_PARAMS_H

#include <stdint.h>

#include "../../core/can/can_message.h"
#include "../../can_protocol.h"

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

#define CONTROLLER_PARAM_GAUGE_BATT_MODE 45
#define CONTROLLER_PARAM_GAUGE_BATT_MIN 46
#define CONTROLLER_PARAM_GAUGE_BATT_MAX 48
#define CONTROLLER_PARAM_GAUGE_MOTOR_MODE 50
#define CONTROLLER_PARAM_GAUGE_MOTOR_MIN 51
#define CONTROLLER_PARAM_GAUGE_MOTOR_MAX 53
#define CONTROLLER_PARAM_GAUGE_CURRENT_MODE 55
#define CONTROLLER_PARAM_GAUGE_CURRENT_MIN 56
#define CONTROLLER_PARAM_GAUGE_CURRENT_MAX 58

#define CONTROLLER_PARAM_BYTES_V1 45
#define CONTROLLER_PARAM_BYTES 60
#define CONTROLLER_PARAM_NAME_LEN 15

/* Live ADC (GET only, not EEPROM). Programmer capture uses these. */
#define CONTROLLER_PARAM_LIVE_MAINS 60
#define CONTROLLER_PARAM_LIVE_DIR 62
#define CONTROLLER_PARAM_LIVE_LIGHT 64
#define CONTROLLER_PARAM_LIVE_LIGHT2 66
#define CONTROLLER_PARAM_LIVE_DRIVE 68
#define CONTROLLER_PARAM_LIVE_BRAKE 70
#define CONTROLLER_PARAM_LIVE_GAUGE_BATT 72
#define CONTROLLER_PARAM_LIVE_GAUGE_MOTOR 74
#define CONTROLLER_PARAM_LIVE_GAUGE_CURRENT 76

#define CONTROLLER_PARAM_MAGIC 0xC3
#define CONTROLLER_PARAM_SCHEMA 2
#define CONTROLLER_PARAM_SCHEMA_V1 1

struct CONTROLLER_PARAMS {
	uint8_t bytes[CONTROLLER_PARAM_BYTES];
	char name[CONTROLLER_PARAM_NAME_LEN + 1];
};

struct CONTROLLER_PARAM_RESULT {
	uint8_t reply_count;
	CAN_MESSAGE reply[CAN_SETUP_REPLY_MAX];
	bool changed;
};

void controller_params_defaults(CONTROLLER_PARAMS &params, uint16_t resolution, bool dual_brake, uint32_t module_version);
void controller_params_init_levers(CONTROLLER_PARAMS &params, uint16_t resolution, bool dual_brake);
void controller_params_init_gauges(CONTROLLER_PARAMS &params);
bool controller_params_levers_look_dual(const CONTROLLER_PARAMS &params, uint16_t resolution);

uint16_t controller_params_get16(const CONTROLLER_PARAMS &params, uint8_t index);
void controller_params_put16(CONTROLLER_PARAMS &params, uint8_t index, uint16_t value);

/* copy up to max points; returns the stored count */
uint8_t controller_params_points(const CONTROLLER_PARAMS &params, uint8_t count_index, uint8_t max_points, uint16_t *out);

/* 0=mains 1=dir 2=light 3=light2 4=drive 5=brake, or -1 */
int8_t controller_params_live_channel(uint8_t index);

/* mode-byte index of the previewed gauge, or 0 */
uint8_t controller_params_live_gauge_mode(uint8_t index);

CONTROLLER_PARAM_RESULT controller_params_on_can(
	CONTROLLER_PARAMS &params,
	const CAN_MESSAGE &message,
	uint16_t self_uuid,
	uint32_t module_version,
	uint8_t module_type
);

#endif
