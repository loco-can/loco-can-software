#include "motor.h"


MOTOR::MOTOR(void) {}

void MOTOR::begin(uint8_t port) {
	_servo.attach(port);

	set_limits(SERVO_DEFAULT_MIN, SERVO_DEFAULT_MAX);
	set_value_limits(SERVO_DEFAULT_MIN_VALUE, SERVO_DEFAULT_MAX_VALUE);
}

void MOTOR::set_limits(uint16_t min, uint16_t max) {
	_min = min;
	_max = max;
}

void MOTOR::set_value_limits(uint16_t val_min, uint16_t val_max) {
	_val_min = val_min;
	_val_max = val_max;
}

void MOTOR::set(uint16_t value) {

	uint16_t calc;

	if (value > _val_max) {
		value = _val_max;
	}

	calc = map(value, _val_min, _val_max, _min, _max);

	_servo.write(calc);
}