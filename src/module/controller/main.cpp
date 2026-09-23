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
#include "../../core/servo/intelliServo.h"
#include <EEPROM.h>


extern CAN_COM can;


/* intelliButton::check() returns 2 for a short release and 1 for a long one. */
#define CONTROLLER_BUTTON_SHORT 2

#define CONTROLLER_VEHICLE_TIMEOUT (MODULE_HEARTBEAT_TIMEOUT * 2)
#define CONTROLLER_EEPROM_BYTES 80


#ifdef MODULE_CONTROLLER_CONFIG_H

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

	bool dual = false;
	#ifdef CONTROLLER_BREAK_PORT
		dual = true;
	#endif

	uint8_t version = (uint8_t)CONTROLLER_MODULE_VERSION;
	uint8_t magic = EEPROM.read(0);
	uint8_t schema = EEPROM.read(1);
	uint8_t stored_version = EEPROM.read(2);

	if (magic != CONTROLLER_PARAM_MAGIC || schema != CONTROLLER_PARAM_SCHEMA || stored_version != version) {
		controller_params_defaults(_params, PLATFORM_ANALOG_RESOLUTION, dual, version);
		_save_params();
		return;
	}

	for (uint8_t i = 0; i < CONTROLLER_PARAM_BYTES; i++) {
		_params.bytes[i] = EEPROM.read((int)(3 + i));
	}

	for (uint8_t i = 0; i < CONTROLLER_PARAM_NAME_LEN; i++) {
		_params.name[i] = (char)EEPROM.read((int)(3 + CONTROLLER_PARAM_BYTES + i));
	}
	_params.name[CONTROLLER_PARAM_NAME_LEN] = 0;
	_params.bytes[CONTROLLER_PARAM_VERSION] = version;
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

	(void)points;
	(void)point_count;

#ifdef CONTROLLER_HAS_GAUGES
	_refresh_gauges();
#endif
}


void MODULE_CONTROLLER::_handle_setup(CAN_MESSAGE message) {

	CONTROLLER_PARAM_RESULT result = controller_params_on_can(
		_params,
		message,
		(uint16_t)can.uuid(),
		(uint8_t)CONTROLLER_MODULE_VERSION,
		CONTROLLER_TYPE_ID
	);

	for (uint8_t i = 0; i < result.reply_count && i < 2; i++) {
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

#ifdef CONTROLLER_HAS_GAUGES
	for (uint8_t channel = 0; channel < CONTROLLER_GAUGE_COUNT; channel++) {
		_gauge_mode[channel] = 0xFF;
		_gauge_duty[channel] = 0xFFFF;
	}
	for (uint8_t row = 0; row < CONTROLLER_GAUGE_ROWS; row++) {
		for (uint8_t i = 0; i < CONTROLLER_GAUGE_SLOTS; i++) {
			_gauge_samples[row][i].uuid = 0;
			_gauge_samples[row][i].percentage = 0;
			_gauge_samples[row][i].reference = 0;
			_gauge_samples[row][i].seen = 0;
		}
	}
#endif

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

	can.register_filter(CAN_ID_MASK, CAN_ID_LIGHT_CURRENT);
	can.register_filter(CAN_ID_MASK, CAN_ID_DRIVE);
	can.register_filter(CAN_ID_MASK, CAN_ID_DRIVE_HEARTBEAT);
	can.register_filter(CAN_ID_MASK, CAN_ID_VEHICLE_STATUS);
	can.register_filter(0x7FF, CAN_ID_EMERGENCY);
	can.register_filter(CAN_ID_MASK, CAN_ID_LIGHT);
	can.register_filter(0x7FF, CAN_ID_REQUEST);
	can.register_filter(0x780, CAN_ID_SETUP);
#ifdef CONTROLLER_HAS_GAUGES
	can.register_filter(0x700, CAN_ID_CURRENT);
	can.register_filter(0x700, CAN_ID_VOLTAGE);
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
		_dir = _dir_switch.get();
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
		#ifdef CONTROLLER_BREAK_PORT
			_drive_value = controller_map_axis(drive_raw, drive_zero, drive_full);
			_break_value = controller_map_axis(analogRead(CONTROLLER_BREAK_PORT), brake_zero, brake_full);
		#else
			(void)brake_zero;
			controller_split_calibrated(drive_raw, brake_full, drive_zero, drive_full, _drive_value, _break_value);
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

		default:
#ifdef CONTROLLER_HAS_GAUGES
			_note_gauge(message);
#endif
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
	drive.brake = fault ? 1023 : (speed ? _break_value : 0);

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


#ifdef CONTROLLER_HAS_GAUGES

#if defined(MODULE_ARCH_AVR)

static volatile uint8_t controller_soft_pin[CONTROLLER_GAUGE_COUNT];
static volatile uint8_t controller_soft_duty[CONTROLLER_GAUGE_COUNT];
static volatile uint8_t controller_soft_used = 0;

ISR(TIMER2_OVF_vect) {

	static uint8_t phase = 0;
	phase++;

	uint8_t n = controller_soft_used;
	for (uint8_t i = 0; i < n; i++) {
		if (phase < controller_soft_duty[i]) {
			digitalWrite(controller_soft_pin[i], HIGH);
		}
		else {
			digitalWrite(controller_soft_pin[i], LOW);
		}
	}
}

static void controller_soft_timer_on(void) {

	static bool started = false;

	if (!started) {
		started = true;
		TCCR2A = 0;
		TCCR2B = (1 << CS22);
		TCNT2 = 0;
	}

	TIMSK2 |= (1 << TOIE2);
}

static int8_t controller_soft_find(uint8_t pin) {

	for (uint8_t i = 0; i < controller_soft_used; i++) {
		if (controller_soft_pin[i] == pin) {
			return (int8_t)i;
		}
	}

	return -1;
}

static void controller_soft_clear(uint8_t pin) {

	uint8_t saved = SREG;
	cli();

	int8_t idx = controller_soft_find(pin);
	if (idx >= 0) {
		for (uint8_t i = (uint8_t)idx; (uint8_t)(i + 1) < controller_soft_used; i++) {
			controller_soft_pin[i] = controller_soft_pin[i + 1];
			controller_soft_duty[i] = controller_soft_duty[i + 1];
		}
		controller_soft_used--;
	}

	if (controller_soft_used == 0) {
		TIMSK2 &= (uint8_t)~(1 << TOIE2);
	}

	SREG = saved;
	digitalWrite(pin, LOW);
}

static void controller_soft_set(uint8_t pin, uint8_t duty) {

	pinMode(pin, OUTPUT);

	uint8_t saved = SREG;
	cli();

	int8_t idx = controller_soft_find(pin);
	if (idx < 0 && controller_soft_used < CONTROLLER_GAUGE_COUNT) {
		idx = (int8_t)controller_soft_used;
		controller_soft_pin[idx] = pin;
		controller_soft_used++;
	}

	if (idx >= 0) {
		controller_soft_duty[idx] = duty;
	}

	SREG = saved;
	controller_soft_timer_on();
}

#endif

static bool controller_gauge_pin_pwm(uint8_t pin) {

#if defined(MODULE_ARCH_AVR)
	switch (pin) {
		case 3:
		case 5:
		case 6:
		case 9:
		case 10:
		case 11:
			return true;
		default:
			return false;
	}
#else
	(void)pin;
	return true;
#endif
}

static void controller_gauge_analog(uint8_t pin, uint8_t duty) {

	if (controller_gauge_pin_pwm(pin)) {
#if defined(MODULE_ARCH_AVR)
		controller_soft_clear(pin);
#endif
		pinMode(pin, OUTPUT);
		analogWrite(pin, duty);
		return;
	}

#if defined(MODULE_ARCH_AVR)
	controller_soft_set(pin, duty);
#else
	pinMode(pin, OUTPUT);
	analogWrite(pin, duty);
#endif
}

#ifdef CONTROLLER_BATTERY_VOLTAGE_PORT
static INTELLISERVO controller_gauge_batt_servo;
#endif
#ifdef CONTROLLER_MOTOR_VOLTAGE_PORT
static INTELLISERVO controller_gauge_motor_v_servo;
#endif
#if defined(CONTROLLER_CURRENT_PORT) || defined(CONTROLLER_BATTERY_CURRENT_PORT)
static INTELLISERVO controller_gauge_current_servo;
#endif
#ifdef CONTROLLER_MOTOR_CURRENT_PORT
static INTELLISERVO controller_gauge_motor_i_servo;
#endif

static INTELLISERVO *controller_gauge_servo(uint8_t channel) {

	switch (channel) {
#ifdef CONTROLLER_BATTERY_VOLTAGE_PORT
		case CONTROLLER_GAUGE_BATT_VOLTAGE:
			return &controller_gauge_batt_servo;
#endif
#ifdef CONTROLLER_MOTOR_VOLTAGE_PORT
		case CONTROLLER_GAUGE_MOTOR_VOLTAGE:
			return &controller_gauge_motor_v_servo;
#endif
#if defined(CONTROLLER_CURRENT_PORT) || defined(CONTROLLER_BATTERY_CURRENT_PORT)
		case CONTROLLER_GAUGE_CURRENT:
			return &controller_gauge_current_servo;
#endif
#ifdef CONTROLLER_MOTOR_CURRENT_PORT
		case CONTROLLER_GAUGE_MOTOR_CURRENT:
			return &controller_gauge_motor_i_servo;
#endif
		default:
			return 0;
	}
}

int16_t MODULE_CONTROLLER::_gauge_port(uint8_t channel) {

	switch (channel) {
		case CONTROLLER_GAUGE_BATT_VOLTAGE:
#ifdef CONTROLLER_BATTERY_VOLTAGE_PORT
			return CONTROLLER_BATTERY_VOLTAGE_PORT;
#else
			return -1;
#endif
		case CONTROLLER_GAUGE_MOTOR_VOLTAGE:
#ifdef CONTROLLER_MOTOR_VOLTAGE_PORT
			return CONTROLLER_MOTOR_VOLTAGE_PORT;
#else
			return -1;
#endif
		case CONTROLLER_GAUGE_CURRENT:
#if defined(CONTROLLER_CURRENT_PORT)
			return CONTROLLER_CURRENT_PORT;
#elif defined(CONTROLLER_BATTERY_CURRENT_PORT)
			return CONTROLLER_BATTERY_CURRENT_PORT;
#else
			return -1;
#endif
		case CONTROLLER_GAUGE_MOTOR_CURRENT:
#ifdef CONTROLLER_MOTOR_CURRENT_PORT
			return CONTROLLER_MOTOR_CURRENT_PORT;
#else
			return -1;
#endif
		default:
			return -1;
	}
}

void MODULE_CONTROLLER::_note_gauge(CAN_MESSAGE message) {

	uint8_t channel = controller_gauge_channel(message.id);
	if (channel == CONTROLLER_GAUGE_NONE) {
		return;
	}

	uint16_t percentage = 0;
	uint16_t reference = 0;
	uint8_t index = 0;
	if (!controller_gauge_decode(message.data, message.size, percentage, reference, index)) {
		return;
	}

	uint8_t row = controller_gauge_row(channel);
	if (row == CONTROLLER_GAUGE_NONE) {
		return;
	}

	(void)index;
	controller_gauge_note(
		_gauge_samples[row],
		CONTROLLER_GAUGE_SLOTS,
		message.uuid,
		percentage,
		reference,
		(uint16_t)millis(),
		(uint16_t)CONTROLLER_VEHICLE_TIMEOUT
	);
}

void MODULE_CONTROLLER::_refresh_gauges(void) {

	uint16_t now = (uint16_t)millis();

	for (uint8_t channel = 0; channel < CONTROLLER_GAUGE_COUNT; channel++) {
		int16_t port = _gauge_port(channel);
		uint8_t row = controller_gauge_row(channel);
		if (port < 0 || row == CONTROLLER_GAUGE_NONE) {
			continue;
		}

		uint8_t pin = (uint8_t)port;
		uint8_t mode = _params.bytes[controller_gauge_mode_index(channel)];
		if (mode != CONTROLLER_GAUGE_MODE_SERVO) {
			mode = CONTROLLER_GAUGE_MODE_ANALOG;
		}

		uint16_t full = CONTROLLER_GAUGE_ANALOG_FULL;
		if (mode == CONTROLLER_GAUGE_MODE_SERVO) {
			full = CONTROLLER_GAUGE_SERVO_FULL;
		}

		uint16_t percentage = 0;
		uint16_t source_reference = 0;
		bool live = controller_gauge_select(
			_gauge_samples[row],
			CONTROLLER_GAUGE_SLOTS,
			controller_gauge_reduce(channel),
			now,
			(uint16_t)CONTROLLER_VEHICLE_TIMEOUT,
			percentage,
			source_reference
		);

		uint16_t duty = 0;
		if (live) {
			duty = controller_gauge_output(
				percentage,
				source_reference,
				controller_params_get16(_params, controller_gauge_ref_index(channel)),
				full
			);
		}

		if (_gauge_mode[channel] != mode) {
			INTELLISERVO *servo = controller_gauge_servo(channel);
			if (mode == CONTROLLER_GAUGE_MODE_SERVO) {
#if defined(MODULE_ARCH_AVR)
				controller_soft_clear(pin);
#else
				digitalWrite(pin, LOW);
#endif
				if (servo != 0) {
					servo->begin(pin);
					servo->set_limits(0, CONTROLLER_GAUGE_SERVO_FULL);
					servo->set_value_limits(0, CONTROLLER_GAUGE_SERVO_FULL);
				}
			}
			else if (servo != 0) {
				servo->end();
			}

			_gauge_mode[channel] = mode;
			_gauge_duty[channel] = 0xFFFF;
		}

		if (_gauge_duty[channel] == duty) {
			continue;
		}

		_gauge_duty[channel] = duty;

		if (mode == CONTROLLER_GAUGE_MODE_SERVO) {
			INTELLISERVO *servo = controller_gauge_servo(channel);
			if (servo != 0) {
				servo->set(duty);
			}
		}
		else {
			controller_gauge_analog(pin, (uint8_t)duty);
		}
	}
}

#endif


void MODULE_CONTROLLER::update(CAN_MESSAGE message) {

	if (message.id == CAN_ID_REQUEST || (message.id & 0x780) == CAN_ID_SETUP) {
		_handle_setup(message);
	}
	else if (message.uuid != 0) {
		#ifdef DEVEL
			Serial.print("got message ");
			can.print_message(message);
		#endif
		_handle_can(message);
	}

	_refresh_vehicles();
	_read_controls();

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
	in.system_error = _system_error;

	uint8_t previous = _status;
	_status = controller_next_status(_status, in, _nulled);

	if (!(_status == CONTROLLER_STATUS_MOVING && previous == CONTROLLER_STATUS_MOVING)) {
		_dir_latched = _dir;
	}

	if (previous != CONTROLLER_STATUS_OFF && previous != CONTROLLER_STATUS_LOCKED
		&& _status == CONTROLLER_STATUS_OFF && _system_error) {
		_fault_frame = true;
	}

	_step_setup(previous);

	if (_mains == CONTROLLER_MAINS_OFF) {
		_horn_emergency = false;
		_can_emergency = false;
		_system_error = _vehicle_error;
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

	if (_setup_release) {
		_send_setup(true);
		_setup_release = false;
	}

#ifdef CONTROLLER_HAS_GAUGES
	_refresh_gauges();
#endif

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
