/*
 * Map analog CAN fields onto 0..1023 servo positions.
 */

#include "mapping.h"


void servo_bus_clear(SERVO_BUS &bus) {

	bus.drive = 0;
	bus.brake = 0;
	bus.power = 0;
	bus.dir = 0;
	bus.speed = 0;
	bus.tacho = 0;
	bus.module_current = 0;
	bus.motor_current = 0;
	bus.batt_current = 0;
	bus.voltage = 0;
	bus.motor_voltage = 0;
	bus.batt_voltage = 0;
	bus.alive = false;
}


uint16_t servo_analog_scale(uint16_t value, uint16_t full_scale) {

	if (full_scale == 0) {
		if (value > SERVO_ANALOG_MAX) {
			return SERVO_ANALOG_MAX;
		}
		return value;
	}

	if (value > full_scale) {
		value = full_scale;
	}

	return (uint16_t)(((uint32_t)value * (uint32_t)SERVO_ANALOG_MAX) / full_scale);
}


static uint16_t servo_le16(const uint8_t *data) {

	return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}


static uint16_t servo_frame_analog(const CAN_MESSAGE &message) {

	if (message.size < 2) {
		return 0;
	}

	uint16_t value = servo_le16(message.data);

	if (message.id == CAN_ID_MOTOR_VOLTAGE) {
		int16_t signed_value = (int16_t)value;
		if (signed_value < 0) {
			signed_value = (int16_t)(-signed_value);
		}
		value = (uint16_t)signed_value;
	}

	uint16_t full_scale = 0;
	if (message.size >= 4) {
		full_scale = servo_le16(message.data + 2);
	}

	return servo_analog_scale(value, full_scale);
}


static void servo_apply_drive(SERVO_BUS &bus, const CAN_MESSAGE &message) {

	if (message.size < 1) {
		return;
	}

	bus.dir = (message.data[0] & (uint8_t)(1 << CONTROL_DIR_FLAG))
		? SERVO_ANALOG_MAX
		: 0;
	bus.alive = true;

	if (message.size < 8) {
		return;
	}

	bus.drive = (uint16_t)(((message.data[2] & 0x03) << 8) | message.data[3]);
	bus.power = (uint16_t)(((message.data[4] & 0x03) << 8) | message.data[5]);
	bus.brake = (uint16_t)(((message.data[6] & 0x03) << 8) | message.data[7]);
}


void servo_bus_apply(SERVO_BUS &bus, const CAN_MESSAGE &message) {

	if (message.id == CAN_ID_EMERGENCY) {
		servo_bus_clear(bus);
		return;
	}

	switch (message.id) {

		case CAN_ID_DRIVE:
			servo_apply_drive(bus, message);
			break;

		case CAN_ID_DIR:
			if (message.size >= 1) {
				bus.dir = message.data[0] ? SERVO_ANALOG_MAX : 0;
				bus.alive = true;
			}
			break;

		case CAN_ID_SPEED:
			bus.speed = servo_frame_analog(message);
			bus.alive = true;
			break;

		case CAN_ID_TACHO:
			bus.tacho = servo_frame_analog(message);
			bus.alive = true;
			break;

		case CAN_ID_MODULE_CURRENT:
			bus.module_current = servo_frame_analog(message);
			bus.alive = true;
			break;

		case CAN_ID_MOTOR_CURRENT:
			bus.motor_current = servo_frame_analog(message);
			bus.alive = true;
			break;

		case CAN_ID_BATT_CURRENT:
			bus.batt_current = servo_frame_analog(message);
			bus.alive = true;
			break;

		case CAN_ID_VOLTAGE:
			bus.voltage = servo_frame_analog(message);
			bus.alive = true;
			break;

		case CAN_ID_MOTOR_VOLTAGE:
			bus.motor_voltage = servo_frame_analog(message);
			bus.alive = true;
			break;

		case CAN_ID_BATT_VOLTAGE:
		case CAN_ID_BATT_1_VOLTAGE:
		case CAN_ID_BATT_2_VOLTAGE:
			bus.batt_voltage = servo_frame_analog(message);
			bus.alive = true;
			break;

		default:
			break;
	}
}


uint16_t servo_output_value(uint8_t map, const SERVO_BUS &bus) {

	uint16_t value = 0;

	switch (servo_map_source(map)) {

		case SERVO_SRC_DRIVE:
			value = bus.drive;
			break;
		case SERVO_SRC_BRAKE:
			value = bus.brake;
			break;
		case SERVO_SRC_POWER:
			value = bus.power;
			break;
		case SERVO_SRC_DIR:
			value = bus.dir;
			break;
		case SERVO_SRC_SPEED:
			value = bus.speed;
			break;
		case SERVO_SRC_TACHO:
			value = bus.tacho;
			break;
		case SERVO_SRC_MODULE_CURRENT:
			value = bus.module_current;
			break;
		case SERVO_SRC_MOTOR_CURRENT:
			value = bus.motor_current;
			break;
		case SERVO_SRC_BATT_CURRENT:
			value = bus.batt_current;
			break;
		case SERVO_SRC_VOLTAGE:
			value = bus.voltage;
			break;
		case SERVO_SRC_MOTOR_VOLTAGE:
			value = bus.motor_voltage;
			break;
		case SERVO_SRC_BATT_VOLTAGE:
			value = bus.batt_voltage;
			break;
		default:
			return 0;
	}

	if (value > SERVO_ANALOG_MAX) {
		value = SERVO_ANALOG_MAX;
	}

	if (servo_map_invert(map)) {
		value = (uint16_t)(SERVO_ANALOG_MAX - value);
	}

	return value;
}
