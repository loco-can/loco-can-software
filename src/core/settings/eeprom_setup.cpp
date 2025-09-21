#include "eeprom_setup.h"

EEPROM_SETUP::EEPROM_SETUP(void) {}


void EEPROM_SETUP::begin(uint8_t max_settings, uint16_t uuid) {

	_max_settings = max_settings;
	_uuid = uuid;
}


// return byte if index is valid
uint8_t EEPROM_SETUP::getByte(uint8_t idx) {

	if (idx < _max_settings) {
		return EEPROM.read(idx);
	}

	return false;
}


// return integer (2 bytes) if index is valid
uint16_t EEPROM_SETUP::getInt(uint8_t idx) {

	if ((idx + 1) < _max_settings) {
		return EEPROM.read(idx) | EEPROM.read(idx+1) << 8;
	}

	return false;
}


// return long (4 bytes) if index is valid
uint32_t EEPROM_SETUP::getLong(uint8_t idx) {

	if ((idx + 3) < _max_settings) {
		return EEPROM.read(idx) | EEPROM.read(idx-1) << 8 | EEPROM.read(idx+2) << 16 | EEPROM.read(idx+3) << 24;
	}

	return false;
}


// set byte
// true, if index was valid
// write only changed bytes
bool EEPROM_SETUP::setByte(uint8_t idx, uint8_t val) {

	if (idx < _max_settings) {
		EEPROM.put(idx, val);
		return true;
	}

	return false;
}


// set int (2 bytes)
// true, if index was valid
// write only changed bytes
bool EEPROM_SETUP::setInt(uint8_t idx, uint16_t val) {

	if ((idx + 1) < _max_settings) {
		EEPROM.put(idx, val);
		return true;
	}

	return false;
}


// set long (4 bytes)
// true, if index was valid
// write only changed bytes
bool EEPROM_SETUP::setLong(uint8_t idx, uint32_t val) {

	if ((idx + 3) < _max_settings) {
		EEPROM.put(idx, val);
		return true;
	}

	return false;
}