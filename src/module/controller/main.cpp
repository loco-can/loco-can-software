/*
 * Loco-CAN controller function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../config.h"
#include "main.h"
#include "drive_codec.h"
#include "../../core/setup/setup_mode.h"
#include <string.h>


extern CAN_COM can;


/* intelliButton::check() returns 2 for a short release and 1 for a long one. */
#define CONTROLLER_BUTTON_SHORT 2

#define CONTROLLER_VEHICLE_TIMEOUT (MODULE_HEARTBEAT_TIMEOUT * 2)
#define CONTROLLER_EEPROM_BYTES 96


#ifdef MODULE_CONTROLLER_CONFIG_H

#include <EEPROM.h>

static uint8_t light_from_position(uint8_t pos) {

	switch (pos) {
		case 0:
			return 0;
		case 1:
			return (uint8_t)(1 << LIGHT_POSIT);
		case 2:
			return (uint8_t)((1 << LIGHT_POSIT) | (1 << LIGHT_LOW));
		case 3:
			return (uint8_t)((1 << LIGHT_POSIT) | (1 << LIGHT_LOW) | (1 << LIGHT_HIGH));
		default:
			return (uint8_t)((1 << LIGHT_MAIN) | (1 << LIGHT_TRAIN) | (1 << LIGHT_POSIT));
	}
}


static uint8_t light2_from_position(uint8_t pos) {

	uint8_t bits = 0;

	if (pos >= 1) {
		bits |= (uint8_t)(1 << LIGHT_CAB);
	}
	if (pos >= 2) {
		bits |= (uint8_t)(1 << LIGHT_BACK);
	}
	if (pos >= 3) {
		bits |= (uint8_t)(1 << LIGHT_INSTR);
	}

	return bits;
}


static void controller_eeprom_begin(void) {

	#ifdef MODULE_ARCH_ESP32
		EEPROM.begin(CONTROLLER_EEPROM_BYTES);
	#endif
}


static void controller_eeprom_put(int idx, uint8_t value) {

	if (EEPROM.read(idx) != value) {
		EEPROM.write(idx, value);
	}
}


void MODULE_CONTROLLER::_load_params(void) {

	controller_eeprom_begin();

	const bool dual = CONTROLLER_HAS_DUAL_BRAKE;

	uint8_t version = (uint8_t)LOCO_VERSION_CODE(CONTROLLER_MODULE_VERSION);
	uint8_t magic = EEPROM.read(0);
	uint8_t schema = EEPROM.read(1);
	uint8_t stored_version = EEPROM.read(2);

	if (magic != CONTROLLER_PARAM_MAGIC || stored_version != version
		|| (schema != CONTROLLER_PARAM_SCHEMA && schema != CONTROLLER_PARAM_SCHEMA_V1)) {
		controller_params_defaults(_params, PLATFORM_ANALOG_RESOLUTION, dual, CONTROLLER_MODULE_VERSION);
		_save_params();
		return;
	}

	uint8_t stored_bytes = CONTROLLER_PARAM_BYTES;
	if (schema == CONTROLLER_PARAM_SCHEMA_V1) {
		stored_bytes = CONTROLLER_PARAM_BYTES_V1;
	}

	for (uint8_t i = 0; i < CONTROLLER_PARAM_BYTES; i++) {
		_params.bytes[i] = 0;
	}

	for (uint8_t i = 0; i < stored_bytes; i++) {
		_params.bytes[i] = EEPROM.read((int)(3 + i));
	}

	for (uint8_t i = 0; i < CONTROLLER_PARAM_NAME_LEN; i++) {
		_params.name[i] = (char)EEPROM.read((int)(3 + stored_bytes + i));
	}
	_params.name[CONTROLLER_PARAM_NAME_LEN] = 0;
	_params.bytes[CONTROLLER_PARAM_VERSION] = version;

	bool changed = false;

	if (schema == CONTROLLER_PARAM_SCHEMA_V1) {
		controller_params_init_gauges(_params);
		changed = true;
	}

	if (!dual && controller_params_levers_look_dual(_params, PLATFORM_ANALOG_RESOLUTION)) {
		controller_params_init_levers(_params, PLATFORM_ANALOG_RESOLUTION, false);
		changed = true;
	}

	if (changed) {
		_save_params();
	}
}


void MODULE_CONTROLLER::_save_params(void) {

	controller_eeprom_begin();

	controller_eeprom_put(0, (uint8_t)CONTROLLER_PARAM_MAGIC);
	controller_eeprom_put(1, (uint8_t)CONTROLLER_PARAM_SCHEMA);
	controller_eeprom_put(2, _params.bytes[CONTROLLER_PARAM_VERSION]);

	for (uint8_t i = 0; i < CONTROLLER_PARAM_BYTES; i++) {
		controller_eeprom_put((int)(3 + i), _params.bytes[i]);
	}

	for (uint8_t i = 0; i < CONTROLLER_PARAM_NAME_LEN; i++) {
		controller_eeprom_put((int)(3 + CONTROLLER_PARAM_BYTES + i), (uint8_t)_params.name[i]);
	}

	#ifdef MODULE_ARCH_ESP32
		EEPROM.commit();
	#endif
}


void MODULE_CONTROLLER::_apply_params(void) {

	uint16_t points[CONTROLLER_PARAM_LIGHT_POINTS];
	uint8_t point_count = 0;

	#ifdef CONTROLLER_MAINS_PORT
		point_count = controller_params_points(_params, CONTROLLER_PARAM_MAINS_COUNT, CONTROLLER_PARAM_MAINS_POINTS, points);
		_mains_switch.set_positions(points, point_count);
	#endif

	#ifdef CONTROLLER_DIR_PORT
		point_count = controller_params_points(_params, CONTROLLER_PARAM_DIR_COUNT, CONTROLLER_PARAM_DIR_POINTS, points);
		_dir_switch.set_positions(points, point_count);
	#endif

	#ifdef CONTROLLER_LIGHT_PORT
		point_count = controller_params_points(_params, CONTROLLER_PARAM_LIGHT_COUNT, CONTROLLER_PARAM_LIGHT_POINTS, points);
		_light_switch.set_positions(points, point_count);
	#endif

	#ifdef CONTROLLER_LIGHT2_PORT
		point_count = controller_params_points(_params, CONTROLLER_PARAM_LIGHT2_COUNT, CONTROLLER_PARAM_LIGHT2_POINTS, points);
		_light2_switch.set_positions(points, point_count);
	#endif

#if defined(CONTROLLER_BATTERY_VOLTAGE_PORT) || defined(CONTROLLER_MOTOR_VOLTAGE_PORT) || defined(CONTROLLER_CURRENT_PORT)
	for (uint8_t i = 0; i < _gauge_count; i++) {
		uint8_t mode_at = _gauges[i].param_mode;
		_gauges[i].mode = controller_gauge_mode(_params.bytes[mode_at]);
		_gauges[i].out_min = controller_params_get16(_params, (uint8_t)(mode_at + 1));
		_gauges[i].out_max = controller_params_get16(_params, (uint8_t)(mode_at + 3));
		pinMode(_gauges[i].port, OUTPUT);
		digitalWrite(_gauges[i].port, LOW);
	}
#endif

	(void)points;
	(void)point_count;
}


bool MODULE_CONTROLLER::_live_analog(uint8_t index, uint16_t &adc) {

	adc = 0;
	const int8_t channel = controller_params_live_channel(index);
	if (channel < 0) {
		return false;
	}

	switch (channel) {
#ifdef CONTROLLER_MAINS_PORT
		case 0:
			adc = _mains_switch.get_analog();
			return true;
#endif
#ifdef CONTROLLER_DIR_PORT
		case 1:
			adc = _dir_switch.get_analog();
			return true;
#endif
#ifdef CONTROLLER_LIGHT_PORT
		case 2:
			adc = _light_switch.get_analog();
			return true;
#endif
#ifdef CONTROLLER_LIGHT2_PORT
		case 3:
			adc = _light2_switch.get_analog();
			return true;
#endif
#ifdef CONTROLLER_DRIVE_PORT
		case 4:
			adc = analogRead(CONTROLLER_DRIVE_PORT);
			return true;
		case 5:
#if CONTROLLER_HAS_DUAL_BRAKE
			adc = analogRead(CONTROLLER_BREAK_PORT);
#else
			adc = analogRead(CONTROLLER_DRIVE_PORT);
#endif
			return true;
#endif
		default:
			return false;
	}
}


bool MODULE_CONTROLLER::_gauge_preview(uint8_t live_index, uint16_t &value) const {

#if defined(CONTROLLER_BATTERY_VOLTAGE_PORT) || defined(CONTROLLER_MOTOR_VOLTAGE_PORT) || defined(CONTROLLER_CURRENT_PORT)
	const uint8_t mode_at = controller_params_live_gauge_mode(live_index);
	if (mode_at == 0) {
		return false;
	}

	for (uint8_t i = 0; i < _gauge_count; i++) {
		if (_gauges[i].param_mode != mode_at) {
			continue;
		}
		value = _gauges[i].preview_on ? _gauges[i].preview : _gauges[i].out_min;
		return true;
	}
#else
	(void)live_index;
#endif
	(void)value;
	return false;
}


bool MODULE_CONTROLLER::_set_gauge_preview(uint8_t live_index, uint16_t value) {

#if defined(CONTROLLER_BATTERY_VOLTAGE_PORT) || defined(CONTROLLER_MOTOR_VOLTAGE_PORT) || defined(CONTROLLER_CURRENT_PORT)
	const uint8_t mode_at = controller_params_live_gauge_mode(live_index);
	if (mode_at == 0) {
		return false;
	}

	for (uint8_t i = 0; i < _gauge_count; i++) {
		if (_gauges[i].param_mode != mode_at) {
			continue;
		}
		uint16_t limit = _gauges[i].mode == CONTROLLER_GAUGE_ANALOG
			? (uint16_t)CONTROLLER_GAUGE_ANALOG_MAX
			: (uint16_t)CONTROLLER_GAUGE_SERVO_MAX;
		if (value > limit) {
			value = limit;
		}
		_gauges[i].preview = value;
		_gauges[i].preview_on = 1;
		return true;
	}
#else
	(void)live_index;
	(void)value;
#endif
	return false;
}


void MODULE_CONTROLLER::_handle_setup(CAN_MESSAGE message) {

	if (message.id == CAN_ID_REQUEST && message.size >= 3) {
		uint16_t target = ((uint16_t)message.data[0] << 8) | message.data[1];
		if (target == (uint16_t)can.uuid()) {
			uint16_t live = 0;
			if (_live_analog(message.data[2], live) || _gauge_preview(message.data[2], live)) {
				CAN_MESSAGE reply;
				memset(&reply, 0, sizeof(reply));
				reply.id = (uint32_t)(CAN_ID_REPLY | (message.data[2] & 0x7F));
				reply.uuid = (uint16_t)can.uuid();
				reply.size = 2;
				reply.data[0] = (uint8_t)(live & 0xFF);
				reply.data[1] = (uint8_t)(live >> 8);
				can.send(reply);
				return;
			}
		}
	}

	if ((message.id & 0x780) == CAN_ID_SETUP && message.size >= 4) {
		uint8_t index = (uint8_t)(message.id & 0x7F);
		uint16_t target = ((uint16_t)message.data[0] << 8) | message.data[1];
		if (target == (uint16_t)can.uuid() && controller_params_live_gauge_mode(index) != 0) {
			uint16_t value = (uint16_t)message.data[2] | ((uint16_t)message.data[3] << 8);
			if (_set_gauge_preview(index, value)) {
				CAN_MESSAGE reply;
				memset(&reply, 0, sizeof(reply));
				reply.id = (uint32_t)(CAN_ID_REPLY | (index & 0x7F));
				reply.uuid = (uint16_t)can.uuid();
				reply.size = 2;
				reply.data[0] = (uint8_t)(value & 0xFF);
				reply.data[1] = (uint8_t)(value >> 8);
				can.send(reply);
			}
			return;
		}
	}

	CONTROLLER_PARAM_RESULT result = controller_params_on_can(
		_params,
		message,
		(uint16_t)can.uuid(),
		CONTROLLER_MODULE_VERSION,
		MODULE_TYPE
	);

	for (uint8_t i = 0; i < result.reply_count && i < CAN_SETUP_REPLY_MAX; i++) {
		can.send(result.reply[i]);
	}

	if (result.changed) {
		_save_params();
		_apply_params();
	}
}


void MODULE_CONTROLLER::begin(void) {

	_drive_time.begin(CAN_ID_DRIVE_TIME);
	_heartbeat_time.begin(CAN_ID_DRIVE_HEARTBEAT_TIME);
	_foreign_timeout.begin((uint16_t)(5 * CAN_ID_DRIVE_HEARTBEAT_TIME));

	_status = CONTROLLER_STATUS_OFF;
	_nulled = false;
	_system_error = false;
	_horn_emergency = false;
	_can_emergency = false;
	_vehicle_error = false;
	_bus_moving = false;
	_foreign = false;
	_fault_frame = false;
	_fault_latched = false;
	_idle_frame = false;
	_setup_release = false;
	_setup_ignore_release = false;

	_mains = CONTROLLER_MAINS_OFF;
	_dir = CONTROLLER_DIR_NEUTRAL;
	_dir_latched = CONTROLLER_DIR_NEUTRAL;
	_horn = false;
	_horn2 = false;
	_horn_event = 0;
	_horn2_event = 0;
	_light_pos = 0;
	_light2_pos = 0;
	_drive_value = 0;
	_break_value = 0;
	_power_value = 0;
	_vehicle_count = 0;
	_setup_index = 0;

	for (uint8_t i = 0; i < CONTROLLER_MAX_VEHICLES; i++) {
		_vehicles[i].uuid = 0;
		_vehicles[i].status = 0;
		_vehicles[i].seen = 0;
	}

	_controller_flags.begin();

	#ifdef DEBUG
		Serial.println("*************************");
		Serial.println("start function/controller");
	#endif

	can.register_filter(0x760, CAN_ID_MODULE_CURRENT);
	can.register_filter(0x760, CAN_ID_DRIVE);
	can.register_filter(CAN_ID_MASK, CAN_ID_DRIVE_HEARTBEAT);
	can.register_filter(CAN_ID_MASK, CAN_ID_VEHICLE_STATUS);
	can.register_filter(0x7FF, CAN_ID_EMERGENCY);
	can.register_filter(0x740, CAN_ID_VOLTAGE);
	can.register_filter(0x7FF, CAN_ID_REQUEST);
	can.register_filter(0x780, CAN_ID_SETUP);

#if defined(CONTROLLER_BATTERY_VOLTAGE_PORT) || defined(CONTROLLER_MOTOR_VOLTAGE_PORT) || defined(CONTROLLER_CURRENT_PORT)
	_gauge_count = 0;

#ifdef CONTROLLER_BATTERY_VOLTAGE_PORT
	_gauges[_gauge_count].port = CONTROLLER_BATTERY_VOLTAGE_PORT;
	_gauges[_gauge_count].param_mode = CONTROLLER_PARAM_GAUGE_BATT_MODE;
	_gauges[_gauge_count].can_id = CAN_ID_BATT_VOLTAGE;
	_gauges[_gauge_count].signed_abs = 0;
	_gauges[_gauge_count].mode = CONTROLLER_GAUGE_SERVO;
	_gauges[_gauge_count].out_min = 0;
	_gauges[_gauge_count].out_max = CONTROLLER_GAUGE_SERVO_MAX;
	_gauges[_gauge_count].preview = 0;
	_gauges[_gauge_count].preview_on = 0;
	_gauges[_gauge_count].value = 0;
	_gauges[_gauge_count].full_scale = 1023;
	_gauges[_gauge_count].seen = 0;
	_gauge_count++;
#endif
#ifdef CONTROLLER_MOTOR_VOLTAGE_PORT
	_gauges[_gauge_count].port = CONTROLLER_MOTOR_VOLTAGE_PORT;
	_gauges[_gauge_count].param_mode = CONTROLLER_PARAM_GAUGE_MOTOR_MODE;
	_gauges[_gauge_count].can_id = CAN_ID_MOTOR_VOLTAGE;
	_gauges[_gauge_count].signed_abs = 1;
	_gauges[_gauge_count].mode = CONTROLLER_GAUGE_SERVO;
	_gauges[_gauge_count].out_min = 0;
	_gauges[_gauge_count].out_max = CONTROLLER_GAUGE_SERVO_MAX;
	_gauges[_gauge_count].preview = 0;
	_gauges[_gauge_count].preview_on = 0;
	_gauges[_gauge_count].value = 0;
	_gauges[_gauge_count].full_scale = 1023;
	_gauges[_gauge_count].seen = 0;
	_gauge_count++;
#endif
#ifdef CONTROLLER_CURRENT_PORT
	_gauges[_gauge_count].port = CONTROLLER_CURRENT_PORT;
	_gauges[_gauge_count].param_mode = CONTROLLER_PARAM_GAUGE_CURRENT_MODE;
	_gauges[_gauge_count].can_id = CAN_ID_MOTOR_CURRENT;
	_gauges[_gauge_count].signed_abs = 0;
	_gauges[_gauge_count].mode = CONTROLLER_GAUGE_SERVO;
	_gauges[_gauge_count].out_min = 0;
	_gauges[_gauge_count].out_max = CONTROLLER_GAUGE_SERVO_MAX;
	_gauges[_gauge_count].preview = 0;
	_gauges[_gauge_count].preview_on = 0;
	_gauges[_gauge_count].value = 0;
	_gauges[_gauge_count].full_scale = 1023;
	_gauges[_gauge_count].seen = 0;
	_gauge_count++;
#endif
#endif

	_load_params();

	#ifdef CONTROLLER_MAINS_PORT
		_mains_switch.begin(CONTROLLER_MAINS_PORT);
	#endif

	#ifdef CONTROLLER_DIR_PORT
		_dir_switch.begin(CONTROLLER_DIR_PORT);
	#endif

	#ifdef CONTROLLER_HORN_PORT
		#ifdef DEBUG
			Serial.print("> init horn button on port ");
			Serial.println(CONTROLLER_HORN_PORT);
		#endif
		_horn_switch.begin(CONTROLLER_HORN_PORT, BUTTON_LONG_PUSH_TIME);
	#endif

	#ifdef CONTROLLER_HORN2_PORT
		#ifdef DEBUG
			Serial.print("> init second horn button on port ");
			Serial.println(CONTROLLER_HORN2_PORT);
		#endif
		_horn2_switch.begin(CONTROLLER_HORN2_PORT, BUTTON_LONG_PUSH_TIME);
	#endif

	#ifdef CONTROLLER_LIGHT_PORT
		#ifdef DEBUG
			Serial.print("> init analog light switch on port ");
			Serial.println(CONTROLLER_LIGHT_PORT);
		#endif
		_light_switch.begin(CONTROLLER_LIGHT_PORT);
	#endif

	#ifdef CONTROLLER_LIGHT2_PORT
		#ifdef DEBUG
			Serial.print("> init second analog light switch on port ");
			Serial.println(CONTROLLER_LIGHT2_PORT);
		#endif
		_light2_switch.begin(CONTROLLER_LIGHT2_PORT);
	#endif

	#ifdef CONTROLLER_INSTRUMENT_LIGHT_PORT
		pinMode(CONTROLLER_INSTRUMENT_LIGHT_PORT, OUTPUT);
		digitalWrite(CONTROLLER_INSTRUMENT_LIGHT_PORT, LOW);
	#endif

#if defined(CONTROLLER_STATUS_RED_PORT) && defined(CONTROLLER_STATUS_GREEN_PORT)
	_status_led.begin(CONTROLLER_STATUS_RED_PORT, CONTROLLER_STATUS_GREEN_PORT);
	_led_mode = 0xFF;
#endif

	_apply_params();
}


void MODULE_CONTROLLER::_read_controls(void) {

	#ifdef CONTROLLER_MAINS_PORT
		_mains = _mains_switch.get();
	#else
		_mains = CONTROLLER_MAINS_OFF;
	#endif

	#ifdef CONTROLLER_DIR_PORT
		_dir = controller_dir_from_switch(_dir_switch.get(), _dir_switch.count());
	#else
		_dir = CONTROLLER_DIR_NEUTRAL;
	#endif

	#ifdef CONTROLLER_HORN_PORT
		_horn = _horn_switch.pushed();
		_horn_event = _horn_switch.check();
	#else
		_horn = false;
		_horn_event = 0;
	#endif

	#ifdef CONTROLLER_HORN2_PORT
		_horn2 = _horn2_switch.pushed();
		_horn2_event = _horn2_switch.check();
	#else
		_horn2 = false;
		_horn2_event = 0;
	#endif

	#ifdef CONTROLLER_LIGHT_PORT
		_light_pos = _light_switch.get();
	#else
		_light_pos = 0;
	#endif

	#ifdef CONTROLLER_LIGHT2_PORT
		_light2_pos = _light2_switch.get();
	#else
		_light2_pos = 0;
	#endif

	#ifdef CONTROLLER_DRIVE_PORT
		uint16_t drive_raw = analogRead(CONTROLLER_DRIVE_PORT);
		uint16_t drive_zero = controller_params_get16(_params, CONTROLLER_PARAM_DRIVE_ZERO);
		uint16_t drive_full = controller_params_get16(_params, CONTROLLER_PARAM_DRIVE_FULL);
		uint16_t brake_zero = controller_params_get16(_params, CONTROLLER_PARAM_BRAKE_ZERO);
		uint16_t brake_full = controller_params_get16(_params, CONTROLLER_PARAM_BRAKE_FULL);
		#if CONTROLLER_HAS_DUAL_BRAKE
			_drive_value = controller_map_axis(drive_raw, drive_zero, drive_full);
			_break_value = controller_map_axis(analogRead(CONTROLLER_BREAK_PORT), brake_zero, brake_full);
		#else
			controller_split_calibrated(
				drive_raw,
				drive_zero,
				drive_full,
				brake_zero,
				brake_full,
				_drive_value,
				_break_value
			);
		#endif
	#else
		_drive_value = 0;
		_break_value = 0;
	#endif

	#ifdef CONTROLLER_POWER_PORT
		_power_value = controller_scale_10bit(analogRead(CONTROLLER_POWER_PORT), PLATFORM_ANALOG_RESOLUTION);
	#else
		_power_value = (_drive_value > 0) ? 1023 : 0;
	#endif
}


void MODULE_CONTROLLER::_note_vehicle(uint16_t uuid, uint8_t status) {

	if (uuid == 0 || uuid == (uint16_t)can.uuid()) {
		return;
	}

	uint32_t now = millis();

	for (uint8_t i = 0; i < CONTROLLER_MAX_VEHICLES; i++) {
		if (_vehicles[i].uuid == uuid) {
			_vehicles[i].status = status;
			_vehicles[i].seen = now;
			return;
		}
	}

	for (uint8_t i = 0; i < CONTROLLER_MAX_VEHICLES; i++) {
		if (_vehicles[i].uuid == 0) {
			_vehicles[i].uuid = uuid;
			_vehicles[i].status = status;
			_vehicles[i].seen = now;
			return;
		}
	}
}


void MODULE_CONTROLLER::_refresh_vehicles(void) {

	uint32_t now = millis();

	_vehicle_count = 0;
	_bus_moving = false;
	_vehicle_error = false;

	for (uint8_t i = 0; i < CONTROLLER_MAX_VEHICLES; i++) {
		if (_vehicles[i].uuid == 0) {
			continue;
		}

		if ((uint32_t)(now - _vehicles[i].seen) > CONTROLLER_VEHICLE_TIMEOUT) {
			_vehicles[i].uuid = 0;
			_vehicles[i].status = 0;
			_vehicles[i].seen = 0;
			continue;
		}

		_vehicle_count++;

		if (_vehicles[i].status & (uint8_t)(1 << MOVING_FLAG)) {
			_bus_moving = true;
		}
		if (_vehicles[i].status & (uint8_t)(1 << ERROR_FLAG)) {
			_vehicle_error = true;
		}
	}

	if (_vehicle_count > 0 && _setup_index >= _vehicle_count) {
		_setup_index = 0;
	}
}


void MODULE_CONTROLLER::_handle_can(CAN_MESSAGE message) {

	if (message.uuid == 0 || message.uuid == (uint16_t)can.uuid()) {
		return;
	}

	switch (message.id) {

		case CAN_ID_DRIVE:
		case CAN_ID_DRIVE_HEARTBEAT:
			_foreign = true;
			_foreign_timeout.retrigger();
			break;

		case CAN_ID_VEHICLE_STATUS:
			if (message.size >= 1) {
				_note_vehicle(message.uuid, message.data[0]);
			}
			break;

		case CAN_ID_EMERGENCY:
			_can_emergency = true;
			break;

		case CAN_ID_LIGHT:
			if (!controller_is_commanding(_status) && message.size >= 1) {
				_apply_instrument(message.data[0]);
			}
			break;

		case CAN_ID_BATT_VOLTAGE:
		case CAN_ID_MOTOR_VOLTAGE:
		case CAN_ID_MOTOR_CURRENT:
			_note_gauge(message.id, message.uuid, message.data, message.size);
			break;

		default:
			break;
	}
}


uint16_t MODULE_CONTROLLER::_main_loco(void) {

	uint8_t want = 0;

	if (_status == CONTROLLER_STATUS_SETUP) {
		want = _setup_index;
	}

	uint8_t seen = 0;

	for (uint8_t i = 0; i < CONTROLLER_MAX_VEHICLES; i++) {
		if (_vehicles[i].uuid == 0) {
			continue;
		}
		if (seen == want) {
			return _vehicles[i].uuid;
		}
		seen++;
	}

	return 0;
}


void MODULE_CONTROLLER::_step_setup(uint8_t previous) {

	if (previous != CONTROLLER_STATUS_SETUP && _status == CONTROLLER_STATUS_SETUP) {
		_setup_index = 0;
		_setup_ignore_release = true;
	}

	if (previous == CONTROLLER_STATUS_SETUP && _status != CONTROLLER_STATUS_SETUP) {
		_setup_release = true;
	}

	if (_status != CONTROLLER_STATUS_SETUP) {
		return;
	}

	bool step = false;

	if (_horn_event != 0 || _horn2_event != 0) {
		if (_setup_ignore_release) {
			_setup_ignore_release = false;
		}
		else if (_horn_event == CONTROLLER_BUTTON_SHORT || _horn2_event == CONTROLLER_BUTTON_SHORT) {
			step = true;
		}
	}

	if (step && _vehicle_count > 0) {
		_setup_index++;
		if (_setup_index >= _vehicle_count) {
			_setup_index = 0;
		}
	}
}


void MODULE_CONTROLLER::_note_gauge(uint16_t id, uint16_t uuid, const uint8_t *data, uint8_t size) {

#if defined(CONTROLLER_BATTERY_VOLTAGE_PORT) || defined(CONTROLLER_MOTOR_VOLTAGE_PORT) || defined(CONTROLLER_CURRENT_PORT)
	uint16_t loco = _main_loco();
	if (loco != 0 && uuid != loco) {
		return;
	}

	for (uint8_t i = 0; i < _gauge_count; i++) {
		if (_gauges[i].can_id != id) {
			continue;
		}

		uint16_t value = 0;
		uint16_t full_scale = 1023;
		if (!controller_gauge_unpack(data, size, _gauges[i].signed_abs, value, full_scale)) {
			return;
		}

		_gauges[i].value = value;
		_gauges[i].full_scale = full_scale;
		uint16_t now = (uint16_t)millis();
		_gauges[i].seen = now == 0 ? 1 : now;
		return;
	}
#else
	(void)id;
	(void)uuid;
	(void)data;
	(void)size;
#endif
}


void MODULE_CONTROLLER::_write_gauges(bool park) {

#if defined(CONTROLLER_BATTERY_VOLTAGE_PORT) || defined(CONTROLLER_MOTOR_VOLTAGE_PORT) || defined(CONTROLLER_CURRENT_PORT)
	static uint16_t servo_ms = 0;
	uint16_t now = (uint16_t)millis();
	bool pulse = (uint16_t)(now - servo_ms) >= 20;
	if (pulse) {
		servo_ms = now;
	}

	for (uint8_t i = 0; i < _gauge_count; i++) {
		if (!park) {
			_gauges[i].preview_on = 0;
		}

		bool stale = _gauges[i].seen == 0
			|| (uint16_t)(now - _gauges[i].seen) > CONTROLLER_GAUGE_TIMEOUT;
		uint16_t out = _gauges[i].out_min;

		if (park && _gauges[i].preview_on) {
			out = _gauges[i].preview;
		}
		else if (!park && !stale) {
			out = controller_gauge_output(
				_gauges[i].value,
				_gauges[i].full_scale,
				_gauges[i].out_min,
				_gauges[i].out_max
			);
		}

		if (_gauges[i].mode == CONTROLLER_GAUGE_SERVO) {
			if (!pulse) {
				continue;
			}
			if (out > CONTROLLER_GAUGE_SERVO_MAX) {
				out = CONTROLLER_GAUGE_SERVO_MAX;
			}
			uint16_t us = (uint16_t)(544 + (((uint32_t)out * 1856UL) / 180));
			digitalWrite(_gauges[i].port, HIGH);
			delayMicroseconds(us);
			digitalWrite(_gauges[i].port, LOW);
		}
		else {
			if (out > CONTROLLER_GAUGE_ANALOG_MAX) {
				out = CONTROLLER_GAUGE_ANALOG_MAX;
			}
			analogWrite(_gauges[i].port, (uint8_t)out);
		}
	}
#else
	(void)park;
#endif
}


void MODULE_CONTROLLER::_apply_instrument(uint8_t light) {

	#ifdef CONTROLLER_INSTRUMENT_LIGHT_PORT
		digitalWrite(CONTROLLER_INSTRUMENT_LIGHT_PORT, (light & (uint8_t)(1 << LIGHT_INSTR)) ? HIGH : LOW);
	#else
		(void)light;
	#endif
}


void MODULE_CONTROLLER::_send_drive(bool fault) {

	CONTROLLER_DRIVE drive;
	bool speed = controller_transmits_speed(_status) && !fault;

	drive.error = fault;
	drive.aux1 = false;
	drive.aux = false;
	drive.down = (!fault && _horn2);
	drive.up = (!fault && _horn);
	drive.reverse = (!fault && _dir_latched == CONTROLLER_DIR_REVERSE);
	drive.motor = (!fault && controller_motor_on(_status));
	drive.mains = (!fault && _mains != CONTROLLER_MAINS_OFF && controller_is_commanding(_status));
	drive.multi = (!fault && _vehicle_count > 1);
	drive.loco = fault ? 0 : _main_loco();
	drive.drive = speed ? _drive_value : 0;
	drive.power = speed ? _power_value : 0;
	drive.brake = fault ? 1023 : _break_value;

	_message.id = CAN_ID_DRIVE;
	_message.size = 8;
	controller_pack_drive(drive, _message.data);
	can.send(_message);
}


void MODULE_CONTROLLER::_send_signal(void) {

	uint8_t signal = 0;

	if (_horn) {
		signal |= (uint8_t)(1 << SIGNAL_HIGH);
	}
	if (_horn2) {
		signal |= (uint8_t)(1 << SIGNAL_LOW);
	}

	_message.id = CAN_ID_SIGNAL;
	_message.size = 1;
	_message.data[0] = signal;
	can.send(_message);
}


void MODULE_CONTROLLER::_send_light(void) {

	uint8_t light = (uint8_t)(light_from_position(_light_pos) | light2_from_position(_light2_pos));

	_message.id = CAN_ID_LIGHT;
	_message.size = 1;
	_message.data[0] = light;
	can.send(_message);

	_apply_instrument(light);
}


void MODULE_CONTROLLER::_send_setup(bool disable) {

	uint16_t loco = disable ? 0 : _main_loco();
	uint8_t flags = 0;

	if (disable) {
		flags |= (uint8_t)(1 << 1);
	}
	else if (_dir != CONTROLLER_DIR_NEUTRAL) {
		flags |= (uint8_t)(1 << 0);
	}

	_message.id = CAN_ID_LOCO_SETUP;
	_message.size = 3;
	_message.data[0] = flags;
	_message.data[1] = (uint8_t)(loco & 0xFF);
	_message.data[2] = (uint8_t)((loco >> 8) & 0xFF);
	can.send(_message);
}


#if defined(CONTROLLER_STATUS_RED_PORT) && defined(CONTROLLER_STATUS_GREEN_PORT)

#define CONTROLLER_LED_OFF_DARK 1
#define CONTROLLER_LED_OFF_BUS 2
#define CONTROLLER_LED_FOREIGN 3
#define CONTROLLER_LED_ERROR 4
#define CONTROLLER_LED_STANDBY 5
#define CONTROLLER_LED_STANDBY_NOLOCO 6
#define CONTROLLER_LED_NULLED 7
#define CONTROLLER_LED_ON 8
#define CONTROLLER_LED_READY 9
#define CONTROLLER_LED_MOVING 10
#define CONTROLLER_LED_SETUP 11

void MODULE_CONTROLLER::_update_led(void) {

	uint8_t mode = CONTROLLER_LED_OFF_DARK;

	if (_system_error && !controller_is_commanding(_status)) {
		mode = CONTROLLER_LED_ERROR;
	}
	else {
		switch (_status) {
			case CONTROLLER_STATUS_LOCKED:
				mode = CONTROLLER_LED_FOREIGN;
				break;
			case CONTROLLER_STATUS_STANDBY:
				mode = (_vehicle_count == 0) ? CONTROLLER_LED_STANDBY_NOLOCO : CONTROLLER_LED_STANDBY;
				break;
			case CONTROLLER_STATUS_NULLED:
				mode = CONTROLLER_LED_NULLED;
				break;
			case CONTROLLER_STATUS_ON:
				mode = CONTROLLER_LED_ON;
				break;
			case CONTROLLER_STATUS_READY:
				mode = CONTROLLER_LED_READY;
				break;
			case CONTROLLER_STATUS_MOVING:
				mode = CONTROLLER_LED_MOVING;
				break;
			case CONTROLLER_STATUS_SETUP:
				mode = CONTROLLER_LED_SETUP;
				break;
			case CONTROLLER_STATUS_OFF:
			default:
				if (_foreign) {
					mode = CONTROLLER_LED_FOREIGN;
				}
				else if (can.alive()) {
					mode = CONTROLLER_LED_OFF_BUS;
				}
				else {
					mode = CONTROLLER_LED_OFF_DARK;
				}
				break;
		}
	}

	if (mode != _led_mode) {
		_led_mode = mode;

		switch (mode) {
			case CONTROLLER_LED_OFF_BUS:
				_status_led.color(INTELLILED_GREEN);
				_status_led.flash(1000);
				break;
			case CONTROLLER_LED_FOREIGN:
				_status_led.color(INTELLILED_RED);
				_status_led.on();
				break;
			case CONTROLLER_LED_ERROR:
			case CONTROLLER_LED_NULLED:
				_status_led.color(INTELLILED_RED);
				_status_led.blink(250);
				break;
			case CONTROLLER_LED_STANDBY_NOLOCO:
				_status_led.color(INTELLILED_RED);
				_status_led.blink(500);
				break;
			case CONTROLLER_LED_STANDBY:
				_status_led.color(INTELLILED_YELLOW);
				_status_led.blink(500);
				break;
			case CONTROLLER_LED_ON:
				_status_led.color(INTELLILED_YELLOW);
				_status_led.on();
				break;
			case CONTROLLER_LED_READY:
				_status_led.color(INTELLILED_GREEN);
				_status_led.blink(500);
				break;
			case CONTROLLER_LED_MOVING:
				_status_led.color(INTELLILED_GREEN);
				_status_led.on();
				break;
			case CONTROLLER_LED_SETUP:
				_status_led.color(INTELLILED_YELLOW, INTELLILED_GREEN);
				_status_led.blink(250);
				break;
			case CONTROLLER_LED_OFF_DARK:
			default:
				_status_led.color(INTELLILED_RED);
				_status_led.flash(1000);
				break;
		}
	}

	_status_led.update();
}

#endif


void MODULE_CONTROLLER::update(CAN_MESSAGE message) {

	if (loco_setup.active) {
		if (message.id == CAN_ID_REQUEST || (message.id & 0x780) == CAN_ID_SETUP) {
			_handle_setup(message);
		}
		_write_gauges(true);
		return;
	}

	if (message.uuid != 0) {
		/* #ifdef DEVEL
			Serial.print("got message ");
			can.print_message(message);
		#endif */
		_handle_can(message);
	}

	_refresh_vehicles();
	_read_controls();
	_write_gauges(false);

	if (_horn && _horn2 && _mains != CONTROLLER_MAINS_OFF) {
		_horn_emergency = true;
	}

	_system_error = _horn_emergency || _can_emergency || _vehicle_error;

	bool bus_free = _foreign_timeout.check();
	if (bus_free) {
		_foreign = false;
	}
	_controller_flags.set(CONTROLLER_BLOCKED, _foreign);

	CONTROLLER_INPUTS in;
	in.mains = _mains;
	in.dir = _dir;
	in.horn = _horn;
	in.horn2 = _horn2;
	in.drive_zero = (_drive_value <= CONTROLLER_ZERO_THRESHOLD);
	in.bus_free = bus_free;
	in.foreign = _foreign;
	in.bus_moving = _bus_moving;
	in.emergency = _horn_emergency || _can_emergency;
	in.vehicle_error = _vehicle_error;
	in.fault_latched = _fault_latched;

	uint8_t previous = _status;
	_status = controller_next_status(_status, in, _nulled);

	if (!(_status == CONTROLLER_STATUS_MOVING && previous == CONTROLLER_STATUS_MOVING)) {
		_dir_latched = _dir;
	}

	if (controller_is_commanding(previous) && _status == CONTROLLER_STATUS_OFF) {
		if (in.emergency || (controller_motor_on(previous) && _vehicle_error)) {
			_fault_frame = true;
			_fault_latched = true;
		}
		else if (_mains == CONTROLLER_MAINS_OFF) {
			_idle_frame = true;
		}
	}

	_step_setup(previous);

	if (_mains == CONTROLLER_MAINS_OFF) {
		_horn_emergency = false;
		_can_emergency = false;
		_fault_latched = false;
		_system_error = false;
	}

	#ifdef DEBUG
		if (previous != _status) {
			Serial.print("> controller status ");
			Serial.print(previous);
			Serial.print(" -> ");
			Serial.println(_status);
		}
	#endif

#if defined(CONTROLLER_STATUS_RED_PORT) && defined(CONTROLLER_STATUS_GREEN_PORT)
	_update_led();
#endif

	if (_fault_frame) {
		_send_drive(true);
		_fault_frame = false;
	}

	if (_idle_frame) {
		_send_drive(false);
		_idle_frame = false;
	}

	if (_setup_release) {
		_send_setup(true);
		_setup_release = false;
	}

	if (!controller_is_commanding(_status)) {
		return;
	}

	if (_heartbeat_time.check()) {
		_heartbeat_time.retrigger();
		_message.id = CAN_ID_DRIVE_HEARTBEAT;
		_message.size = 0;
		can.send(_message);
	}

	if (_drive_time.check()) {
		_drive_time.retrigger();
		_send_drive(false);
		_send_signal();
		_send_light();

		if (_status == CONTROLLER_STATUS_SETUP) {
			_send_setup(false);
		}
	}
}

#endif
