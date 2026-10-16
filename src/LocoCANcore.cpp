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
		#if defined(CONFIG_IDF_TARGET_ESP32)
			#if (CAN_RX >= 6 && CAN_RX <= 11) || (CAN_TX >= 6 && CAN_TX <= 11)
				#error "ESP32-WROOM GPIO 6-11 are wired to flash; TWAI on those pins resets the chip"
			#endif
			#if CAN_TX >= 34 && CAN_TX <= 39
				#error "ESP32 GPIO 34-39 are input-only; CAN_TX needs an output pin (RX on 34-39 is fine)"
			#endif
		#endif
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
	#ifndef PROGRAMMER_MODULE_VERSION
		_send_module_heartbeat();
	#endif

	#ifdef DEBUG
		Serial.println();
		Serial.println("*****************************");
		Serial.println("Loco-CAN started successfully");
		Serial.println("*****************************");
	#endif
	/* ***************************************************** */
}


void LocoCANcore::update(void) {

	#ifndef PROGRAMMER_MODULE_VERSION
		if (_module_heartbeat.update()) {
			_send_module_heartbeat();
		}
	#endif

	uint8_t n = 0;

	/*
	 * Drain a bounded number of frames so a busy bus cannot starve the
	 * module heartbeat. fetch() keeps DLC-0 heartbeats, which read()
	 * would filter. With a quiet bus the module still runs once.
	 */
	while (n < 8 && can.fetch(can_message)) {
		n++;
		_module.update(can_message);
	}

	/*
	 * Always tick once without a frame so HTTP/OLED keep running
	 * even when the CAN mailbox stayed busy.
	 */
	can_message.id = 0;
	can_message.uuid = 0;
	can_message.size = 0;
	_module.update(can_message);

}


void LocoCANcore::_send_module_heartbeat(void) {

	#ifndef LOCO_MODULE_TYPE
		#error "LOCO_MODULE_TYPE must be defined by the selected module"
	#endif

	CAN_MESSAGE heartbeat;
	heartbeat.id = CAN_ID_MODULE_HEARTBEAT;
	heartbeat.uuid = 0;
	heartbeat.size = 4;
	heartbeat.data[0] = (uint8_t)LOCO_MODULE_TYPE;
	heartbeat.data[1] = (uint8_t)LOCO_VERSION_MAJOR(LOCO_MODULE_VERSION);
	heartbeat.data[2] = (uint8_t)LOCO_VERSION_MINOR(LOCO_MODULE_VERSION);
	heartbeat.data[3] = (uint8_t)LOCO_VERSION_PATCH(LOCO_MODULE_VERSION);

	/* #ifdef DEBUG
		Serial.print("> module heartbeat type 0x");
		Serial.print((uint8_t)LOCO_MODULE_TYPE, HEX);
		Serial.print(" ver ");
		Serial.print((uint8_t)LOCO_VERSION_MAJOR(LOCO_MODULE_VERSION));
		Serial.print(".");
		Serial.print((uint8_t)LOCO_VERSION_MINOR(LOCO_MODULE_VERSION));
		Serial.print(".");
		Serial.print((uint8_t)LOCO_VERSION_PATCH(LOCO_MODULE_VERSION));
		Serial.println(can.send(heartbeat) ? " ok" : " fail");
	#else */
		can.send(heartbeat);
	/* #endif */
}