/*
 * Loco-CAN can class for ATMEGA
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 */

#ifndef ATMEGA_CAN_H
#define ATMEGA_CAN_H


// use can.h if is ATMEGA or ESP32 
#if BOARD_PLATFORM != ESP32S3


#include "can_message.h"
#include <CAN.h>

class CAN_HANDLER {

	public:
		// start can communication with tx, rx ports
		bool begin(long speed, uint16_t rx, uint16_t tx);
		bool available(void);
		uint16_t parsePacket(void);
		bool packetExtended(void);
		uint32_t packetId(void);
		bool send(CAN_MESSAGE message);
		uint8_t read(void);
};


#endif

#endif