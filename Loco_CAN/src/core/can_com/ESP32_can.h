/*
 * Loco-CAN can class for ESP32
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 */

#ifndef ESP32_CAN_H
#define ESP32_CAN_H

// use twai if is ESP32-S3 
#if BOARD_PLATFORM == ESP32S3


#include "can_message.h"
#include <ESP32-TWAI-CAN.hpp>


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