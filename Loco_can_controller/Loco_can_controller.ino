/*
 * LOCO-CAN Controller Module
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */

/*
 * The controller module is for simple units
 * that use a CAN cable connection for controlling
 * the train.
 *
 * For the connection see the module_board.h file.
 */

/*
 * The communication protocol is defined in the LocoCAN library.
 */


#include "config.h"
#include "module.h"


CAN_COM can_com;

MODULE module;


void setup() {

	// ===================================================================
	// start basic functions
	Serial.begin(115200);


	// ===================================================================
	// INIT CAN
    #ifdef DEBUG
    	Serial.println("Start CAN Controler");
    #endif

	can_com.begin(CAN_BUS_SPEED, CAN_STATUS_LED); // set CAN speed
	can_com.set_alive(500); // set live time for CAN communication timeout


	// register can filters
	can_com.register_filter(CAN_ID_MASK, CAN_ID_LIGHT_CURRENT); // light current
	can_com.register_filter(CAN_ID_MASK, CAN_ID_VEHICLE_STATUS); // status of vehicles
	can_com.register_filter(CAN_ID_MASK, CAN_ID_DRIVE);	// drive commands from other controllers


    // *****************
    // OPTIONAL - depends on the board version
    // init instrument light if output is present
    #ifdef INSTRUMENT_LIGHT
        can_com.register_filter(CAN_ID_MASK, CAN_ID_LIGHT); // light for instruments

        pinMode(INSTRUMENT_LIGHT, OUTPUT);
        digitalWrite(INSTRUMENT_LIGHT, LOW);
    #endif
    // *****************

	can_com.register_filter(CAN_SETUP_MASK, CAN_ID_REPLY);
	can_com.register_filter(CAN_REPLY_MASK, CAN_ID_REPLY);


	// start module
	module.begin();
}


void loop() {

	// update module
	module.update();

}
