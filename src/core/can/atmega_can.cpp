/*
 * Loco-CAN can class for ATMEGA
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */

#include "../../../config.h"

// use can.h if is ATMEGA or ESP32 
#ifdef MODULE_PLATFORM_ATMEGA

	#include "atmega_can.h"
	#include "arduino_can/can.h"


	bool CAN_HANDLER::begin(long speed, uint16_t can_cs, uint16_t can_int) {

		CAN.setPins(can_cs, can_int);
		CAN.begin(speed);

		return true;
	}


	bool CAN_HANDLER::available(void) {
		return CAN.available();
	}


	uint16_t CAN_HANDLER::parsePacket(void) {
		CAN.parsePacket();

		return 0;
	}


	bool CAN_HANDLER::packetExtended(void) {
		return CAN.packetExtended();
	}


	long CAN_HANDLER::packetId(void) {
		return CAN.packetId();
	}


	bool CAN_HANDLER::send(CAN_MESSAGE message) {

		uint8_t i = 0;

		// ATMEGA SEND
		CAN.beginExtendedPacket((message.id << 18) | message.uuid);

		// send data with length
		// restrict to 8 uint8_ts
		while (i < message.size && i < 8) {
			CAN.write(message.data[i++]);  
		}

		CAN.endPacket();

		return true;
	}



	uint8_t CAN_HANDLER::read(void) {
		return CAN.read();
	}

#endif