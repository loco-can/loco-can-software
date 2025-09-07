/*
 * Loco-CAN controller function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../../config.h"
#include "main.h"


void FUNCTION_CONTROLLER::begin(CAN_COM can) {

	#ifdef DEBUG
		Serial.println("*************************");
		Serial.println("start function/controller");
	#endif

	_can = can;

	// register can filters
	_can.register_filter(CAN_ID_MASK, CAN_ID_LIGHT_CURRENT);	// light current
	_can.register_filter(CAN_ID_MASK, CAN_ID_DRIVE);			// drive commands from other controllers

	/*
	 * set all ports
	 */

    // **********************************
	// SWITCHES
	_mains_switch.begin(CONTROLLER_MAINS_PORT, CONTROLLER_ANALOG_RESOLUTION);
	_dir_switch.begin(CONTROLLER_DIR_PORT, CONTROLLER_ANALOG_RESOLUTION);

	// HORN SWITCH
	#ifdef DEBUG
		Serial.print("> init horn button on port ");
		Serial.println(CONTROLLER_HORN_PORT);
	#endif
	_horn_switch.begin(CONTROLLER_HORN_PORT, BUTTON_LONG_PUSH_TIME);

	// SECOND HORN SWITCH
	#ifdef CONTROLLER_HORN2_PORTl
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
		_light_switch.begin(CONTROLLER_LIGHT_PORT, CONTROLLER_ANALOG_RESOLUTION);
	#endif

	// SECOND LIGHT SWITCH
	#ifdef CONTROLLER_LIGHT2_PORT
		#ifdef DEBUG
			Serial.print("> init second analog light switch on port ");
			Serial.println(CONTROLLER_LIGHT2_PORT);
		#endif
		_light2_switch.begin(CONTROLLER_LIGHT2_PORT, CONTROLLER_ANALOG_RESOLUTION);
	#endif

	// INSTRUMENT LIGHT SWITCH
    #ifdef CONTROLLER_INSTRUMENT_LIGHT_PORT
		#ifdef DEBUG
			Serial.print("> init instrument light switch on port ");
			Serial.println(CONTROLLER_LIGHT2_PORT);
		#endif
		_instrument_switch.begin(CONTROLLER_INSTRUMENT_LIGHT_PORT);

		#ifdef DEBUG
			Serial.println("> register CAN_ID_LIGHT");
		#endif

        _can.register_filter(CAN_ID_MASK, CAN_ID_LIGHT); // light for instruments
    #endif
    // **********************************


    // **********************************
    // GAUGES

	// BATTERY VOLTAGE GAUGE
    #ifdef CONTROLLER_BATTERY_VOLTAGE_PORT
		#ifdef DEBUG
			Serial.print("> init battery voltage gauge on port ");
			Serial.println(CONTROLLER_BATTERY_VOLTAGE_PORT);
		#endif
		_battery_voltage_gauge.begin(CONTROLLER_BATTERY_VOLTAGE_PORT);
	#endif

	// MOTOR VOLTAGE GAUGE
    #ifdef CONTROLLER_MOTOR_VOLTAGE_PORT
		#ifdef DEBUG
			Serial.print("> init motor voltage gauge on port ");
			Serial.println(CONTROLLER_MOTOR_VOLTAGE_PORT);
		#endif
		_motor_voltage_gauge.begin(CONTROLLER_MOTOR_VOLTAGE_PORT);
	#endif

	// BATTERY CURRENT GAUGE
    #ifdef CONTROLLER_BATTERY_CURRENT_PORT
		#ifdef DEBUG
			Serial.print("> init battery current gauge on port ");
			Serial.println(CONTROLLER_BATTERY_CURRENT_PORT);
		#endif
		_battery_current_gauge.begin(CONTROLLER_BATTERY_CURRENT_PORT);
	#endif

	// MOTOR CURRENT GAUGE
    #ifdef CONTROLLER_MOTOR_CURRENT_PORT
		#ifdef DEBUG
			Serial.print("> init motor current gauge on port ");
			Serial.println(CONTROLLER_MOTOR_CURRENT_PORT);
		#endif
		_motor_current_gauge.begin(CONTROLLER_MOTOR_CURRENT_PORT);
	#endif

}


void FUNCTION_CONTROLLER::update(void) {

}