/*
 * Loco-CAN can class for ESP32
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 */

#pragma once


#include "can_message.h"
// #include <ESP32-TWAI-CAN.hpp>
#include "driver/twai.h"


class CAN_HANDLER {

	public:
		// start can communication with tx, rx ports
		bool begin(long speed, uint16_t rx, uint16_t tx);
		bool available(void);
		uint16_t parsePacket(void);
		bool packetExtended(void);
		long packetId(void);
		bool send(CAN_MESSAGE message);
		uint8_t read(void);
};
