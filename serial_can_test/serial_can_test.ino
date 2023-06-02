#include "loco_config.h"
#include "can_protocol.h"
#include "can_com.h"
#include "serialcan.h"

CAN_COM can_com;
CAN_MESSAGE message;

SERIALCAN s_can;

// servo = 9
// motor = 5
// controll = 6
#define CAN_STATUS_LED 9


void setup() {

	can_com.begin(CAN_BUS_SPEED, CAN_STATUS_LED);
	s_can.begin(115200);
}


void loop() {

	uint16_t filter;

	filter = can_com.read(&message);

	if (filter) {
		s_can.send(&message);
	}

}