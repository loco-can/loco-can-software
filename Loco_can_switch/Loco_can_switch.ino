/*
 * LOCO-CAN Switch Module
 * (former Light Module)
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020-20232
 * @lizence: GG0
 */

/* The switch module is designed to switch up to
 * six switch outputs according to different modes.
 * The standard setting is for switching lights according
 * to the light modes.
 * The light modes are declared in the loco_config.h
 * file.
 *
 * Each outputs can be assigned to the seven light modes.
 *
 * The current of all outputs is detected and used for
 * short circuit protection.
 */


// =====================================
//
// DATA INDEX MAPPING
//
// =====================================

// byte			content
// --------------------------
// dir = 0
//  0		forward_light 1
//  1		forward_light 2
//  2		forward_light 3
//  3		forward_light 4
//  4		forward_light 5
//  5		forward_light 6
//  6		forward_light 7
//
// dir = 1
//  7		backward_light 1
//  8		backward_light 2
//  9		backward_light 3
//  10		backward_light 4
//  11		backward_light 5
//  12		backward_light 6
//  13		backward_light 7


#include "config.h"
#include "module.h"


MODULE module;


CAN_COM can_com;


void setup() {

	Serial.begin(115200);
	Serial.println("Start Loco-Light");

	can_com.begin(500E3, CAN_STATUS_LED);

	can_com.register_filter(CAN_ID_MASK, CAN_ID_LIGHT);
	can_com.register_filter(CAN_ID_MASK, CAN_ID_DRIVE);
	
	can_com.register_filter(CAN_REQUEST_MASK, CAN_ID_REQUEST);
	can_com.register_filter(CAN_SETUP_MASK, CAN_ID_SETUP);

	Serial.println();
	Serial.print("Device UUID: ");
	Serial.println(can_com.uuid(), HEX);

	module.begin();
}


void loop() {

	// update module
	module.update();
}
