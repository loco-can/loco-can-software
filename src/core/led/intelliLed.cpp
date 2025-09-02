/*
 * INTELLILED class
 * 
 * @autor: Thomas Winkler
 * @copyright: 2019-11-17
 * @lizence: CC0
 */

#include "intelliLed.h"


/*
 * construct INTELLILED
 * port = OUTPUT PORT #
 * optional: port1 = OUTPUT for multi color led
 */
INTELLILED::INTELLILED(void) {
}


INTELLILED::INTELLILED(uint16_t port) {
  begin(port);
}


INTELLILED::INTELLILED(uint16_t port, uint16_t port1) {
  begin(port, port1);
}


// init with one port
void INTELLILED::begin(uint16_t port) {

  _led_count = 1;

  _begin(port, false);
}


// init with two ports for dual led
void INTELLILED::begin(uint16_t port, uint16_t port1) {

  _led_count = 2;

  _begin(port, port1);
}


void INTELLILED::_begin(uint16_t port, uint16_t port1) { 

  _led_port = port;
  _led_port1 = port1;

  _led_color = 0;
  _blink_time = 0;
  _force_blink = 0;

  _timeout = millis();

  // set INTELLILED ports to OUTPUT
  pinMode(_led_port, OUTPUT);
  
  if (_led_port1) {
	pinMode(_led_port1, OUTPUT);
  }

  _reset();
}


/*
 * check if leds are initialized
 * 1: one led
 * 2: two leds
 * 0: no led
 */
uint8_t INTELLILED::available(void) {

  return _led_count;
}


/*
 * get port of led 1
 */
uint16_t INTELLILED::port(void) {
  return _led_port;
}

/*
 * get port of led 2
 */
uint16_t INTELLILED::port1(void) {
  return _led_port1;
}

/*
 * update led: blink or flash
 * is to be called in the main loop
 */
void INTELLILED::update(void) {

  if (_force_blink) {
	_blink_time = _force_blink;
  }


  // blink or flash
  if (_blink_time != 0) {

	// timed out
	if (millis() > (_timeout + (uint16_t)_blink_time)) {

	  // flash
	  if (_flash_status == true) {
		_on();
		delay(10);
		_off();
	  }

	  // blink
	  else {
		toggle();
	  }

	  _timeout = millis();
	}
  }
}


/*
 * toggle INTELLILED
 */
void INTELLILED::toggle(void) {

  if (_is_on) {
  
	_off();
	_is_on = false;
  }

  else {
	_on();
  }
}


/*
 * set blink with time in ms
 */
void INTELLILED::blink(uint16_t blink_time) {

  _flash_off();
  _blink_time = blink_time;
}


/*
 * set ony blink time in ms
 */
void INTELLILED::setBlink(uint16_t blink_time) {

  _blink_time = blink_time;
}


/*
 * force to blink, even if only on with time in ms
 */
void INTELLILED::forceBlink(uint16_t blink_time) {

  _force_blink = blink_time;
}


/*
 * set flash time in ms
 * INTELLILED is on for 5mx
 */
void INTELLILED::flash(uint16_t blink_time) {
  _blink_time = blink_time;
  _flash_on();
}


/*
 * switch led on
 */
void INTELLILED::set(bool status) {

	if (status) {
		on();
	}
	else {
		off();
	}
}


/*
 * switch led on
 */
void INTELLILED::on(void) {

  if (!_force_blink) {
	_reset();
	_on();
  }
}

/*
 * switch led off
 */
void INTELLILED::off(void) {

	_reset();
}



/*
 * set color for multicolor led
 * don't effect normal led
 */

void INTELLILED::color(uint16_t color) {
  _led_color = color;
  _off_color = false;
}

void INTELLILED::color(uint16_t color, uint16_t color1) {
  _led_color = color;
  offColor(color1);
}

void INTELLILED::offColor(uint16_t color) {
  _led_color1 = color;
  _off_color = true;
}


/*
 * reset blink time and switch off
 */
void INTELLILED::_reset(void) {

  if (!_force_blink) {
	blink(0);
  }

  _off();
}


/*
 * set led with colour
 */
void INTELLILED::_set_led(uint16_t color) {

	switch(color) {
		
		case INTELLILED_RED:
		  digitalWrite(_led_port, HIGH);
		  digitalWrite(_led_port1, LOW);
		  break;
  
		case INTELLILED_GREEN:
		  digitalWrite(_led_port, LOW);
		  digitalWrite(_led_port1, HIGH);
		  break;
  
	   case INTELLILED_YELLOW:
		  digitalWrite(_led_port, HIGH);
		  digitalWrite(_led_port1, HIGH);
		  break;
	}
}


/*
 * switch leds off
 */
void INTELLILED::_clear_led() {

  digitalWrite(_led_port, LOW);

  if (_led_port1) {
	digitalWrite(_led_port1, LOW);
  }
}


/*
 * set led port[s] on
 */
void INTELLILED::_on(void) {

  _is_on = true;

  // multicolor INTELLILED
  if (_led_port1) {
	_set_led(_led_color);
  }
  else {
	digitalWrite(_led_port, HIGH);
  }
}


/*
 * set led ports off
 */
void INTELLILED::_off(void) {

  _is_on = false;

  // has off color
  if (_off_color) {

	if (_flash_status) {
	  _clear_led();
	  delay(50);
	  _set_led(_led_color1);
	  delay(10);
	  _clear_led();
	}

	else {
	  _set_led(_led_color1);
	}

  }

  // light off
  else {
	_clear_led();
  }
}

/*
 * switch flash mode on
 */
void INTELLILED::_flash_on(void) {
  _flash_status = true;
}

/*
 * switch flash mode off
 */
void INTELLILED::_flash_off(void) {
  _flash_status = false;
}
