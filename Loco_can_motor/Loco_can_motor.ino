/*
 * LOCO-CAN LK-1 motor driver
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */

/*
 * The module is designed for the MOSFET motor power unit
 * LK-1 of Mr Habersetzer, that have no reverse protection.
 *
 * Both the motor current and the motor voltage is detected
 * for correct operation. An additional power relay can cut
 * the connection to the motor when in standby.
 *
 */


/*
 * SEND:
 * 
 * STATUS
 * byte 0:   7      6      5      4      3      2      1      0
 *         error  ready   stop                 dir   drive  mains
 */

/*
 * RECEIVE:
 * 7 byte package
 *
 * STATUS
 * byte 0:   7      6      5      4      3      2      1      0
 *         error  ready   stop                 dir   drive  mains
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

#include "config.h"
#include "module.h"

#include "intelliled.h"
#include "flags.h"


MODULE module;

CAN_COM can_com;

FLAGS switches;
FLAGS status;


// temp analog input
#define POT A0


void setup() {

	#ifdef DEBUG
		Serial.begin(115200);
		Serial.println("Start LOCO CAN");
	#endif

	// begin CAN communication
	can_com.begin(CAN_BUS_SPEED, CAN_STATUS_LED);
	// can_com.setHeartbeat(HEARTBEAT, CAN_ID_DRIVE_HEARTBEAT);

	#ifdef DEBUG
		Serial.println();
		Serial.print("Device UUID: ");
		Serial.println(can_com.uuid(), HEX);
	#endif


	// register commands to read
	can_com.register_filter(CAN_ID_MASK, CAN_ID_DRIVE);
	can_com.register_filter(CAN_ID_MASK, CAN_ID_LIGHT);
	can_com.register_filter(CAN_ID_MASK, CAN_ID_DRIVE_HEARTBEAT);
	can_com.register_filter(CAN_LOCO_SETUP_MASK, CAN_ID_LOCO_SETUP);


	// init module
	module.begin(RAMP_DEFAULT);

	module.set_ramp(1000);
	module.set_speed(127);
	module.set_break(0);

	module.direction(1);


	// start status
	status.begin();
	switches.begin();

}


void loop() {

	// update module
	module.update();
}
