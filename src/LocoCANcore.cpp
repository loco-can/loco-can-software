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


#include "config.h"

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
	 * AVR: CAN_SS (MCP2515 chip select) and optional CAN_INT.
	 * ESP32: CAN_RX / CAN_TX are the TWAI data pins.
	 * CAN_BUS_SPEED is set in can_protocol.h.
	 */
	#ifdef MODULE_ARCH_ESP32
		can.setPorts(CAN_RX, CAN_TX);
	#else
		#ifndef CAN_SS
			#error "CAN_SS must be defined in the module config (MCP2515 chip select)"
		#endif
		#ifdef CAN_INT
			can.setPorts(CAN_SS, CAN_INT);
		#else
			can.setPorts(CAN_SS);
		#endif
	#endif
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

	_module_heartbeat.begin(MODULE_HEARTBEAT_TIMEOUT);
	_send_module_heartbeat();

	#ifdef DEBUG
		Serial.println();
		Serial.println("*****************************");
		Serial.println("Loco-CAN started successfully");
		Serial.println("*****************************");
	#endif
	/* ***************************************************** */
}


void LocoCANcore::update(void) {

	if (_module_heartbeat.update()) {
		_send_module_heartbeat();
	}

	bool got = false;
	uint8_t n = 0;

	/*
	 * Drain a bounded number of frames so a busy bus cannot starve the
	 * module heartbeat. fetch() keeps DLC-0 heartbeats, which read()
	 * would filter. With a quiet bus the module still runs once.
	 */
	while (n < 8 && can.fetch(can_message)) {
		got = true;
		n++;
		_module.update(can_message);
	}

	if (!got) {
		can_message.id = 0;
		can_message.uuid = 0;
		can_message.size = 0;
		_module.update(can_message);
	}

}


void LocoCANcore::_send_module_heartbeat(void) {

	#ifndef LOCO_MODULE_TYPE
		#error "LOCO_MODULE_TYPE must be defined by the selected module"
	#endif

	CAN_MESSAGE heartbeat;
	heartbeat.id = CAN_ID_MODULE_HEARTBEAT;
	heartbeat.uuid = 0;
	heartbeat.size = 2;
	heartbeat.data[0] = (uint8_t)LOCO_MODULE_TYPE;
	heartbeat.data[1] = (uint8_t)LOCO_MODULE_VERSION;

	/* #ifdef DEBUG
		Serial.print("> module heartbeat type 0x");
		Serial.print((uint8_t)LOCO_MODULE_TYPE, HEX);
		Serial.print(" ver ");
		Serial.print((uint8_t)LOCO_MODULE_VERSION);
		Serial.println(can.send(heartbeat) ? " ok" : " fail");
	#else */
		can.send(heartbeat);
	/* #endif */
}