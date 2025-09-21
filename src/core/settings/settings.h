/*
 * LOCO-CAN module settings file
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020-2025
 * @lizence: GG0
 */

/*
Settings is initialzed with a can_com object, a 8-bit version value, a type value and 
the maximum count of setting values (not bytes). For correct communication the CAN-IDs#
for request, reply and setup must be added.

Each value declaration is registered with a 16 bit value with the format:
	bits 0-3 = size in bytes
	bits 4-7 = command type

The registration of a value increments the setting_count and adds the count of bytes
to the settings_index. After all values are registered, setting_count holds the number of
setting values and setting_index holds the number of bytes used.

The values can be accessed by the settings id (the index of values) or the value type.
The get_value_by_id and get_value_by_type methods return the count of bytes and
the value in the 4 byte buffer.
*/


#pragma once

#include "eeprom_setup.h"
#include "can_com.h"


// convert bytes to a 16/32 bit integer
#define char2int(MSB, LSB) (uint16_t)(((uint8_t) MSB & 0xFF) << 8 | ((int8_t) LSB & 0xFF))
#define char2long(MSB, LSB, MSB1, LSB1) (((uint32_t) ((uint8_t) MSB)) & 0xFF) << 24 | (((uint32_t) ((uint8_t) LSB)) & 0xFF) << 16 | (((uint32_t) ((uint8_t) MSB1)) & 0xFF) << 8 | (((int8_t) LSB1) & 0xFF)

#define MSB(BYTE) (uint8_t)(((uint8_t) BYTE >> 4) & 0x0F)
#define LSB(BYTE) (uint8_t)((uint8_t) BYTE & 0x0F)


struct SETTINGS_TYPE {
	uint8_t type;
	uint8_t size;
	uint8_t index;
};

struct VALUE {
	uint8_t size = 0;
	uint8_t* data = NULL;
};

extern CAN_COM can;


// =====================================
//
// CLASS HEADER
//
// =====================================

class SETTINGS {

	public:
		SETTINGS(void);

		void begin(uint16_t version, uint8_t type, uint8_t count, uint8_t name_size, uint16_t request_id, uint16_t reply_id, uint16_t setup_id);

		bool register_setting(uint16_t); // register settings type and size in bytes: 0xttss

		void load(void);
		void write(void);
		void init(void);

		uint16_t count(void);
		uint16_t bytes(void);

		void setStringMaxSize(uint8_t); // set maximum length for name per package (default=5)
		void setValueMaxSize(uint8_t); // set maximum length for values per package (default=6)

		// communication methods
		void sendGlobalRequest(void); // send request to all modules
		void sendRequest(uint16_t uuid); // send request to UUID-module
		void sendRequest(uint16_t uuid, uint8_t index); // send value request to UUID-module
		void sendRequest(uint16_t uuid, uint8_t index, uint8_t cnt); // send multiple value request to UUID-module

		void setupName(uint16_t uuid, String name); // send setup name data

		void setupValue(uint16_t uuid, uint8_t* value, uint8_t index); // send one setup value
		void setupValue(uint16_t uuid, uint8_t* values, uint8_t index, uint8_t cnt); // send setup values

		void sendInfoReply(uint8_t type); // reply module info
		void sendNameReply(void); // reply module name

		void sendValueReply(uint8_t index); // reply one module value
		void sendValueReply(uint8_t index, uint8_t size); // reply module values


		// storage methods
		char* getName(void);
		void setName(char*);

		int8_t find_setting(uint8_t); // find a setting value index by type; -1 if not found

		bool setValue(uint8_t, VALUE);
		VALUE getValue(uint8_t);
		VALUE get_by_type(uint8_t); // get a setting in buffer by type; return size; -1 if not found


	private:

		EEPROM_SETUP _setup;		
		CAN_COM* _can_com;

		uint8_t _max_count; // max count of settings
		uint8_t* _data = NULL;

		SETTINGS_TYPE* _declaration = NULL;
		uint8_t _setting_count; // count of registered settings
		uint8_t _setting_index; // last index in EEPROM

		uint8_t _string_max_size;
		uint8_t _value_max_size;

		uint16_t _request_id;
		uint16_t _reply_id;
		uint16_t _setup_id;

		uint16_t _version;
		uint8_t _type;

		uint8_t _i;
		uint8_t _buffer[8];
		uint8_t _max_settings;

		char* _name = NULL;
		uint8_t _max_name_size;

		void _sendVersion(uint8_t max_pack_cnt);
		void _sendName(uint16_t uuid, String name, uint8_t max_pack_cnt, uint16_t can_id);
		uint8_t _createCnt(uint8_t cnt, uint8_t max_cnt);
};