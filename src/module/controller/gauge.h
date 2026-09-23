/*
 * Controller gauges.
 *
 * A measurement frame follows INTELLIVALUE: 11-bit percentage in 0.1 %
 * steps (100 % = 1000) and a 16-bit big-endian reference in the same
 * precision the sender stored (12.000 V is 12000). Absolute value is
 * percentage * reference / 1000.
 *
 * Several vehicles can report the same quantity. Battery voltage keeps the
 * lowest percentage (lowest state of charge). Motor voltage and current
 * keep the highest percentage. The needle is that absolute value scaled to
 * the gauge full-scale reference, as an analog duty or a servo angle.
 */
#pragma once

#ifndef CONTROLLER_GAUGE_H
#define CONTROLLER_GAUGE_H

#include <stdint.h>

#include "../../can_protocol.h"
#include "params.h"

#define CONTROLLER_GAUGE_BATT_VOLTAGE 0
#define CONTROLLER_GAUGE_MOTOR_VOLTAGE 1
#define CONTROLLER_GAUGE_CURRENT 2
#define CONTROLLER_GAUGE_MOTOR_CURRENT 3
#define CONTROLLER_GAUGE_COUNT 4
#define CONTROLLER_GAUGE_NONE 0xFF

#define CONTROLLER_GAUGE_REDUCE_MIN 0
#define CONTROLLER_GAUGE_REDUCE_MAX 1

#define CONTROLLER_GAUGE_ANALOG_FULL 255
#define CONTROLLER_GAUGE_SERVO_FULL 180

#define CONTROLLER_GAUGE_PERCENT_FULL 1000

/* Senders remembered per gauge. A newer frame replaces the oldest. */
#define CONTROLLER_GAUGE_SLOTS 4

#if defined(CONTROLLER_BATTERY_VOLTAGE_PORT) || defined(CONTROLLER_MOTOR_VOLTAGE_PORT) || defined(CONTROLLER_CURRENT_PORT) || defined(CONTROLLER_BATTERY_CURRENT_PORT) || defined(CONTROLLER_MOTOR_CURRENT_PORT)
#define CONTROLLER_HAS_GAUGES 1

enum {
#ifdef CONTROLLER_BATTERY_VOLTAGE_PORT
	CONTROLLER_GAUGE_ROW_BATT_VOLTAGE,
#endif
#ifdef CONTROLLER_MOTOR_VOLTAGE_PORT
	CONTROLLER_GAUGE_ROW_MOTOR_VOLTAGE,
#endif
#if defined(CONTROLLER_CURRENT_PORT) || defined(CONTROLLER_BATTERY_CURRENT_PORT)
	CONTROLLER_GAUGE_ROW_CURRENT,
#endif
#ifdef CONTROLLER_MOTOR_CURRENT_PORT
	CONTROLLER_GAUGE_ROW_MOTOR_CURRENT,
#endif
	CONTROLLER_GAUGE_ROWS
};
#endif

struct CONTROLLER_GAUGE_SAMPLE {
	uint16_t uuid;
	uint16_t percentage;
	uint16_t reference;
	uint16_t seen;
};

inline uint8_t controller_gauge_mode_index(uint8_t channel) {
	return (uint8_t)(CONTROLLER_PARAM_GAUGE_BATT_MODE + (uint8_t)(channel * CONTROLLER_PARAM_GAUGE_STRIDE));
}

inline uint8_t controller_gauge_ref_index(uint8_t channel) {
	return (uint8_t)(controller_gauge_mode_index(channel) + 1);
}

inline uint8_t controller_gauge_channel(uint32_t id) {

	switch (id) {
		case CAN_ID_VOLTAGE:
		case CAN_ID_BATT_VOLTAGE:
		case CAN_ID_BATT_1_VOLTAGE:
		case CAN_ID_BATT_2_VOLTAGE:
			return CONTROLLER_GAUGE_BATT_VOLTAGE;
		case CAN_ID_MOTOR_VOLTAGE:
			return CONTROLLER_GAUGE_MOTOR_VOLTAGE;
		case CAN_ID_CURRENT:
		case CAN_ID_BATT_CURRENT:
			return CONTROLLER_GAUGE_CURRENT;
		case CAN_ID_MOTOR_CURRENT:
			return CONTROLLER_GAUGE_MOTOR_CURRENT;
		default:
			return CONTROLLER_GAUGE_NONE;
	}
}

inline uint8_t controller_gauge_row(uint8_t channel) {

	switch (channel) {
#ifdef CONTROLLER_BATTERY_VOLTAGE_PORT
		case CONTROLLER_GAUGE_BATT_VOLTAGE:
			return CONTROLLER_GAUGE_ROW_BATT_VOLTAGE;
#endif
#ifdef CONTROLLER_MOTOR_VOLTAGE_PORT
		case CONTROLLER_GAUGE_MOTOR_VOLTAGE:
			return CONTROLLER_GAUGE_ROW_MOTOR_VOLTAGE;
#endif
#if defined(CONTROLLER_CURRENT_PORT) || defined(CONTROLLER_BATTERY_CURRENT_PORT)
		case CONTROLLER_GAUGE_CURRENT:
			return CONTROLLER_GAUGE_ROW_CURRENT;
#endif
#ifdef CONTROLLER_MOTOR_CURRENT_PORT
		case CONTROLLER_GAUGE_MOTOR_CURRENT:
			return CONTROLLER_GAUGE_ROW_MOTOR_CURRENT;
#endif
		default:
			return CONTROLLER_GAUGE_NONE;
	}
}

inline uint8_t controller_gauge_reduce(uint8_t channel) {

	if (channel == CONTROLLER_GAUGE_BATT_VOLTAGE) {
		return CONTROLLER_GAUGE_REDUCE_MIN;
	}

	return CONTROLLER_GAUGE_REDUCE_MAX;
}

inline bool controller_gauge_decode(const uint8_t *data, uint8_t size, uint16_t &percentage, uint16_t &reference, uint8_t &index) {

	if (data == 0 || size < 4) {
		return false;
	}

	index = (uint8_t)(data[0] >> 5);
	percentage = (uint16_t)data[1] | ((uint16_t)(data[0] & 0x07) << 8);
	reference = ((uint16_t)data[2] << 8) | data[3];
	return true;
}

inline uint16_t controller_gauge_output(uint16_t percentage, uint16_t source_reference, uint16_t gauge_reference, uint16_t full_scale) {

	if (gauge_reference == 0 || full_scale == 0) {
		return 0;
	}

	uint32_t absolute = ((uint32_t)percentage * (uint32_t)source_reference) / CONTROLLER_GAUGE_PERCENT_FULL;
	uint32_t scaled = (absolute * full_scale) / gauge_reference;

	if (scaled > full_scale) {
		scaled = full_scale;
	}

	return (uint16_t)scaled;
}

inline void controller_gauge_expire(CONTROLLER_GAUGE_SAMPLE *slots, uint8_t count, uint16_t now, uint16_t timeout) {

	if (slots == 0) {
		return;
	}

	for (uint8_t i = 0; i < count; i++) {
		if (slots[i].uuid == 0) {
			continue;
		}
		if ((uint16_t)(now - slots[i].seen) > timeout) {
			slots[i].uuid = 0;
			slots[i].percentage = 0;
			slots[i].reference = 0;
			slots[i].seen = 0;
		}
	}
}

inline void controller_gauge_note(CONTROLLER_GAUGE_SAMPLE *slots, uint8_t count, uint16_t uuid, uint16_t percentage, uint16_t reference, uint16_t now, uint16_t timeout) {

	if (slots == 0 || count == 0 || uuid == 0) {
		return;
	}

	controller_gauge_expire(slots, count, now, timeout);

	uint8_t slot = 0;
	bool found = false;
	uint16_t oldest_age = 0;

	for (uint8_t i = 0; i < count; i++) {
		if (slots[i].uuid == uuid) {
			slot = i;
			found = true;
			break;
		}
	}

	if (!found) {
		for (uint8_t i = 0; i < count; i++) {
			if (slots[i].uuid == 0) {
				slot = i;
				found = true;
				break;
			}
			uint16_t age = (uint16_t)(now - slots[i].seen);
			if (!found || age >= oldest_age) {
				oldest_age = age;
				slot = i;
				found = true;
			}
		}
	}

	slots[slot].uuid = uuid;
	slots[slot].percentage = percentage;
	slots[slot].reference = reference;
	slots[slot].seen = now;
}

inline bool controller_gauge_select(CONTROLLER_GAUGE_SAMPLE *slots, uint8_t count, uint8_t reduce, uint16_t now, uint16_t timeout, uint16_t &percentage, uint16_t &reference) {

	controller_gauge_expire(slots, count, now, timeout);

	bool any = false;
	percentage = 0;
	reference = 0;

	for (uint8_t i = 0; i < count; i++) {
		if (slots[i].uuid == 0) {
			continue;
		}
		if (!any || (reduce == CONTROLLER_GAUGE_REDUCE_MIN && slots[i].percentage < percentage) || (reduce == CONTROLLER_GAUGE_REDUCE_MAX && slots[i].percentage > percentage)) {
			any = true;
			percentage = slots[i].percentage;
			reference = slots[i].reference;
		}
	}

	return any;
}

#endif
