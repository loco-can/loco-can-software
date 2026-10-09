/*
 * Loco-CAN servo function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2026
 * @lizence: GG0
 */

#include "main.h"

#ifdef MODULE_SERVO_CONFIG_H

#include <EEPROM.h>
#include "../../core/servo/intelliServo.h"

extern CAN_COM can;

#define SERVO_EEPROM_BYTES 64

static const uint8_t SERVO_OUTPUT_PORT[SERVO_OUTPUT_COUNT] = {
	SERVO_1,
	SERVO_2,
	SERVO_3,
	SERVO_4
};

static INTELLISERVO servo_out[SERVO_OUTPUT_COUNT];


static void servo_eeprom_begin(void) {

	#ifdef MODULE_ARCH_ESP32
		EEPROM.begin(SERVO_EEPROM_BYTES);
	#endif
}


static void servo_eeprom_put(int idx, uint8_t value) {

	if (EEPROM.read(idx) != value) {
		EEPROM.write(idx, value);
	}
}


void MODULE_SERVO::_load_params(void) {

	servo_eeprom_begin();

	uint8_t version = (uint8_t)SERVO_MODULE_VERSION;
	uint8_t magic = EEPROM.read(0);
	uint8_t schema = EEPROM.read(1);
	uint8_t stored_version = EEPROM.read(2);

	if (magic != SERVO_PARAM_MAGIC || schema != SERVO_PARAM_SCHEMA || stored_version != version) {
		servo_params_defaults(_params, version);
		_save_params();
		return;
	}

	for (uint8_t i = 0; i < SERVO_PARAM_BYTES; i++) {
		_params.bytes[i] = EEPROM.read((int)(3 + i));
	}

	for (uint8_t i = 0; i < SERVO_PARAM_NAME_LEN; i++) {
		_params.name[i] = (char)EEPROM.read((int)(3 + SERVO_PARAM_BYTES + i));
	}
	_params.name[SERVO_PARAM_NAME_LEN] = 0;
	_params.bytes[SERVO_PARAM_VERSION] = version;

	for (uint8_t i = 0; i < SERVO_OUTPUT_COUNT; i++) {
		if (!servo_map_valid(_params.bytes[SERVO_PARAM_MAP + i])) {
			servo_params_defaults(_params, version);
			_save_params();
			return;
		}
	}
}


void MODULE_SERVO::_save_params(void) {

	servo_eeprom_begin();

	servo_eeprom_put(0, (uint8_t)SERVO_PARAM_MAGIC);
	servo_eeprom_put(1, (uint8_t)SERVO_PARAM_SCHEMA);
	servo_eeprom_put(2, _params.bytes[SERVO_PARAM_VERSION]);

	for (uint8_t i = 0; i < SERVO_PARAM_BYTES; i++) {
		servo_eeprom_put((int)(3 + i), _params.bytes[i]);
	}

	for (uint8_t i = 0; i < SERVO_PARAM_NAME_LEN; i++) {
		servo_eeprom_put((int)(3 + SERVO_PARAM_BYTES + i), (uint8_t)_params.name[i]);
	}

	#ifdef MODULE_ARCH_ESP32
		EEPROM.commit();
	#endif
}


void MODULE_SERVO::_handle_setup(CAN_MESSAGE message) {

	SERVO_PARAM_RESULT result = servo_params_on_can(
		_params,
		message,
		(uint16_t)can.uuid(),
		(uint8_t)SERVO_MODULE_VERSION,
		SERVO_TYPE_ID
	);

	for (uint8_t i = 0; i < result.reply_count && i < 2; i++) {
		can.send(result.reply[i]);
	}

	if (result.changed) {
		_save_params();
	}
}


void MODULE_SERVO::_write_outputs(void) {

	bool enable = _bus.alive && !_bus_timeout.check();

	for (uint8_t i = 0; i < SERVO_OUTPUT_COUNT; i++) {
		uint8_t map = servo_params_map(_params, i);
		if (servo_map_source(map) == SERVO_SRC_NONE) {
			continue;
		}

		uint16_t value = 0;
		if (enable) {
			value = servo_output_value(map, _bus);
		}
		servo_out[i].set(value);
	}
}


void MODULE_SERVO::begin(void) {

	#ifdef DEBUG
		Serial.println("********************");
		Serial.println("start function/servo");
	#endif

	servo_bus_clear(_bus);
	_bus_timeout.begin(CAN_ALIVE_TIMEOUT);

	for (uint8_t i = 0; i < SERVO_OUTPUT_COUNT; i++) {
		servo_out[i].begin(SERVO_OUTPUT_PORT[i]);
		servo_out[i].set_value_limits(0, SERVO_ANALOG_MAX);
	}

	/* Group masks: analog current / speed / voltage plus drive commands. */
	can.register_filter(0x700, 0x100);
	can.register_filter(0x700, 0x200);
	can.register_filter(0x700, 0x300);
	can.register_filter(0x700, 0x400);
	can.register_filter(0x7FF, CAN_ID_EMERGENCY);
	can.register_filter(0x7FF, CAN_ID_REQUEST);
	can.register_filter(0x780, CAN_ID_SETUP);

	_load_params();

	#ifdef DEBUG
		Serial.println("> servo output maps");
		for (uint8_t i = 0; i < SERVO_OUTPUT_COUNT; i++) {
			uint8_t map = servo_params_map(_params, i);
			Serial.print(">  servo ");
			Serial.print(i + 1);
			Serial.print(" src ");
			Serial.print(servo_map_source(map));
			Serial.print(" inv ");
			Serial.println(servo_map_invert(map) ? 1 : 0);
		}
	#endif
}


void MODULE_SERVO::update(CAN_MESSAGE message) {

	bool frame = message.id != 0 || message.uuid != 0 || message.size != 0;

	if (frame && (message.id == CAN_ID_REQUEST || (message.id & 0x780) == CAN_ID_SETUP)) {
		_handle_setup(message);
	}
	else if (frame && message.uuid != (uint16_t)can.uuid()) {
		servo_bus_apply(_bus, message);
		if (message.id != CAN_ID_EMERGENCY && _bus.alive) {
			_bus_timeout.retrigger();
		}
	}

	_write_outputs();
}

#endif
