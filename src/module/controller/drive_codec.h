/*
 * Pack and scale the controller drive / potentiometer values.
 * The byte layout follows the comment in can_protocol.h.
 */
#pragma once

#ifndef CONTROLLER_DRIVE_CODEC_H
#define CONTROLLER_DRIVE_CODEC_H

#include <stdint.h>

#include "../../can_protocol.h"
#include "status.h"

struct CONTROLLER_DRIVE {
	bool error;
	bool aux1;
	bool aux;
	bool down;
	bool up;
	bool reverse;
	bool motor;
	bool mains;
	bool multi;
	uint16_t loco;
	uint16_t drive;
	uint16_t power;
	uint16_t brake;
};

inline void controller_pack_drive(const CONTROLLER_DRIVE &src, uint8_t out[8]) {

	uint16_t drive = src.drive & 0x3FF;
	uint16_t power = src.power & 0x3FF;
	uint16_t brake = src.brake & 0x3FF;
	uint16_t loco = src.loco;

	for (uint8_t i = 0; i < 8; i++) {
		out[i] = 0;
	}

	if (src.error) out[0] |= (uint8_t)(1 << CONTROL_ERROR_FLAG);
	if (src.aux1) out[0] |= (uint8_t)(1 << CONTROL_AUX_1_FLAG);
	if (src.aux) out[0] |= (uint8_t)(1 << CONTROL_AUX_FLAG);
	if (src.down) out[0] |= (uint8_t)(1 << CONTROL_DOWN_FLAG);
	if (src.up) out[0] |= (uint8_t)(1 << CONTROL_UP_FLAG);
	if (src.reverse) out[0] |= (uint8_t)(1 << CONTROL_DIR_FLAG);
	if (src.motor) out[0] |= (uint8_t)(1 << CONTROL_DRIVE_FLAG);
	if (src.mains) out[0] |= (uint8_t)(1 << CONTROL_MAINS_FLAG);

	/*
	 * byte 2: multi, loco UUID bits 15..11, drive bits 9..8
	 * byte 4: loco UUID bits 10..6, power bits 9..8
	 * byte 6: loco UUID bits 5..0, brake bits 9..8
	 */
	out[2] = (uint8_t)((src.multi ? 0x80 : 0)
		| (((loco >> 11) & 0x1F) << 2)
		| ((drive >> 8) & 0x03));
	out[3] = (uint8_t)(drive & 0xFF);

	out[4] = (uint8_t)((((loco >> 6) & 0x1F) << 2) | ((power >> 8) & 0x03));
	out[5] = (uint8_t)(power & 0xFF);

	out[6] = (uint8_t)(((loco & 0x3F) << 2) | ((brake >> 8) & 0x03));
	out[7] = (uint8_t)(brake & 0xFF);
}

inline uint16_t controller_scale_10bit(uint16_t raw, uint16_t resolution) {

	if (resolution < 2) {
		return 0;
	}

	uint16_t max_in = (uint16_t)(resolution - 1);
	if (raw > max_in) {
		raw = max_in;
	}

	return (uint16_t)(((uint32_t)raw * 1023UL) / max_in);
}

/*
 * Map a raw ADC reading between a stored zero position and a stored full
 * position onto 0..1023. The full position may sit above or below zero.
 * Readings on the zero side of the lever, and the first
 * CONTROLLER_ZERO_THRESHOLD counts, stay at 0.
 */
inline uint16_t controller_map_axis(uint16_t raw, uint16_t zero, uint16_t full) {

	int32_t span = (int32_t)full - (int32_t)zero;
	int32_t delta = (int32_t)raw - (int32_t)zero;

	if (span == 0) {
		return 0;
	}

	if ((span > 0 && delta <= CONTROLLER_ZERO_THRESHOLD) || (span < 0 && delta >= -CONTROLLER_ZERO_THRESHOLD)) {
		return 0;
	}

	if ((span > 0 && delta < 0) || (span < 0 && delta > 0)) {
		return 0;
	}

	if (span < 0) {
		delta = -delta;
		span = -span;
	}

	if (delta >= span) {
		return 1023;
	}

	return (uint16_t)(((uint32_t)delta * 1023UL) / (uint32_t)span);
}


/*
 * One lever: center is zero, above center drives, below center brakes.
 * brake_full, zero and drive_full are the stored ADC positions.
 */
inline void controller_split_calibrated(uint16_t raw, uint16_t brake_full, uint16_t zero, uint16_t drive_full, uint16_t &drive, uint16_t &brake) {

	drive = controller_map_axis(raw, zero, drive_full);
	brake = controller_map_axis(raw, zero, brake_full);
}


/*
 * One lever: center is zero, above center drives, below center brakes.
 */
inline void controller_split_single_pot(uint16_t scaled, uint16_t &drive, uint16_t &brake) {

	const int16_t center = 512;
	int16_t delta = (int16_t)scaled - center;

	drive = 0;
	brake = 0;

	if (delta > CONTROLLER_ZERO_THRESHOLD) {
		uint32_t value = ((uint32_t)delta * 1023UL) / center;
		if (value > 1023) {
			value = 1023;
		}
		drive = (uint16_t)value;
	}
	else if (delta < -CONTROLLER_ZERO_THRESHOLD) {
		uint32_t value = ((uint32_t)(-delta) * 1023UL) / center;
		if (value > 1023) {
			value = 1023;
		}
		brake = (uint16_t)value;
	}
}

#endif
