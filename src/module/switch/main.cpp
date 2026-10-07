/*
 * Loco-CAN switch function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2026
 * @lizence: GG0
 */

#include "main.h"

#ifdef MODULE_SWITCH_CONFIG_H

#include <EEPROM.h>

extern CAN_COM can;

#define SWITCH_EEPROM_BYTES 64

#if SWITCH_PORT_COUNT != SWITCH_OUTPUT_COUNT
	#error "SWITCH_PORT_COUNT must match SWITCH_OUTPUT_COUNT"
#endif

static const uint8_t SWITCH_OUTPUT_PORT[SWITCH_OUTPUT_COUNT] = {
	LIGHT1,
	LIGHT2,
	LIGHT3,
	LIGHT4,
	LIGHT5,
	LIGHT6
};


static void switch_eeprom_begin(void) {

	#ifdef MODULE_ARCH_ESP32
		EEPROM.begin(SWITCH_EEPROM_BYTES);
	#endif
}


static void switch_eeprom_put(int idx, uint8_t value) {

	if (EEPROM.read(idx) != value) {
		EEPROM.write(idx, value);
	}
}


void MODULE_SWITCH::_load_params(void) {

	switch_eeprom_begin();

	uint8_t version = (uint8_t)SWITCH_MODULE_VERSION;
	uint8_t magic = EEPROM.read(0);
	uint8_t schema = EEPROM.read(1);
	uint8_t stored_version = EEPROM.read(2);

	if (magic != SWITCH_PARAM_MAGIC || schema != SWITCH_PARAM_SCHEMA || stored_version != version) {
		switch_params_defaults(_params, version);
		_save_params();
		return;
	}

	for (uint8_t i = 0; i < SWITCH_PARAM_BYTES; i++) {
		_params.bytes[i] = EEPROM.read((int)(3 + i));
	}

	for (uint8_t i = 0; i < SWITCH_PARAM_NAME_LEN; i++) {
		_params.name[i] = (char)EEPROM.read((int)(3 + SWITCH_PARAM_BYTES + i));
	}
	_params.name[SWITCH_PARAM_NAME_LEN] = 0;
	_params.bytes[SWITCH_PARAM_VERSION] = version;

	for (uint8_t i = 0; i < SWITCH_OUTPUT_COUNT; i++) {
		if (!switch_map_valid(_params.bytes[SWITCH_PARAM_MAP + i])) {
			switch_params_defaults(_params, version);
			_save_params();
			return;
		}
	}
}


void MODULE_SWITCH::_save_params(void) {

	switch_eeprom_begin();

	switch_eeprom_put(0, (uint8_t)SWITCH_PARAM_MAGIC);
	switch_eeprom_put(1, (uint8_t)SWITCH_PARAM_SCHEMA);
	switch_eeprom_put(2, _params.bytes[SWITCH_PARAM_VERSION]);

	for (uint8_t i = 0; i < SWITCH_PARAM_BYTES; i++) {
		switch_eeprom_put((int)(3 + i), _params.bytes[i]);
	}

	for (uint8_t i = 0; i < SWITCH_PARAM_NAME_LEN; i++) {
		switch_eeprom_put((int)(3 + SWITCH_PARAM_BYTES + i), (uint8_t)_params.name[i]);
	}

	#ifdef MODULE_ARCH_ESP32
		EEPROM.commit();
	#endif
}


void MODULE_SWITCH::_handle_setup(CAN_MESSAGE message) {

	SWITCH_PARAM_RESULT result = switch_params_on_can(
		_params,
		message,
		(uint16_t)can.uuid(),
		(uint8_t)SWITCH_MODULE_VERSION,
		SWITCH_TYPE_ID
	);

	for (uint8_t i = 0; i < result.reply_count && i < 2; i++) {
		can.send(result.reply[i]);
	}

	if (result.changed) {
		_save_params();
	}
}


void MODULE_SWITCH::_sample_current(void) {

	uint16_t raw = analogRead(SWITCH_CURRENT_PORT);
	uint16_t limit = switch_params_max_current(_params);

	_milliamp = switch_current_from_adc(raw, PLATFORM_ANALOG_RESOLUTION, SWITCH_CURRENT_FULL_SCALE_MA);

	if (switch_current_over(_milliamp, limit)) {
		if (!_overcurrent) {
			_overcurrent = true;
			#ifdef DEBUG
				Serial.print("> switch overcurrent mA ");
				Serial.println(_milliamp);
			#endif
			_send_emergency();
			_send_current();
		}
		_overcurrent_hold.retrigger();
	}
	else if (_overcurrent && _overcurrent_hold.check()) {
		_overcurrent = false;
	}
}


void MODULE_SWITCH::_send_current(void) {

	CAN_MESSAGE message;

	_current_time.retrigger();

	message.id = SWITCH_CURRENT_ID;
	message.uuid = 0;
	message.size = SWITCH_CURRENT_FRAME;
	for (uint8_t i = 0; i < 8; i++) {
		message.data[i] = 0;
	}
	switch_current_pack(_milliamp, switch_params_max_current(_params), message.data);
	can.send(message);
}


void MODULE_SWITCH::_send_emergency(void) {

	CAN_MESSAGE message;

	message.id = CAN_ID_EMERGENCY;
	message.uuid = 0;
	message.size = 0;
	for (uint8_t i = 0; i < 8; i++) {
		message.data[i] = 0;
	}
	can.send(message);
}


void MODULE_SWITCH::_write_outputs(void) {

	bool enable = !_overcurrent && _bus.alive && !_bus_timeout.check();

	for (uint8_t i = 0; i < SWITCH_OUTPUT_COUNT; i++) {
		bool on = enable && switch_output_level(switch_params_map(_params, i), _bus);
		digitalWrite(SWITCH_OUTPUT_PORT[i], on ? HIGH : LOW);
	}
}


void MODULE_SWITCH::begin(void) {

	#ifdef DEBUG
		Serial.println("*********************");
		Serial.println("start function/switch");
	#endif

	switch_bus_clear(_bus);
	_bus_timeout.begin(CAN_ALIVE_TIMEOUT);
	_current_time.begin(SWITCH_CURRENT_PERIOD_MS);
	_overcurrent_hold.begin(SWITCH_OVERCURRENT_HOLD_MS);
	_milliamp = 0;
	_overcurrent = false;

	for (uint8_t i = 0; i < SWITCH_OUTPUT_COUNT; i++) {
		pinMode(SWITCH_OUTPUT_PORT[i], OUTPUT);
		digitalWrite(SWITCH_OUTPUT_PORT[i], LOW);
	}

	pinMode(SWITCH_CURRENT_PORT, INPUT);

	can.register_filter(CAN_ID_MASK, CAN_ID_LIGHT);
	can.register_filter(CAN_ID_MASK, CAN_ID_SIGNAL);
	can.register_filter(CAN_ID_MASK, CAN_ID_DRIVE);
	can.register_filter(0x7FF, CAN_ID_EMERGENCY);
	can.register_filter(0x7FF, CAN_ID_REQUEST);
	can.register_filter(0x780, CAN_ID_SETUP);

	_load_params();

	#ifdef DEBUG
		Serial.println("> switch output functions");
		for (uint8_t i = 0; i < SWITCH_OUTPUT_COUNT; i++) {
			uint8_t map = switch_params_map(_params, i);
			Serial.print(">  out ");
			Serial.print(i + 1);
			Serial.print(" msg ");
			Serial.print(switch_map_message(map));
			Serial.print(" bit ");
			Serial.print(switch_map_bit(map));
			Serial.print(" dir ");
			Serial.println(switch_map_dir(map));
		}
	#endif
}


void MODULE_SWITCH::update(CAN_MESSAGE message) {

	bool frame = message.id != 0 || message.uuid != 0 || message.size != 0;

	if (frame && (message.id == CAN_ID_REQUEST || (message.id & 0x780) == CAN_ID_SETUP)) {
		_handle_setup(message);
	}
	else if (frame && message.uuid != 0 && message.uuid != (uint16_t)can.uuid()) {
		if (message.id == CAN_ID_LIGHT || message.id == CAN_ID_SIGNAL || message.id == CAN_ID_DRIVE || message.id == CAN_ID_EMERGENCY) {
			switch_bus_apply(_bus, message);
			if (message.id != CAN_ID_EMERGENCY && message.size >= 1) {
				_bus_timeout.retrigger();
			}
		}
	}

	_sample_current();

	if (_current_time.check()) {
		_send_current();
	}

	_write_outputs();
}

#endif
