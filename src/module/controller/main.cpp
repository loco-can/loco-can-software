/*
 * Loco-CAN controller function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../config.h"
#include "main.h"


extern CAN_COM can;


#ifdef MODULE_CONTROLLER_CONFIG_H

void MODULE_CONTROLLER::begin(void) {

	// start drive package timeout
	_drive_time.begin(CAN_ID_DRIVE_TIME);
	_heartbeat_time.begin(CAN_ID_DRIVE_HEARTBEAT_TIME);

	// set initial status
	_status = CONTROLLER_STATUS_OFF;


	#ifdef DEBUG
		Serial.println("*************************");
		Serial.println("start function/controller");
	#endif


	// register can filters
	can.register_filter(CAN_ID_MASK, CAN_ID_LIGHT_CURRENT);	// light current
	can.register_filter(CAN_ID_MASK, CAN_ID_DRIVE);			// drive commands from other controllers

	/*
	 * set all ports
	 */

	// **********************************
	// SWITCHES
	_mains_switch.begin(CONTROLLER_MAINS_PORT);
	_dir_switch.begin(CONTROLLER_DIR_PORT);

	// HORN SWITCH
	#ifdef CONTROLLER_HORN_PORT
		#ifdef DEBUG
			Serial.print("> init horn button on port ");
			Serial.println(CONTROLLER_HORN_PORT);
		#endif
		_horn_switch.begin(CONTROLLER_HORN_PORT, BUTTON_LONG_PUSH_TIME);
	#endif

	// SECOND HORN SWITCH
	#ifdef CONTROLLER_HORN2_PORT
		#ifdef DEBUG
			Serial.print("> init second horn button on port ");
			Serial.println(CONTROLLER_HORN2_PORT);
		#endif
		_horn2_switch.begin(CONTROLLER_HORN2_PORT, BUTTON_LONG_PUSH_TIME);
	#endif

	// LIGHT SWITCH
	#ifdef CONTROLLER_LIGHT_PORT
		#ifdef DEBUG
			Serial.print("> init analog light switch on port ");
			Serial.println(CONTROLLER_LIGHT_PORT);
		#endif
		_light_switch.begin(CONTROLLER_LIGHT_PORT);
	#endif

	// SECOND LIGHT SWITCH
	#ifdef CONTROLLER_LIGHT2_PORT
		#ifdef DEBUG
			Serial.print("> init second analog light switch on port ");
			Serial.println(CONTROLLER_LIGHT2_PORT);
		#endif
		_light2_switch.begin(CONTROLLER_LIGHT2_PORT);
	#endif

	// INSTRUMENT LIGHT SWITCH
	#ifdef CONTROLLER_INSTRUMENT_LIGHT_PORT
		#ifdef DEBUG
			Serial.print("> init instrument light switch on port ");
			Serial.println(CONTROLLER_INSTRUMENT_LIGHT_PORT);
		#endif
		_instrument_switch.begin(CONTROLLER_INSTRUMENT_LIGHT_PORT);

		#ifdef DEBUG
			Serial.println("> register CAN_ID_LIGHT");
		#endif

		can.register_filter(CAN_ID_MASK, CAN_ID_LIGHT); // light for instruments
	#endif
	// **********************************

	// init stati
}


void MODULE_CONTROLLER::update(CAN_MESSAGE message) {


	// ==================
	// check for messages
	if (message.uuid != 0) {

		#ifdef DEVEL
			Serial.print("got message ");
			can.print_message(message);
		#endif

		// ===========================
		// select action by message id
		switch(message.id) {

			// drive message from other controllers
			// set controller to blocked
			case CAN_ID_DRIVE:
				break;

			// vehicle status
			// handshake if controller is active
			case CAN_ID_VEHICLE_STATUS:

				// set current vehicle status
				break;

			// light message to switch instrument light
			case CAN_ID_LIGHT:
				break;

			default:
				break;
		}
	}


	// ===========================
	// get switches


	// ===========================
	// get analog values
	uint16_t analog_value = analogRead(CONTROLLER_DRIVE_PORT);



	// ===========================
	// update status


	// ==================
	// check state

	
	// ==================
	// send drive package not off or locked
	if (_heartbeat_time.check()) {

		_heartbeat_time.retrigger();

		_message.id = CAN_ID_DRIVE_HEARTBEAT;
		_message.size = 0;

		can.send(_message);
	}

	if (_drive_time.check() && _status >= CONTROLLER_STATUS_STANDBY) {

		_drive_time.retrigger();

		_message.id = CAN_ID_DRIVE;
		_message.size = 2;
		_message.data[0] = 0b10010010;
		_message.data[1] = 0x23;

		can.send(_message);
	}
}

#endif