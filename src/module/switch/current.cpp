/*
 * Scale, compare, and pack the switch module current.
 */

#include "current.h"


uint16_t switch_current_from_adc(uint16_t raw, uint16_t resolution, uint16_t full_scale_ma) {

	if (resolution < 2 || full_scale_ma == 0) {
		return 0;
	}

	uint16_t max_in = (uint16_t)(resolution - 1);
	if (raw > max_in) {
		raw = max_in;
	}

	return (uint16_t)(((uint32_t)raw * full_scale_ma) / max_in);
}


bool switch_current_over(uint16_t milliamp, uint16_t max_milliamp) {

	return milliamp > max_milliamp;
}


void switch_current_pack(uint16_t milliamp, uint16_t max_milliamp, uint8_t out[SWITCH_CURRENT_FRAME]) {

	uint16_t percentage = 0;

	if (max_milliamp > 0) {
		uint32_t scaled = ((uint32_t)milliamp * SWITCH_CURRENT_PERCENT_FULL) / max_milliamp;
		if (scaled > SWITCH_CURRENT_PERCENT_MAX) {
			scaled = SWITCH_CURRENT_PERCENT_MAX;
		}
		percentage = (uint16_t)scaled;
	}

	out[0] = (uint8_t)((percentage >> 8) & 0x07);
	out[1] = (uint8_t)(percentage & 0xFF);
	out[2] = (uint8_t)((max_milliamp >> 8) & 0xFF);
	out[3] = (uint8_t)(max_milliamp & 0xFF);
}


uint16_t switch_current_unpack(const uint8_t data[SWITCH_CURRENT_FRAME]) {

	uint16_t percentage = (uint16_t)(((uint16_t)(data[0] & 0x07) << 8) | data[1]);
	uint16_t reference = (uint16_t)(((uint16_t)data[2] << 8) | data[3]);

	return (uint16_t)(((uint32_t)percentage * reference) / SWITCH_CURRENT_PERCENT_FULL);
}
