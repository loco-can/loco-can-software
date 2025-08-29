/*
 * Loco-CAN can class for ESP32
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 */

// use twai if is ESP32-S3 
#if BOARD_PLATFORM == ESP32S3

#include "esp32_can.h"
#include "can_com.h"
#include <ESP32-TWAI-CAN.hpp>


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
	} else {
		Serial.println("CAN bus failed!");
	}
}


bool CAN_HANDLER::available(void) {

}


uint16_t CAN_HANDLER::parsePacket(void) {

}


bool CAN_HANDLER::packetExtended(void) {

}


uint32_t CAN_HANDLER::packetId(void) {

}


bool CAN_HANDLER::send(CAN_MESSAGE message) {

	CanFrame ESP32_frame = { 0 };
    ESP32_frame.identifier = message.id << 18 | message.uuid;
    ESP32_frame.extd = 1;
    ESP32_frame.data_length_code = message.size;
    
}


uint8_t CAN_HANDLER::read(void) {

}

#endif