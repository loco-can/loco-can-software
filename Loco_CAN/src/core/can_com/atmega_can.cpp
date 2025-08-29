/*
 * Loco-CAN can class for ATMEGA
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */

// use can.h if is ATMEGA or ESP32 
#if BOARD_PLATFORM != ESP32S3

#include "atmega_can.h"
#include "can_com.h"
#include <CAN.h>


void CAN_HANDLER::begin(long speed, uint16_t can_cs, uint16_t can_int) {
	CAN.begin(speed);
}


bool CAN_HANDLER::available(void) {
	return CAN.available();
}


uint16_t CAN_HANDLER::parsePacket(void) {
	CAN.parsePacket();
}


bool CAN_HANDLER::packetExtended(void) {
	return CAN.packetExtended();
}


uint32_t CAN_HANDLER::packetId(void) {
	return CAN.packetId;
}


bool CAN_HANDLER::send(CAN_MESSAGE message) {

	// ATMEGA SEND
	CAN.beginExtendedPacket((id << 18) | _uuid);

	// send data with length
	// restrict to 8 uint8_ts
	while (i < length && i < 8) {
		CAN.write(data[i++]);  
	}

	CAN.endPacket();
}



uint8_t CAN_HANDLER::read(void) {
	return CAN.read();
}

#endif