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
 */
#include "config.h"
// #include "src/core/can/can_com.h"
// #include "src/LocoCANcore.h"


/*
 * create CAN communication
 */
// CAN_COM can_com(CAN_RX, CAN_TX);


/*
 * create core
 */
// LocoCANcore core



void setup() {

	Serial.begin(115200);
	Serial.println("startup");

	// start module
	Serial.println("start core");
	// core.begin();
}                   


void loop() {
	// core.update();
}