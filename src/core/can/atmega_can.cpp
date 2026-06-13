/*
 * Loco-CAN can class for ATMEGA
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */

#include "../../config.h"

// use can.h if is ATMEGA or ESP32 
#ifdef MODULE_ARCH_AVR

	#include "atmega_can.h"
	#include "arduino_can/CAN.h"


	bool CAN_HANDLER::begin(long speed, uint16_t can_cs, uint16_t can_int) {

		CAN.setPins(can_cs, can_int);
		CAN.begin(speed);

		return true;
	}


	bool CAN_HANDLER::available(void) {
		return CAN.available();
	}


	uint16_t CAN_HANDLER::parsePacket(void) {
		return CAN.parsePacket();
	}


	bool CAN_HANDLER::packetExtended(void) {
		return CAN.packetExtended();
	}


	long CAN_HANDLER::packetId(void) {
		return CAN.packetId();
	}


	bool CAN_HANDLER::send(CAN_MESSAGE message) {

		if (message.size > 8) {
			return false;
		}

		const long can_id = ((long)message.id << 18) | (message.uuid & 0x3FFFF);

		if (!CAN.beginExtendedPacket(can_id, message.size)) {
			return false;
		}

		for (uint8_t i = 0; i < message.size; i++) {
			if (CAN.write(message.data[i]) != 1) {
				return false;
			}
		}

		return CAN.endPacket() == 1;
	}



	uint8_t CAN_HANDLER::read(void) {
		return CAN.read();
	}

#endif