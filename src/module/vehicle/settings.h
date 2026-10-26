/*
 * Vehicle-module parameters stored in EEPROM and edited over the CAN
 * settings protocol (doc/SETTINGS.md): 0x7FF request, 0x780 reply,
 * 0x700-0x77F write. The byte layout matches the controller module.
 *
 *   0      version code, major*10+minor (2.1.12 is stored as 21, not writable)
 *   1      plugin type (direct PWM driver, 4QD, Curtis)
 *   2      count of battery voltages to measure
 *   3      PWM mode: drive only, or drive and brake
 *   4      direction logic reversed by loco-setup
 *   5..6   minimum |motor voltage| that still allows a direction change
 *
 * CAN id 0x700 writes the module name. Ids 0x701.. write parameter bytes.
 */
#pragma once

#ifndef VEHICLE_SETTINGS_H
#define VEHICLE_SETTINGS_H

#include <stdint.h>

#include "../../core/can/can_message.h"
#include "../../can_protocol.h"

#define VEHICLE_PARAM_VERSION 0
#define VEHICLE_PARAM_PLUGIN 1
#define VEHICLE_PARAM_BATTERY_COUNT 2
#define VEHICLE_PARAM_PWM_MODE 3
#define VEHICLE_PARAM_REVERSE 4
#define VEHICLE_PARAM_VOLTAGE_MIN 5

#define VEHICLE_PARAM_BYTES 7
#define VEHICLE_SETTINGS_NAME_LEN 15

#define VEHICLE_SETTINGS_MAGIC 0xE4
#define VEHICLE_SETTINGS_SCHEMA 1

/* How many battery inputs the board can measure. */
#define VEHICLE_BATT_MAX 3

/* Fresh EEPROM image: one battery, both PWM outputs, direct plugin. */
#define VEHICLE_VOLTAGE_MIN_DEFAULT 16

struct VEHICLE_SETTINGS {
	uint8_t bytes[VEHICLE_PARAM_BYTES];
	char name[VEHICLE_SETTINGS_NAME_LEN + 1];
};

struct VEHICLE_SETTINGS_RESULT {
	uint8_t reply_count;
	CAN_MESSAGE reply[CAN_SETUP_REPLY_MAX];
	bool changed;
};

void vehicle_settings_defaults(VEHICLE_SETTINGS &settings, uint32_t module_version);

uint8_t vehicle_settings_plugin(const VEHICLE_SETTINGS &settings);
uint8_t vehicle_settings_battery_count(const VEHICLE_SETTINGS &settings);
uint8_t vehicle_settings_pwm_mode(const VEHICLE_SETTINGS &settings);
bool vehicle_settings_reversed(const VEHICLE_SETTINGS &settings);
uint16_t vehicle_settings_voltage_min(const VEHICLE_SETTINGS &settings);

void vehicle_settings_set_reversed(VEHICLE_SETTINGS &settings, bool reversed);

uint16_t vehicle_settings_get16(const VEHICLE_SETTINGS &settings, uint8_t index);
void vehicle_settings_put16(VEHICLE_SETTINGS &settings, uint8_t index, uint16_t value);

VEHICLE_SETTINGS_RESULT vehicle_settings_on_can(
	VEHICLE_SETTINGS &settings,
	const CAN_MESSAGE &message,
	uint16_t self_uuid,
	uint32_t module_version,
	uint8_t module_type
);

#endif
