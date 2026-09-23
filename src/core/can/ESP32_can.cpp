/*
 * Loco-CAN can class for ESP32
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 */

#include "../../config.h"

// use twai if is ESP32
#ifdef MODULE_ARCH_ESP32

	#include "ESP32_can.h"
	#include "ESP32-TWAI-CAN.hpp"


	CAN_HANDLER::CAN_HANDLER() :
		_rxId(-1),
		_rxExtended(false),
		_rxLength(0),
		_rxIndex(0)
	{
		memset(_rxData, 0, sizeof(_rxData));
	}


	bool CAN_HANDLER::begin(long speed, uint16_t can_rx, uint16_t can_tx) {

		// TwaiCAN::setPins(tx, rx)
		ESP32Can.setPins(can_tx, can_rx);

	    // .setSpeed() and .begin() functions require to use TwaiSpeed enum,
	    // but you can easily convert it from numerical value using .convertSpeed()
	    ESP32Can.setSpeed(ESP32Can.convertSpeed(speed));


	    // start CAN bus
		if(ESP32Can.begin()) {
			Serial.println("CAN bus started!");
			return true;

		}

		else {
			Serial.println("CAN bus failed!");
			return false;
		}
	}


	bool CAN_HANDLER::available(void) {
		return _rxIndex < _rxLength;
	}


	uint16_t CAN_HANDLER::parsePacket(void) {

		CanFrame frame = { 0 };

		if (!ESP32Can.readFrame(frame, 0)) {
			_rxId = -1;
			_rxExtended = false;
			_rxLength = 0;
			_rxIndex = 0;
			return 0;
		}

		_rxId = frame.identifier;
		_rxExtended = frame.extd;
		_rxLength = frame.data_length_code;
		_rxIndex = 0;

		if (_rxLength > 8) {
			_rxLength = 8;
		}

		for (uint8_t i = 0; i < _rxLength; i++) {
			_rxData[i] = frame.data[i];
		}

		return _rxLength;
	}


	bool CAN_HANDLER::packetExtended(void) {
		return _rxExtended;
	}


	long CAN_HANDLER::packetId(void) {
		return _rxId;
	}


	bool CAN_HANDLER::send(CAN_MESSAGE message) {

		if (message.size > 8) {
			return false;
		}

		CanFrame frame = { 0 };
		frame.identifier = ((uint32_t)message.id << 18) | (message.uuid & 0x3FFFF);
		frame.extd = 1;
		frame.data_length_code = message.size;

		for (uint8_t i = 0; i < message.size; i++) {
			frame.data[i] = message.data[i];
		}

		return ESP32Can.writeFrame(frame);
	}


	uint8_t CAN_HANDLER::read(void) {

		if (_rxIndex >= _rxLength) {
			return 0;
		}

		return _rxData[_rxIndex++];
	}


#endif
