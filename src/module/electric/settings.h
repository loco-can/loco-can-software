/*
 * Electric-module parameters stored in EEPROM and edited over the CAN
 * settings protocol (doc/SETTINGS.md): 0x7FF request, 0x780 reply,
 * 0x700-0x77F write. The byte layout matches the controller module.
 *
 *   0      version (running firmware, not writable)
 *   1      plugin type (direct PWM driver, 4QD, Curtis)
 *   2      count of battery voltages to measure
 *   3      PWM mode: drive only, or drive and brake
 *   4      direction logic reversed by loco-setup
 *   5..6   minimum |motor voltage| that still allows a direction change
 *
 * CAN id 0x700 writes the module name. Ids 0x701.. write parameter bytes.
 */
#pragma once

#ifndef ELECTRIC_SETTINGS_H
#define ELECTRIC_SETTINGS_H

#include <stdint.h>

#include "../../core/can/can_message.h"

/* Actuator group, type 0 (doc/SETTINGS.md). */
#define ELECTRIC_TYPE_ID 0x20

#define ELECTRIC_PARAM_VERSION 0
#define ELECTRIC_PARAM_PLUGIN 1
#define ELECTRIC_PARAM_BATTERY_COUNT 2
#define ELECTRIC_PARAM_PWM_MODE 3
#define ELECTRIC_PARAM_REVERSE 4
#define ELECTRIC_PARAM_VOLTAGE_MIN 5

#define ELECTRIC_PARAM_BYTES 7
#define ELECTRIC_SETTINGS_NAME_LEN 15

#define ELECTRIC_SETTINGS_MAGIC 0xE4
#define ELECTRIC_SETTINGS_SCHEMA 1

/* How many battery inputs the board can measure. */
#define ELECTRIC_BATT_MAX 3

/* Fresh EEPROM image: one battery, both PWM outputs, direct plugin. */
#define ELECTRIC_VOLTAGE_MIN_DEFAULT 16

struct ELECTRIC_SETTINGS {
	uint8_t bytes[ELECTRIC_PARAM_BYTES];
	char name[ELECTRIC_SETTINGS_NAME_LEN + 1];
};

struct ELECTRIC_SETTINGS_RESULT {
	uint8_t reply_count;
	CAN_MESSAGE reply[2];
	bool changed;
};

void electric_settings_defaults(ELECTRIC_SETTINGS &settings, uint8_t module_version);

uint8_t electric_settings_plugin(const ELECTRIC_SETTINGS &settings);
uint8_t electric_settings_battery_count(const ELECTRIC_SETTINGS &settings);
uint8_t electric_settings_pwm_mode(const ELECTRIC_SETTINGS &settings);
bool electric_settings_reversed(const ELECTRIC_SETTINGS &settings);
uint16_t electric_settings_voltage_min(const ELECTRIC_SETTINGS &settings);

void electric_settings_set_reversed(ELECTRIC_SETTINGS &settings, bool reversed);

uint16_t electric_settings_get16(const ELECTRIC_SETTINGS &settings, uint8_t index);
void electric_settings_put16(ELECTRIC_SETTINGS &settings, uint8_t index, uint16_t value);

ELECTRIC_SETTINGS_RESULT electric_settings_on_can(
	ELECTRIC_SETTINGS &settings,
	const CAN_MESSAGE &message,
	uint16_t self_uuid,
	uint8_t module_version,
	uint8_t module_type
);

#endif
