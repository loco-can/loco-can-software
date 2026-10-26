/*
 * Pack motor and battery readings into CAN frames.
 */

#include "sensors.h"


static void electric_pack_reading(uint8_t out[4], uint16_t value, uint16_t reference) {

	out[0] = (uint8_t)(value & 0xFF);
	out[1] = (uint8_t)((value >> 8) & 0xFF);
	out[2] = (uint8_t)(reference & 0xFF);
	out[3] = (uint8_t)((reference >> 8) & 0xFF);
}


static uint16_t electric_battery_id(uint8_t index) {

	switch (index) {
		case 0:
			return CAN_ID_BATT_VOLTAGE;
		case 1:
			return CAN_ID_BATT_1_VOLTAGE;
		case 2:
			return CAN_ID_BATT_2_VOLTAGE;
		default:
			return 0;
	}
}


uint8_t electric_sensor_frames(
	int16_t motor_voltage,
	const uint16_t *batteries,
	uint8_t battery_count,
	uint16_t reference,
	ELECTRIC_SENSOR_FRAME *out,
	uint8_t max_out
) {

	if (out == 0 || max_out == 0) {
		return 0;
	}

	uint8_t count = 0;

	out[count].id = CAN_ID_MOTOR_VOLTAGE;
	out[count].size = 4;
	electric_pack_reading(out[count].data, (uint16_t)motor_voltage, reference);
	count++;

	if (batteries == 0 || battery_count == 0 || count >= max_out) {
		return count;
	}

	if (battery_count > ELECTRIC_BATT_MAX) {
		battery_count = ELECTRIC_BATT_MAX;
	}

	uint32_t sum = 0;

	for (uint8_t i = 0; i < battery_count && count < max_out; i++) {
		uint16_t id = electric_battery_id(i);
		if (id == 0) {
			break;
		}

		out[count].id = id;
		out[count].size = 4;
		electric_pack_reading(out[count].data, batteries[i], reference);
		sum += batteries[i];
		count++;
	}

	if (count < max_out && battery_count > 0) {
		if (sum > 0xFFFFUL) {
			sum = 0xFFFFUL;
		}
		out[count].id = CAN_ID_VOLTAGE;
		out[count].size = 4;
		electric_pack_reading(out[count].data, (uint16_t)sum, reference);
		count++;
	}

	return count;
}
