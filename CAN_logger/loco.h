#ifndef LOCO_H
#define LOCO_H

#include <Arduino.h>
#include "oled_display.h"
#include "can_com.h"
#include "analog_meter.h"


class LOCO {
	public:
		void begin(OLED_DISPLAY*);
		void update(CAN_MESSAGE*);

	private:
		OLED_DISPLAY* _display;

		ANALOG_METER _battery_voltage;
		ANALOG_METER _motor_voltage;
		ANALOG_METER _current;
		ANALOG_METER _drive;
		ANALOG_METER _break; 
		ANALOG_METER _speed;

		uint16_t testval;
};

#endif