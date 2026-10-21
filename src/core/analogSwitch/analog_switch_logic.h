/*
 * Nearest programmed analog-switch point. Index 0 is the first stored
 * point, not the lowest voltage.
 */
#pragma once

#ifndef ANALOG_SWITCH_LOGIC_H
#define ANALOG_SWITCH_LOGIC_H

#include <stdint.h>

inline uint8_t analog_switch_nearest(const uint16_t *positions, uint8_t count, uint16_t analogval) {

	if (count == 0 || positions == 0) {
		return 0;
	}

	uint8_t nearest = 0;
	uint16_t best = 0xFFFF;

	for (uint8_t i = 0; i < count; i++) {
		uint16_t pos = positions[i];
		uint16_t dist = (analogval > pos) ? (uint16_t)(analogval - pos) : (uint16_t)(pos - analogval);

		if (dist < best) {
			best = dist;
			nearest = i;
		}
	}

	return nearest;
}

#endif
