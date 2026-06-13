/*
 * Loco-CAN can class for ESP32
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 */

#include "../../config.h"

// use twai if is ESP32-S3 
#ifdef MODULE_ARCH_ESP32

	#include "ESP32_can.h"
	#include "ESP32-TWAI-CAN.hpp"
	#include "driver/twai.h"


	// #include "can_com.h"
	// #include <ESP32-TWAI-CAN.hpp>


	bool CAN_HANDLER::begin(long speed, uint16_t can_rx, uint16_t can_tx) {

		ESP32Can.setPins(can_rx, can_tx);

	    // You can set custom size for the queues - those are default
		// ESP32Can.setRxQueueSize(ESP_QUEUE_SIZE);
		// ESP32Can.setTxQueueSize(ESP_QUEUE_SIZE);


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

	}


	uint16_t CAN_HANDLER::parsePacket(void) {

		return 0;
	}


	bool CAN_HANDLER::packetExtended(void) {

	}


	long CAN_HANDLER::packetId(void) {

	}


	bool CAN_HANDLER::send(CAN_MESSAGE message) {

		if (message.size > 8) {
			return false;
		}

		CanFrame frame = { 0 };
		frame.identifier = ((uint32_t)message.id << 18) | (message.uuid & 0x3FFFF);
		frame.flags = TWAI_MSG_FLAG_EXTD;
		frame.data_length_code = message.size;

		for (uint8_t i = 0; i < message.size; i++) {
			frame.data[i] = message.data[i];
		}

		return ESP32Can.writeFrame(frame);
	}


	uint8_t CAN_HANDLER::read(void) {

	}


#endif