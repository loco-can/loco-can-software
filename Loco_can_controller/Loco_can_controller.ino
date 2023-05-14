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
 * SEND:
 * 7 byte package
 *
 * STATUS
 * byte 0:   7      6      5      4      3      2      1      0
 *         error  ready   stop   horn   aux    dir   drive  mains
 *
 * DRIVE VALUE: 10-bit value of drive voltage
 * byte 1: drive bit 8-9
 * byte 2: drive bit 0-7
 *
 * POWER VALUE: 10-bit value of drive max power
 * byte 3: power bit 8-9
 * byte 4: power bit 0-7
 *
 * BREAK VALUE: 10-bit value of break intensity
 * byte 5: break bit 8-9
 * byte 6: break bit 0-7
 */

/*
 * RECEIVE:
 * 
 * STATUS
 * byte 0:   7      6      5      4      3      2      1      0
 *         error  ready   stop          aux    dir   drive  mains
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
