/*
 * Loco-CAN controller function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

#include "../../../config.h"
#include "main.h"


extern CAN_COM can;


void FUNCTION_CONTROLLER::begin(void) {

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
			Serial.println(CONTROLLER_INSTRUMENT_LIGHT_PORT);
		#endif
		_instrument_switch.begin(CONTROLLER_INSTRUMENT_LIGHT_PORT);

		#ifdef DEBUG
			Serial.println("> register CAN_ID_LIGHT");
		#endif

        can.register_filter(CAN_ID_MASK, CAN_ID_LIGHT); // light for instruments
    #endif
    // **********************************

}


void FUNCTION_CONTROLLER::update(void) {

}