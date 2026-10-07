/*
 * Loco-CAN electric loco function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "main.h"

#include <EEPROM.h>

#ifdef MODULE_ELECTRIC_CONFIG_H


#define ELECTRIC_EEPROM_BYTES 64
#define ELECTRIC_DRIVE_TIMEOUT (CAN_ID_DRIVE_TIME * 3)
#define ELECTRIC_DIRECTION_GAP 30
#define ELECTRIC_SETUP_TIMEOUT 500
#define ELECTRIC_SENSOR_PERIOD 200
#define ELECTRIC_MODULE_HEARTBEAT_PERIOD (MODULE_HEARTBEAT_TIMEOUT / 2)

#define ELECTRIC_SETUP_DIRECTION 0
#define ELECTRIC_SETUP_DISABLE 1


extern CAN_COM can;


static const uint8_t ELECTRIC_BATT_PORTS[ELECTRIC_BATT_MAX] = {
	ELECTRIC_BATT_0,
	ELECTRIC_BATT_1,
	ELECTRIC_BATT_2
};


static void electric_eeprom_begin(void) {

	#ifdef MODULE_ARCH_ESP32
		EEPROM.begin(ELECTRIC_EEPROM_BYTES);
	#endif
}


static void electric_eeprom_put(int idx, uint8_t value) {

	if (EEPROM.read(idx) != value) {
		EEPROM.write(idx, value);
	}
}


void MODULE_ELECTRIC::_load_settings(void) {

	electric_eeprom_begin();

	uint8_t version = (uint8_t)ELECTRIC_MODULE_VERSION;
	uint8_t magic = EEPROM.read(0);
	uint8_t schema = EEPROM.read(1);
	uint8_t stored_version = EEPROM.read(2);

	if (magic != ELECTRIC_SETTINGS_MAGIC || schema != ELECTRIC_SETTINGS_SCHEMA || stored_version != version) {
		electric_settings_defaults(_settings, version);
		_save_settings();
		return;
	}

	for (uint8_t i = 0; i < ELECTRIC_PARAM_BYTES; i++) {
		_settings.bytes[i] = EEPROM.read((int)(3 + i));
	}

	for (uint8_t i = 0; i < ELECTRIC_SETTINGS_NAME_LEN; i++) {
		_settings.name[i] = (char)EEPROM.read((int)(3 + ELECTRIC_PARAM_BYTES + i));
	}
	_settings.name[ELECTRIC_SETTINGS_NAME_LEN] = 0;
	_settings.bytes[ELECTRIC_PARAM_VERSION] = version;
}


void MODULE_ELECTRIC::_save_settings(void) {

	electric_eeprom_begin();

	electric_eeprom_put(0, (uint8_t)ELECTRIC_SETTINGS_MAGIC);
	electric_eeprom_put(1, (uint8_t)ELECTRIC_SETTINGS_SCHEMA);
	electric_eeprom_put(2, _settings.bytes[ELECTRIC_PARAM_VERSION]);

	for (uint8_t i = 0; i < ELECTRIC_PARAM_BYTES; i++) {
		electric_eeprom_put((int)(3 + i), _settings.bytes[i]);
	}

	for (uint8_t i = 0; i < ELECTRIC_SETTINGS_NAME_LEN; i++) {
		electric_eeprom_put((int)(3 + ELECTRIC_PARAM_BYTES + i), (uint8_t)_settings.name[i]);
	}

	#ifdef MODULE_ARCH_ESP32
		EEPROM.commit();
	#endif
}


void MODULE_ELECTRIC::_handle_settings(CAN_MESSAGE message) {

	ELECTRIC_SETTINGS_RESULT result = electric_settings_on_can(
		_settings,
		message,
		(uint16_t)can.uuid(),
		(uint8_t)ELECTRIC_MODULE_VERSION,
		ELECTRIC_TYPE_ID
	);

	for (uint8_t i = 0; i < result.reply_count && i < 2; i++) {
		can.send(result.reply[i]);
	}

	if (result.changed) {
		_save_settings();
	}
}


void MODULE_ELECTRIC::_handle_can(CAN_MESSAGE message) {

	switch (message.id) {

		case CAN_ID_DRIVE:
			if (!electric_unpack_drive(message.data, message.size, _command)) {
				break;
			}
			_seen_drive = true;
			_drive_timeout.retrigger();
			if (_command.error) {
				_emergency = true;
			}
			if (!_command.mains && !_command.error) {
				_emergency = false;
				_link_fault = false;
			}
			break;

		case CAN_ID_DRIVE_HEARTBEAT:
			/* Drive frames time the link out. A heartbeat on its own does not refresh a throttle command. */
			break;

		case CAN_ID_EMERGENCY:
			_emergency = true;
			break;

		case CAN_ID_LOCO_SETUP:
			if (message.size < 3) {
				break;
			}
			{
				bool disable = (message.data[0] & (uint8_t)(1 << ELECTRIC_SETUP_DISABLE)) != 0;
				bool direction = (message.data[0] & (uint8_t)(1 << ELECTRIC_SETUP_DIRECTION)) != 0;
				uint16_t loco = (uint16_t)message.data[1] | ((uint16_t)message.data[2] << 8);

				if (disable || loco == 0) {
					_setup_active = false;
					_setup_selected = false;
					_setup_dir_level = false;
					break;
				}

				_setup_active = true;
				_setup_timeout.retrigger();
				_setup_selected = loco == (uint16_t)can.uuid();

				if (_setup_selected) {
					if (direction && !_setup_dir_level) {
						electric_settings_set_reversed(_settings, !electric_settings_reversed(_settings));
						_save_settings();
					}
					_setup_dir_level = direction;
				}
				else {
					_setup_dir_level = false;
				}
			}
			break;

		default:
			break;
	}
}


void MODULE_ELECTRIC::_read_sensors(void) {

	int32_t plus = _motor.get(MEASURE_VALUE_1);
	int32_t minus = _motor.get(MEASURE_VALUE_2);
	_motor_voltage = plus - minus;

	uint8_t count = electric_settings_battery_count(_settings);

	for (uint8_t i = 0; i < count; i++) {
		int32_t raw = _battery_input[i].get();
		if (raw < 0) {
			raw = 0;
		}
		if (raw > 65535) {
			raw = 65535;
		}
		_battery[i] = (uint16_t)raw;
	}
}


void MODULE_ELECTRIC::_write_outputs(const ELECTRIC_OUTPUT &output) {

	analogWrite(DRIVE_PWM, output.drive_pwm);
	analogWrite(DRIVE_BREAK, output.brake_pwm);

	/*
	 * Open the series contactor before the direction relays move, and
	 * close it only after the new direction is on the pins.
	 */
	#ifdef DRIVE_POWER
		if (!output.power) {
			digitalWrite(DRIVE_POWER, LOW);
		}
	#endif

	if (output.forward && output.reverse) {
		digitalWrite(DRIVE_FORW, HIGH);
		digitalWrite(DRIVE_REV, HIGH);
	}
	else if (output.forward) {
		digitalWrite(DRIVE_REV, LOW);
		digitalWrite(DRIVE_FORW, HIGH);
	}
	else if (output.reverse) {
		digitalWrite(DRIVE_FORW, LOW);
		digitalWrite(DRIVE_REV, HIGH);
	}
	else {
		digitalWrite(DRIVE_FORW, LOW);
		digitalWrite(DRIVE_REV, LOW);
	}

	#ifdef DRIVE_POWER
		if (output.power) {
			digitalWrite(DRIVE_POWER, HIGH);
		}
	#endif
}


void MODULE_ELECTRIC::_send_status(const ELECTRIC_STATUS_BITS &bits) {

	if (!_status_time.update()) {
		return;
	}

	uint16_t uuid = (uint16_t)can.uuid();

	_tx.id = CAN_ID_VEHICLE_STATUS;
	_tx.size = 3;
	_tx.data[0] = electric_status_byte(bits);
	_tx.data[1] = (uint8_t)(uuid & 0xFF);
	_tx.data[2] = (uint8_t)((uuid >> 8) & 0xFF);
	can.send(_tx);
}


void MODULE_ELECTRIC::_send_sensors(void) {

	if (!_sensor_time.update()) {
		return;
	}

	int32_t motor = _motor_voltage;
	if (motor > 32767) {
		motor = 32767;
	}
	if (motor < -32768) {
		motor = -32768;
	}

	uint8_t count = electric_settings_battery_count(_settings);
	uint16_t reference = 0;
	if (PLATFORM_ANALOG_RESOLUTION > 0) {
		reference = (uint16_t)(PLATFORM_ANALOG_RESOLUTION - 1);
	}

	ELECTRIC_SENSOR_FRAME frames[ELECTRIC_SENSOR_FRAME_MAX];
	uint8_t frames_n = electric_sensor_frames(
		(int16_t)motor,
		_battery,
		count,
		reference,
		frames,
		ELECTRIC_SENSOR_FRAME_MAX
	);

	for (uint8_t i = 0; i < frames_n; i++) {
		_tx.id = frames[i].id;
		_tx.size = frames[i].size;
		for (uint8_t b = 0; b < frames[i].size && b < 4; b++) {
			_tx.data[b] = frames[i].data[b];
		}
		can.send(_tx);
	}
}


void MODULE_ELECTRIC::_send_module_heartbeat(void) {

	if (!_module_heartbeat.update()) {
		return;
	}

	_tx.id = CAN_ID_MODULE_HEARTBEAT;
	_tx.size = 0;
	can.send(_tx);
}


void MODULE_ELECTRIC::begin(void) {

	#ifdef DEBUG
		Serial.println("********************");
		Serial.println("start function/motor");
	#endif

	_drive_timeout.begin(ELECTRIC_DRIVE_TIMEOUT);
	_direction_gap.begin(ELECTRIC_DIRECTION_GAP);
	_setup_timeout.begin(ELECTRIC_SETUP_TIMEOUT);
	_status_time.begin(CAN_ID_DRIVE_TIME);
	_sensor_time.begin(ELECTRIC_SENSOR_PERIOD);
	_module_heartbeat.begin(ELECTRIC_MODULE_HEARTBEAT_PERIOD);

	electric_plugin_reset(_plugin_state);
	_command.present = false;
	_command.mains = false;
	_command.motor = false;
	_command.reverse = false;
	_command.error = false;
	_command.emergency = false;
	_command.multi = false;
	_command.loco = 0;
	_command.drive = 0;
	_command.brake = 0;
	_command.power = 0;

	_seen_drive = false;
	_link_fault = false;
	_emergency = false;
	_setup_active = false;
	_setup_selected = false;
	_setup_dir_level = false;
	_gap_waiting = false;
	_motor_voltage = 0;

	for (uint8_t i = 0; i < ELECTRIC_BATT_MAX; i++) {
		_battery[i] = 0;
	}

	can.register_filter(CAN_ID_MASK, CAN_ID_DRIVE);
	can.register_filter(CAN_ID_MASK, CAN_ID_DRIVE_HEARTBEAT);
	can.register_filter(0x7FF, CAN_ID_EMERGENCY);
	can.register_filter(0x7FF, CAN_ID_REQUEST);
	can.register_filter(0x780, CAN_ID_SETUP);
	can.register_filter(0x7FF, CAN_ID_LOCO_SETUP);

	_load_settings();
	_applied_plugin = electric_settings_plugin(_settings);
	_applied_mode = electric_settings_pwm_mode(_settings);

	pinMode(DRIVE_PWM, OUTPUT);
	pinMode(DRIVE_BREAK, OUTPUT);
	pinMode(DRIVE_FORW, OUTPUT);
	pinMode(DRIVE_REV, OUTPUT);
	analogWrite(DRIVE_PWM, 0);
	analogWrite(DRIVE_BREAK, 0);
	digitalWrite(DRIVE_FORW, LOW);
	digitalWrite(DRIVE_REV, LOW);

	#ifdef DRIVE_POWER
		pinMode(DRIVE_POWER, OUTPUT);
		digitalWrite(DRIVE_POWER, LOW);
	#endif

	_motor.begin(DRIVE_MOTOR_VOLTAGE_PLUS, DRIVE_MOTOR_VOLTAGE_MINUS);
	_motor.set_filter(0.02f);

	for (uint8_t i = 0; i < ELECTRIC_BATT_MAX; i++) {
		_battery_input[i].begin(ELECTRIC_BATT_PORTS[i]);
		_battery_input[i].set_filter(0.05f);
	}
}


void MODULE_ELECTRIC::update(CAN_MESSAGE message) {

	if (message.id == CAN_ID_REQUEST || (message.id & 0x780) == CAN_ID_SETUP) {
		_handle_settings(message);
	}
	else if (message.uuid != 0) {
		_handle_can(message);
	}

	if (_setup_active && _setup_timeout.check()) {
		_setup_active = false;
		_setup_selected = false;
		_setup_dir_level = false;
	}

	bool drive_fresh = _seen_drive && !_drive_timeout.check();
	if (_seen_drive && !drive_fresh && _command.mains) {
		_link_fault = true;
	}

	_read_sensors();

	uint8_t plugin = electric_settings_plugin(_settings);
	uint8_t mode = electric_settings_pwm_mode(_settings);
	if (plugin != _applied_plugin || mode != _applied_mode) {
		electric_plugin_reset(_plugin_state);
		_applied_plugin = plugin;
		_applied_mode = mode;
	}

	bool setup_idle = _setup_active && !_setup_selected;

	ELECTRIC_PLUGIN_INPUT input;
	input.pwm_mode = mode;
	#ifdef DRIVE_POWER
		input.power_relay_fitted = true;
	#else
		input.power_relay_fitted = false;
	#endif
	input.voltage_min = electric_settings_voltage_min(_settings);
	input.motor_voltage = _motor_voltage;
	input.reversed = electric_settings_reversed(_settings);
	input.gap_elapsed = _gap_waiting && _direction_gap.check();
	input.command = _command;
	input.command.present = drive_fresh && !_emergency;
	input.command.emergency = _emergency || _link_fault;
	if (setup_idle) {
		input.command.motor = false;
		input.command.drive = 0;
		input.command.brake = 0;
		input.command.power = 0;
	}

	ELECTRIC_OUTPUT output = electric_plugin_apply(plugin, input, _plugin_state);
	if (output.direction == ELECTRIC_DIR_NONE && output.direction_blocked) {
		if (!_gap_waiting) {
			_gap_waiting = true;
			_direction_gap.retrigger();
		}
	}
	else {
		_gap_waiting = false;
	}
	_write_outputs(output);

	ELECTRIC_STATUS_BITS bits = electric_vehicle_status(
		input.command,
		output,
		_motor_voltage,
		input.voltage_min,
		input.reversed,
		_link_fault,
		setup_idle
	);

	_send_status(bits);
	_send_sensors();
	_send_module_heartbeat();
}

#endif
