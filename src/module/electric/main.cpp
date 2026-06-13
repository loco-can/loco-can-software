/*
 * Loco-CAN electric loco function
 *
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 */

// #include "../../config.h"
#include "main.h"

#ifdef MODULE_ELECTRIC_CONFIG_H


void MODULE_ELECTRIC::begin(void) {

	#ifdef DEBUG
		Serial.println("********************");
		Serial.println("start function/motor");
	#endif

}

void MODULE_ELECTRIC::update(CAN_MESSAGE message) {

	can_message = message;
}

#endif