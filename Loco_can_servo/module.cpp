#include "module.h"


void MODULE::begin(CAN_COM* can_com) {
	_can_com = can_com;

	_servo[0].begin(SERVO_1);
	_servo[1].begin(SERVO_2);
	_servo[2].begin(SERVO_3);
	_servo[3].begin(SERVO_4);
}

void MODULE::update(void) {

	
}