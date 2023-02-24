#include "status.h"


void STATUS::begin(uint8_t max) {
	_max = max;
	_status = false;
}

void STATUS::set(uint8_t status) {

	if (status < _max) {
		_status = status;
	}
}

uint8_t STATUS::get(void) {
	return _status;
}

bool STATUS::is(uint8_t status) {
	return _status == status;
}

uint8_t STATUS::count(void) {
	return _max;
}

uint8_t STATUS::next(void) {
	if (_status < _max) {
		_status++;
	}
	else {
		_status = 0;
	}
}

uint8_t STATUS::previous(void) {
	if (_status > 0) {
		_status--;
	}
	else {
		_status = _max;
	}
}