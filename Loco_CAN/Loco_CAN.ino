/*
 * Loco-CAN
 * 
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 *
 * Version 3.x
 *
 * The new Loco-CAN software consists of only one package, that can be
 * configured in the config.h file depending on the used hardware. All
 * available functions are organized as libraries and are included when
 * needed.  
 */

/*
 * The can protocol is defined in can_protocol.h
 *
 * All functions used in the module are included
 */


/*
 * basic includes
 * config.h: all module settings
 * board.h: hardware settings depending on hw version
 * can_com.h: CAN communication class
 * settings.h: configure parameters via CAN
 */
#include "config.h"
#include "boards.h"
#include "can_com.h"
// #include "settings.h"


/*
 * create CAN communication
 */
CAN_COM can_com(CAN_RX, CAN_TX);


/*
 * The main routines of the module are included by selecting
 * the module in config.h
 */
MODULE module;



void setup() {

	Serial.begin(115200);
	Serial.println("startup");

	// start module
	module.begin();
}                   


void loop() {
	module.update();
}