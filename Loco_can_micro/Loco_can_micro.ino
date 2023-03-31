/*
 * LOCO-CAN Micro Module
 * Based on ATtiny85 MCU
 * 
 * @author: Thomas H Winkler
 * @copyright: 2023
 * @lizence: GG0
 */

/* The micro module offers two I/O pins for different applications.
 * The ports can be used as analog output, servo motor control
 * or digital or analog inputs.
 * It has a standard can port and is fully compatible with the system.
 */


// =====================================
//
// DATA INDEX MAPPING
//
// =====================================


#include "config.h"
#include "module.h"


MODULE module;


CAN_COM can_com;


void setup() {

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
