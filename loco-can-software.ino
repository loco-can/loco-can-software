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
 * config.h: module selection and parameter settings
 * can_com.h: CAN communication class
 * can_com.h: CAN communication class
 * LocoCANcore.h: The main class
 */
#include "config.h"
#include "src/LocoCANcore.h"

/*
 * create core
 */
LocoCANcore core;


// STARTUP
void setup() {

	#ifdef DEBUG
		Serial.begin(115200);

		Serial.println("***********************");
		Serial.println("* Welcome to Loco-Can *");
		Serial.println("* The Bus  for Trains *");
		Serial.println("***********************");
		Serial.println();
		Serial.print("MODULE: ");
		Serial.print(MODULE);
		Serial.print(", VERSION: ");
		Serial.println(MODULE_VERSION);
		Serial.println();
	#endif

	core.begin();
}                   


// MAIN LOOP
void loop() {
	core.update();
}