/*
 * Loco-CAN
 * 
 * @author: Thomas H Winkler
 * @copyright: 2018-2025
 * @lizence: GG0
 *
 * Version 3.x
 *
 */


// #include "../config.h"
#include "can_protocol.h"
#include "LocoCANcore.h"


CAN_COM can;
CAN_MESSAGE can_message;


void LocoCANcore::begin(void) {

	#ifdef DEBUG
		Serial.println("*******************");
		Serial.println("* starting core");
	#endif

	/*
	 * create CAN class
	 * CAN_TX, CAN_TX and CAN_STATUS_LED are defined in the module settings h file
	 * CAN_BUS_SPEED is set in the can_protocol.h
	 */
	can.setPorts(CAN_RX, CAN_TX);
	can.set_alive(CAN_ALIVE_TIMEOUT);
	can.begin(CAN_BUS_SPEED, CAN_STATUS_LED); // start with one CAN LED


	/* ********************************************************
	 * start module
	 ******************************************************* */
	#ifdef DEBUG
		Serial.println("*******************");
		Serial.println("starting functions");
	#endif

	// =========================
	// start module
	_module.begin();

	#ifdef DEBUG
		Serial.println();
		Serial.println("*****************************");
		Serial.println("Loco-CAN started successfully");
		Serial.println("*****************************");
	#endif
	/* ***************************************************** */
}


void LocoCANcore::update(void) {

	can_message.uuid = 0;

	can.read(can_message);

	/*
	 * update registered functions
	 */

	// ==========================
	// update module
	#ifdef MODULE_CONTROLLER_H
		_module.update(can_message);
	#endif

}