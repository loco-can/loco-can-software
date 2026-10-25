/*
 * Map CAN voltage / current frames onto a controller gauge output.
 *
 * Analog mode writes 0..255 PWM. Servo mode writes 0..180 degrees.
 * out_min / out_max are the output endpoints for a zero and full-scale
 * reading. out_max may sit below out_min to invert the needle.
 */
#pragma once

#ifndef CONTROLLER_GAUGE_H
#define CONTROLLER_GAUGE_H

#include <stdint.h>

#define CONTROLLER_GAUGE_ANALOG 0
#define CONTROLLER_GAUGE_SERVO 1

#define CONTROLLER_GAUGE_ANALOG_MAX 255
#define CONTROLLER_GAUGE_SERVO_MAX 180
#define CONTROLLER_GAUGE_TIMEOUT 2000

inline uint8_t controller_gauge_mode(uint8_t stored) {

	return stored == CONTROLLER_GAUGE_ANALOG ? CONTROLLER_GAUGE_ANALOG : CONTROLLER_GAUGE_SERVO;
}

inline uint16_t controller_gauge_default_max(uint8_t mode) {

	return controller_gauge_mode(mode) == CONTROLLER_GAUGE_ANALOG
		? (uint16_t)CONTROLLER_GAUGE_ANALOG_MAX
		: (uint16_t)CONTROLLER_GAUGE_SERVO_MAX;
}

inline uint16_t controller_gauge_reading(uint16_t raw, bool signed_abs) {

	if (!signed_abs) {
		return raw;
	}

	int16_t value = (int16_t)raw;
	if (value < 0) {
		value = (int16_t)(-value);
	}

	return (uint16_t)value;
}

inline uint16_t controller_gauge_output(uint16_t value, uint16_t full_scale, uint16_t out_min, uint16_t out_max) {

	if (full_scale == 0) {
		full_scale = 1023;
	}

	if (value > full_scale) {
		value = full_scale;
	}

	if (out_min == out_max) {
		return out_min;
	}

	int32_t span = (int32_t)out_max - (int32_t)out_min;
	int32_t out = (int32_t)out_min + (((int32_t)value * span) / (int32_t)full_scale);

	if (out < 0) {
		out = 0;
	}
	if (out > 65535) {
		out = 65535;
	}

	return (uint16_t)out;
}

inline bool controller_gauge_unpack(
	const uint8_t *data,
	uint8_t size,
	bool signed_abs,
	uint16_t &value,
	uint16_t &full_scale
) {

	if (data == 0 || size < 2) {
		return false;
	}

	uint16_t raw = (uint16_t)data[0] | ((uint16_t)data[1] << 8);
	value = controller_gauge_reading(raw, signed_abs);
	full_scale = 1023;

	if (size >= 4) {
		uint16_t reference = (uint16_t)data[2] | ((uint16_t)data[3] << 8);
		if (reference != 0) {
			full_scale = reference;
		}
	}

	return true;
}

#endif
