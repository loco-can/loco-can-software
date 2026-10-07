/*
 * Sensor frames published by the electric module.
 *
 * Each frame is four bytes:
 *   0..1  little-endian reading (motor voltage is signed)
 *   2..3  little-endian ADC full-scale reference
 *
 * Battery readings use CAN_ID_BATT_VOLTAGE and the following ids.
 * The series sum is CAN_ID_VOLTAGE. Motor voltage is CAN_ID_MOTOR_VOLTAGE.
 */
#pragma once

#ifndef ELECTRIC_SENSORS_H
#define ELECTRIC_SENSORS_H

#include <stdint.h>

#include "../../can_protocol.h"
#include "settings.h"

#define ELECTRIC_SENSOR_FRAME_MAX (2 + ELECTRIC_BATT_MAX)

struct ELECTRIC_SENSOR_FRAME {
	uint16_t id;
	uint8_t size;
	uint8_t data[4];
};

uint8_t electric_sensor_frames(
	int16_t motor_voltage,
	const uint16_t *batteries,
	uint8_t battery_count,
	uint16_t reference,
	ELECTRIC_SENSOR_FRAME *out,
	uint8_t max_out
);

#endif
