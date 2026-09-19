/*
 * Loco-CAN can class for ESP32
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 */
#pragma once

#ifndef ESP32_CAN_H
#define ESP32_CAN_H


#include "can_message.h"
#include "driver/twai.h"


class CAN_HANDLER {

	public:
		CAN_HANDLER();

		// start can communication with rx, tx ports
		bool begin(long speed, uint16_t rx, uint16_t tx);
		bool available(void);
		uint16_t parsePacket(void);
		bool packetExtended(void);
		long packetId(void);
		bool send(CAN_MESSAGE message);
		uint8_t read(void);

	private:
		long _rxId;
		bool _rxExtended;
		uint8_t _rxLength;
		uint8_t _rxIndex;
		uint8_t _rxData[8];
};


#endif
