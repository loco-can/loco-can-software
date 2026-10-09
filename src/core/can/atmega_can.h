/*
 * Loco-CAN can class for ATMEGA
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 */
#pragma once

#ifndef ATMEGA_CAN_H
#define ATMEGA_CAN_H


// #include "../../config.h"

#include "can_message.h"
#include "arduino_can/CAN.h"


class CAN_HANDLER {

	public:
		// start MCP2515; irq < 0 means INT is not connected
		bool begin(long speed, uint16_t ss, int irq);
		bool available(void);
		uint16_t parsePacket(void);
		bool packetExtended(void);
		long packetId(void);
		bool send(CAN_MESSAGE message);
		uint8_t read(void);
};


// ATMEGA_CAN_HANDLER CAN_HANDLER

#endif