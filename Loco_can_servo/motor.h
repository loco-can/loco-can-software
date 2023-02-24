#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>
#include <Servo.h>

#define SERVO_DEFAULT_MIN 0
#define SERVO_DEFAULT_MAX 180

#define SERVO_DEFAULT_MIN_VALUE 0
#define SERVO_DEFAULT_MAX_VALUE 1000

class MOTOR {

	public:
		MOTOR(void);
		void begin(uint8_t _port);
		void set_limits(uint16_t min, uint16_t max); // set min, max motor limits
		void set_value_limits(uint16_t, uint16_t); // set min max input value

		void set(uint16_t);

	private:
		Servo _servo;
		uint16_t _min;
		uint16_t _max;
		uint16_t _val_min;
		uint16_t _val_max;
};

#endif