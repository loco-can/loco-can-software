#ifndef EEPROM_SETUP_H
#define EEPROM_SETUP_H

#include <Arduino.h>
#include <EEPROM.h>

class EEPROM_SETUP {

	public:
		EEPROM_SETUP(void);
		void begin(uint8_t, uint16_t);

		uint8_t getByte(uint8_t);
		uint16_t getInt(uint8_t);
		uint32_t getLong(uint8_t);

		bool setByte(uint8_t, uint8_t);
		bool setInt(uint8_t, uint16_t);
		bool setLong(uint8_t, uint32_t);

	private:
		uint8_t _max_settings;
		uint16_t _uuid;
};

#endif