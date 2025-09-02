/*
 * IntelliTimeout class
 * 
 * @autor: Thomas Winkler
 * @copyright: 2019-11-17
 * @lizence: CC0
 *
 * change:
 *		add progress
 */


#include <Arduino.h>
#include "intelliTimeout.h"


INTELLITIMEOUT::INTELLITIMEOUT() {
}


void INTELLITIMEOUT::begin(uint16_t time) {

	_timeout = time;

	retrigger();
}


bool INTELLITIMEOUT::check(void) {

	return (_last_time + _timeout) < millis();
}


void INTELLITIMEOUT::retrigger(void) {

	_last_time = millis();
}


bool INTELLITIMEOUT::update(void) {

	if (check()) {
		retrigger();

		return true;
	}

	return false;
}

uint32_t INTELLITIMEOUT::rest(void) {

	if (millis() < (_last_time + _timeout)) {		
		return (_last_time + _timeout) - millis();
	}

	return false;
}

uint8_t INTELLITIMEOUT::progress(void) {

	return (millis() - _last_time) * 100 / _timeout;
}