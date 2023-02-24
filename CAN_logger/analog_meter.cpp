#include "analog_meter.h"


void ANALOG_METER::begin(char* name, OLED_DISPLAY* display, uint8_t x, uint8_t y, uint32_t max, uint16_t color) {

	_display = display;
	_name = name;
	_color = color;

	_value = 0;

	// set size to default
	size(ANALOG_METER_WIDTH, ANALOG_METER_HEIGHT);

	_x = x;
	_y = y;
	_max_val = max;

	_draw();
}


// set size
void ANALOG_METER::width(uint8_t w) {
	_width = w;
}

void ANALOG_METER::height(uint8_t h) {
	_height = h;
}

void ANALOG_METER::size(uint8_t w, uint8_t h) {
	_height = h;
	_width = w;
}


void ANALOG_METER::update(uint16_t val) {
	
	uint8_t v, dv, delta;
	uint8_t x, y, w, h;

	if (val > _max_val) {
		val = _max_val;
	}

	// update if has changed
	if (val != _value) {

		delta = abs(val - _value);

		v = (uint8_t)(((((float)val / (float)_max_val) * (float)_height)) - 2);
		dv = (uint8_t)(((((float)delta / (float)_max_val) * (float)_height)) - 2);

		x = _x + (_width / 2) + 5;
		y = _y + ANALOG_METER_NAME_HEIGHT + _height - v + 1;
		w = (_width / 2) - 5;

		// draw more of block
		if (val > _value) {
Serial.print("+");
Serial.print(v);
Serial.print(" ");
Serial.println(delta);
			_display->block(x, y, w, dv, _color);
		}

		// remove part of block
		if (val < _value) {
Serial.print("-");
Serial.print(v);
Serial.print(" ");
Serial.println(dv);
			_display->clear(x, y, w, dv);
		}

		// _display->clear(x, y, w, v);
		// _display->block(x, y, w, v, _color);
	}

	_value = val;
}


void ANALOG_METER::_draw(void) {

	_display->clear(_x, _y, _width, _height + ANALOG_METER_NAME_HEIGHT);

	_display->textSize(1);

	_display->position(_x, _y);
	_display->drawText(_name, GREY);

	_display->position(_x, _y + ANALOG_METER_NAME_HEIGHT + 1);
	_display->drawByte(_max_val / 1000, WHITE);

	if (_max_val >= 1000) {
		_display->position(_x, _y + ANALOG_METER_NAME_HEIGHT + (_height / 2) -4);
		_display->drawByte(_max_val / 2000, WHITE);
	}

	_display->position(_x, _y + _height + ANALOG_METER_NAME_HEIGHT - 8);
	_display->drawText("0", WHITE);

	_display->line(_x + (_width / 2), _y + ANALOG_METER_NAME_HEIGHT, _width / 2, WHITE);
	_display->vline(_x + (_width / 2) + 4, _y + ANALOG_METER_NAME_HEIGHT, _height, WHITE);
	_display->line(_x + (_width / 2), _y + _height + ANALOG_METER_NAME_HEIGHT, _width / 2, WHITE);
}