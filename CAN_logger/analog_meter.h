#ifndef ANALOG_METER_H
#define ANALOG_METER_H


#include <Arduino.h>
#include "oled_display.h"


#define ANALOG_METER_WIDTH 18
#define ANALOG_METER_HEIGHT 80
#define ANALOG_METER_NAME_HEIGHT 10


class ANALOG_METER {

	public:
		void begin(char*, OLED_DISPLAY*, uint8_t, uint8_t, uint32_t, uint16_t); // display*, x-offset, y-offset, max value of full scale
		void update(uint16_t);
		void size(uint8_t, uint8_t);
		void width(uint8_t);
		void height(uint8_t);

	private:
		OLED_DISPLAY* _display;

		uint8_t _x;
		uint8_t _y;
		uint8_t _height;
		uint8_t _width;

		uint32_t _max_val;
		char* _name;
		uint16_t _color;

		uint32_t _value;

		void _draw(void);
};

#endif