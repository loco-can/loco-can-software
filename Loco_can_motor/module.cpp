/*
 * LOCO-CAN LK-motor driver
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */


#include "module.h"
#include "config.h"

#include "can_com.h"
#include "can_protocol.h"

#include "intelliled.h"
#include "pwm_ramp.h"
#include "flags.h"

extern CAN_COM can_com;



/* *************************************************
 * class construtors
 */
MODULE::MODULE(void) {
}


// start class with user acceleration values
void MODULE::begin(uint16_t ramp_time) {

	pinMode(RELAIS_F, OUTPUT);
	pinMode(RELAIS_R, OUTPUT);

	#ifdef RELAIS_MAIN
		pinMode(RELAIS_MAIN, OUTPUT);
	#endif

	_set_ramp(MOTOR_DEFAULT_RAMP);

	_set_speed(0);
	_set_dir(0);

	_current_speed = 0;
	_current_dir = 0;

	
	// reset direction outputs
	_clear_dir();

	// start drive PWM
	_pwm_drive.begin(PWM_DRIVE, PWM_RAMP_UPDATE_RATE, DRIVE_FREQUENCY);
	_pwm_drive.set_ramp(MOTOR_DEFAULT_RAMP);

	// start break PWM
	_pwm_break.begin(PWM_BREAK, PWM_RAMP_UPDATE_RATE, BREAK_FREQUENCY);
	_pwm_break.set_ramp(MOTOR_DEFAULT_RAMP);


	// set initial status
	_drive_nulled = false;
	_emergency = false;
	_standing = false;
	_motor_status = MOTOR_STATUS_EMERGENCY;

	// start drive data and standing and voltage timeout
	_heartbeat_timeout.begin(1000);
	_standing_timeout.begin(1000);
	_voltage_timeout.begin(1000);

	// start voltage
	_voltage.begin(VOLTAGE_0, VOLTAGE_1, ANALOG_RESOLUTION, 30);
	_voltage.set_filter(0.25);
}


/* *************************************************
 * update methods
 */
void MODULE::update(void) {

	CAN_MESSAGE message;


	_receive(message);

	_set_status(message);

}


// get can messages
void MODULE::_receive(CAN_MESSAGE) {

	uint16_t speed_val;
	uint16_t break_val;
	uint16_t power_val;

	uint16_t multi_uuid;

	uint32_t filter;


	// check for message
	if (filter = can_com.read(&message)) {

		switch(filter) {

			case CAN_ID_DRIVE:

				// remove messy data
				if (message.data[0] != 0) {
					_switches.set(message.data[0]);
					_switches_1.set(message.data[1]);
				}

// Serial.print(filter);
// Serial.print(" ");
// Serial.println(_switches.get(), BIN);

				// =======================================
				// set values
				// set direction
				module.direction(_switches.get_flag(DIR_FLAG));


				// set switches
				status.set_flag(MAINS_FLAG, _switches.get_flag(MAINS_FLAG));
				status.set_flag(DRIVE_FLAG, _switches.get_flag(DRIVE_FLAG));


				// get driving data (10 bits)
				speed_val = char2int (message.data[2] & 0x03, message.data[3]);
				break_val = char2int (message.data[4] & 0x03, message.data[4]);
				power_val = char2int (message.data[6] & 0x03, message.data[7]);


				// get multi traktion data
				multi_uuid = (message.data[6] >> 10) | ((message.data[4] >> 10) << 6) | ((message.data[2] >> 10) << 12);


				// bit 15 = 1 => is multi traktion
				if (multi_uuid & 0x8000) {


					// remove multi flag and set multi status
					multi_uuid &= 0x7FFF;
					_motor_status.set_flag(MULTI_FLAG, true);
					_slave = true;


					// check if main loco
					if (can_com.uuid() = _multi_uuid) {
						_slave = false;
					}
				}

				else {
					_motor_status.set_flag(MULTI_FLAG, false);
				}


				// set speed values
				_set_speed(speed_val / 4);
				_set_break(break_val / 4);

				break;

			case CAN_ID_DRIVE_HEARTBEAT:
				_heartbeat_timeout.retrigger();
				break;

			// case CAN_ID_SIGNAL:
				// break;

			// case CAN_ID_LIGHT:
			// 	Serial.print("Light ");
			// 	break;

			// case CAN_ID_MOTOR_CURRENT:
			// 	Serial.print("Motor Current ");
			// 	break;

			// case CAN_ID_BATT_VOLTAGE:
			// 	Serial.print("Battery voltage ");
			// 	break;
		}
	}
}






// get motor status from can message
void MODULE::_set_status(CAN_MESSAGE) {


	// ========================================
	// vehicle is standing
	if (_standing() == false) {
		_standing = false;
		_standing_timeout.retrigger();
	}

	else {
		_standing = true;

		// standing timed out => switch off main relais
		if (_standing_timeout.check() == true) {
			_mains_relais = false;
		}
	}


	// ========================================
	// heartbeat timeout > EMERGENCY STOP
	if (_heartbeat_timeout.check()) {
		_motor_status = MOTOR_STATUS_EMERGENCY;
		_status.set_flag(MAINS_FLAG, false);
	}


	// ========================================
	// is active
	// ========================================

	// ========================================
	// setup message for me
	if (message.id == CAN_ID_LOCO_SETUP && message.uuid = can_com.uuid()) {
		_motor_status == MOTOR_STATUS_SETUP;
	}


	// normal operation
	else {


		// ========================================
		// mains on
		if (_switches.get_flag(CONTROL_MAINS_FLAG) == true) {


			// drive is on
			if (_switches.get_flag(CONTROL_DRIVE_FLAG) == true) {

				// mains relais on
				_mains_relais = true;


				// is breaking
				if (_break_val != 0) {
					_motor_status = MOTOR_STATUS_BREAKING;
				}

				else {


					// is driving
					if (_drive_val != 0) {
						_motor_status = MOTOR_STATUS_DRIVING;
					}


					// is idling
					else {
						_motor_status = MOTOR_STATUS_IDLE;
					}
				}
			}


			// drive is off
			else {
				_motor_status = MOTOR_STATUS_STANDBY;
			}
		}

		// mains off
		else {
			_motor_status = MOTOR_STATUS_OFF;
			_status.set_flag(MAINS_FLAG, false);
		}
	}
}


// update hardware
// set relais and motor pwm
void _update_motor(void) {


	// normal operation
	if (_motor_status != MOTOR_STATUS_SETUP) {


		// ========================================
		// is active => send can data
		if (_motor_status != MOTOR_STATUS_EMERGENCY && _motor_status != MOTOR_STATUS_OFF) {

			_set_ramp(MOTOR_DEFAULT_RAMP);


			// stopped
			if (_motor_status == MOTOR_STATUS_STANDBY || _motor_status == MOTOR_STATUS_READY) {
				_set_speed(0);
				_set_break(0);
			}


			// driving
			else {

				// ========================================
				// loco is in motion
				if (standing() == false) {

					switch(_motor_status) {

						case MOTOR_STATUS_IDLE:
							_set_speed(0);
							_set_break(0);
							break;

						case MOTOR_STATUS_BREAKING:
							_set_speed(0);
							_set_break(_break_val);
							break;

						case MOTOR_STATUS_DRIVING:
							_set_speed(_drive_val);
							_set_break(0);
							break;
					}
				}


				// ========================================
				// is standing => set direction
				else {

					// loco is reversed
					if (_status.get_flag(REVERSE) == true) {
						_set_dir(!_status.get_flag(DIR_FLAG));
					}

					else {
						_set_dir(_status.get_flag(DIR_FLAG));
					}
				}
			}

			_send();
		}


		// ========================================
		// emergency: no heartbeat
		else {
			_set_ramp(MOTOR_EMERGENCY_RAMP);

			_set_speed(0);
			_Set_break(MOTOR_EMERGENCY_BREAK);

			_send_vehicle();
		}
	}


	// ========================================
	// loco setup mode
	else {
		_set_speed(0);
		_set_break(0);

		_setup();
		_send_seup();
	}


	_update_pwm();
}


// ========================================
// update hardware
// prevent hardware shot circuit between drive and break
void _update_pwm(void) {


	// ========================================
	// switch mains relais
	// drive is on
	if (_status.get_flag(DRIVE_FLAG) = true) {


		// is standing
		if (_standing() == true) {
			_set_mains(false);
		}

		else {
			_set_mains(true);
		}
	}

	
	// ========================================
	// breaking value (has priority over driving if both values are != 0)
	if (_target_break != 0) {

		_pwm_drive.pwmWrite(0);

		// only break when drive stopped
		if (_pwm_drive.stopped() == true) {
			_pwm_drive.pwmWrite(_target_break);
		}
	}


	// driving driving value
	else {

		if (_target_speed != 0) {

			_pwm_break.pwmWrite(0);

			// only drive when break stopped
			if (_pwm_break.stopped() == true) {
				_pwm_drive.pwmWrite(_target_speed);
			}
		}
	}


	// update ramps
	_pwm_drive.update();
	_pwm_break.update();
}


// send active CAN data
void _send(void) {

}

// send standby vehicle CAN data
void _send_vehicle(void) {

}

// send setup CAN data
void _send_setup(void) {
	
}



// ========================================
// setup method
void _setup(void) {

}







// set acceleration
void MODULE::_set_ramp(uint16_t ramp) {
	_pwm_drive.set_ramp(ramp);
}

// set mains relais
void MODULE::_set_mains(bool stat) {
	digitalWrite(RELAIS_MAIN, stat);
}

// set direction outputs
void MODULE::_set_dir(bool dir) {
	digitalWrite(RELAIS_F, !dir);
	digitalWrite(RELAIS_R, dir);
}

// clear direction outputs
void MODULE::_clear_dir(void) {
	digitalWrite(RELAIS_F, LOW);
	digitalWrite(RELAIS_R, LOW);
}

// check if direction is set
bool MODULE::_has_direction(void) {
	return digitalRead(RELAIS_F) || digitalRead(RELAIS_R);
}


// loco is stopped ?
// pwm drive and break is null
// voltage is under limit
bool MODULE::_stopped(void) {
	return _pwm_drive.stopped() && _standing() && _pwm_break.stopped();
}


// check if motor is standing
// use timeout to delay status change
bool MODULE::_standing(void) {

	// voltage under limit and controller not driving
	if (abs(_motor_voltage()) <= VOLTAGE_ZERO && _target_speed == 0) {

		_voltage_timeout.retrigger();

		if (_standing_timeout.check()) {
			return true;
		}
	}

	else {

		if (_voltage_timeout.check()) {
			_standing_timeout.retrigger();
		}
	}

	return false;
}


int16_t MODULE::_motor_voltage(void) {
	return _voltage.get(MEASURE_VALUE_RELATIVE);
}


// send data to can
bool MODULE::send(uint8_t* data, uint8_t length, long id) {

	// send data
	can_com.send(data, length, id);
}






// 	// error if not nulled
// 	if (!_drive_nulled && status.get_flag(DRIVE_FLAG)) {
// 		status.set_flag(ERROR_FLAG, true);
// 	}


// 	// =======================================
// 	// heartbeat timeout > STOP ALL
// 	if (_heartbeat_timeout.check()) {

// 		_switches.set(0);

// 		_emergency = true;
// 		_drive_nulled = false;

// 		status.set_flag(ERROR_FLAG, true);
// 	}


// 	// heartbeat OK
// 	else {
// 		// mains on
// 		if (status.get_flag(MAINS_FLAG)) {

// 			status.set_flag(ERROR_FLAG, false);
// 			_emergency = false;


// 			// send speed
// 			data[0] = _current_speed >> 8;
// 			data[1] = _current_speed & 0xFF;
// 			send(data, 2, CAN_ID_SPEED);

// 			// send motor voltage
// 			data[0] = _motor_voltage() >> 8;
// 			data[1] = _motor_voltage() % 0xFF;
// 			send(data, 2, CAN_ID_MOTOR_VOLTAGE);

// 		}

// 		// mains off
// 		else {
// 			status.set_flag(ERROR_FLAG, true);
// 			_emergency = true;
// 		}
// 	}



// // Serial.print("switch: ");
// // Serial.print(_switches.get_flag(MAINS_FLAG), BIN);
// // Serial.print(" status: ");
// // Serial.print(status.get(), BIN);
// // Serial.print(" emerg: ");
// // Serial.print(_emergency);
// // Serial.print(" V: ");
// // Serial.println(_motor_voltage());


// 	// =======================================
// 	// =======================================
// 	// mains on if stopped and mains switch on
// 	#ifdef RELAIS_MAIN

// 		if (stopped() || _emergency) {

// 			// switch on main relais
// 			if (_switches.get_flag(MAINS_FLAG) != 0) {
// 				_set_mains(true);
// 			}

// 			// mains relais OFF
// 			else {
// 				_set_mains(false);
// 				_clear_dir();
// 			}
// 		}


// 	#endif

// 	// =======================================
// 	// =======================================
// 	// status is ready to drive
// 	if (ready() && !_emergency) {

// 		// =======================================
// 		// update direction when drive is stopped
// 		if (stopped()) {

// 			_current_dir = _target_dir;

// 			// set direction
// 			_set_dir(_current_dir);
// 		}

// 		// set drive if break is stopped and nulled and direction is valid
// 		if (_pwm_break.stopped() && _drive_nulled && _current_dir == _target_dir && _has_direction()) {
// 			_pwm_drive.pwmWrite(_target_speed);
// 		}

// 		// set break if drive is stopped
// 		if (_pwm_drive.stopped()) {
// 			_pwm_break.pwmWrite(_target_break);
// 		}
// 	}


// 	// all OFF > drive and break OFF + mains relais OFF
// 	else {

// 		_pwm_drive.pwmWrite(0);
// 		_pwm_break.pwmWrite(0);

// 		status.set_flag(DRIVE_FLAG, false);

// 		// _emergency = true;
// 		_drive_nulled = false;

// 		// clear directions when stopped and mains off
// 		if (stopped() && !_switches.get_flag(MAINS_FLAG)) {
// 			_clear_dir();
// 		}
// 	}


// 	// set stop flag
// 	if (standing()) {
// 		status.set_flag(STOP_FLAG, true);
// 	}

// 	else {
// 		status.set_flag(STOP_FLAG, false);
// 	}


// 	// emergency break
// 	// if (_emergency) {

// 	// 	if (get_break() > 250) {
// 	// 		_emergency = false;
// 	// 	}

// 	// 	_pwm_drive.pwmWrite(0);

// 	// 	if (_pwm_drive.stopped()) {

// 	// 		// emergency break
// 	// 		_pwm_break.set_ramp(RAMP_EMERGENCY);
// 	// 		set_break(255);
// 	// 	}
// 	// }

// 	// else {
// 	// 	_pwm_break.set_ramp(RAMP_DEFAULT);
// 	// }



// 	// update speed/break and get current values
// 	_current_speed = _pwm_drive.update();
// 	_current_break = _pwm_break.update();


// 	// =======================================
// 	// send status
// 	data[0] = status.get();
// 	send(data, 1, CAN_ID_DRIVE);

// }