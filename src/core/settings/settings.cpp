/*
 * LOCO-CAN module settings file
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */

#include "settings.h"
#include "can_com.h"


SETTINGS::SETTINGS(void) {}


// start setting
// init uuid and default name, if not valid in EEPROM
void SETTINGS::begin(CAN_COM* _can_com, uint16_t version, uint8_t type, uint8_t count, uint8_t name_size, uint16_t request, uint16_t reply, uint16_t setup) {

	_can_com = _can_com;

	_max_count = count;
	_declaration = new SETTINGS_TYPE[count];

	_max_name_size;
	_name = new char[_max_name_size];

	// count of settings and bytes
	_setting_count = 0;
	_setting_index = 0;

	// CAN communication IDs
	_request_id = request;
	_reply_id = reply;
	_setup_id = setup;

	_version = version; // module version
	_type = type; // module type

}


// register settings balue
// def: 0xttss with tt=type, ss=size in bytes
// _setting_count = count of settings
// _settings_index = count of bytes, depending on the settings declaration
bool SETTINGS::register_setting(uint16_t def) {

	uint8_t type = def >> 8;
	uint8_t size = def & 0xff;

	// check if enough space to store
	if (_setting_count <= _max_count) {

		_declaration[_setting_count].type = type;
		_declaration[_setting_count].size = size;
		_declaration[_setting_count].index = _setting_index;

		_setting_count++;
		_setting_index += size;

		return true;
	}

	return false;
}


// return max settings
uint16_t SETTINGS::count(void) {
	return _setting_count;
}


// return max bytes
uint16_t SETTINGS::bytes(void) {
	return _setting_index;
}


// init data with _setting_index bytes
// load EEPROM data
void SETTINGS::load(void) {

	uint8_t i;

	init();

	// load name from EEPROM
	if (_name != NULL) {
		for (i = 0; i < _max_name_size; i++) {
			_name[i] = _setup.getByte(i);
		}		
	}

	// load values from EEPROM
	if (_data != NULL) {
		for (i = _max_name_size; i < _setting_index; i++) {
			_data[i] = _setup.getByte(i);
		}
	}

	// init eeprom
	_setup.begin(_setting_index, _can_com->uuid());
}


// write data to EEPROM
// write only changed bytes
void SETTINGS::write(void) {

	if (_data != NULL) {

		// write name to EEPROM
		for (uint8_t i = 0; i < _max_name_size; i++) {
			_setup.setByte(i, _name[i]);
		}

		// write values to EEPROM
		for (uint8_t i = _max_name_size; i < _setting_index; i++) {
			_setup.setByte(i, _data[i]);
		}
	}
}


// initialize memory for settings
void SETTINGS::init(void) {

	// destroy _data if exists
	if (_data != NULL) {
		delete _data;
	}

	// allocate memory
	_data = new uint8_t[_setting_index];
}


// ================================================
//
// STORAGE methods


// return name
char* SETTINGS::getName(void) {
	return _name;
}


// set name and write to eeprom
void SETTINGS::setName(char* name) {
	_name = name;

	// write to EEPROM
	write();
}



// get data by type
// use _setup_declaration to find index and size
// return value (byte, int, long)

// set data in the same manner

// find a setting by type
// return the value index (is not the EEPROM address index)
int8_t SETTINGS::find_setting(uint8_t type) {

	uint8_t i;

	for (i = 0; i < _setting_count; i++) {

		// declaration found
		if (type == _declaration[i].type) {
			return i;
		}
	}

	return -1;
}


// set value by value index
// value has to be of value type (char, int, long)
bool SETTINGS::setValue(uint8_t index, VALUE value) {

	uint8_t size = value.size;
	uint8_t adr = _declaration[index].index;

	// valid index and valid value type
	if (index < _max_settings && size == _declaration[index].size) {

		// save value to data
		for (uint8_t i = 0; i < size; i++) {
			_data[adr + i] = value.data[i];
		}

		// write to EEPROM
		write();

		return true;
	}

	return false;
}


// get value by value index
// return value type
VALUE SETTINGS::getValue(uint8_t index) {

	// create return value
	VALUE value;

	uint8_t adr;
	uint8_t size = 0;
	value.size = size;

	// valid index
	if (index < _max_settings) {

		// get size and index from declaration
		adr = _declaration[index].index;
		size = _declaration[index].size;

		value.data = new uint8_t[size];

		// copy data to value
		for (uint8_t i = 0; i < size; i++) {
			value.data[i] |= _data[adr + i];
		}
	}

	return value;
}


// get value by value type
VALUE SETTINGS::get_by_type(uint8_t type) {

	int8_t idx;
	VALUE value;

	if ((idx = find_setting((uint8_t)type)) >= 0) {
		value = getValue(idx);
	}

	return value;
}


// ================================================
//
// REQUESTS

// global request
void SETTINGS::sendGlobalRequest(void) {

	_buffer[0] = 0xFF;

	_can_com->send(_buffer, 1, _request_id);

}

// UUID info request
void SETTINGS::sendRequest(uint16_t uuid) {

	_buffer[0] = MSB(uuid);
	_buffer[1] = LSB(uuid);

	_can_com->send(_buffer, 2, _request_id);

}

// value request
void SETTINGS::sendRequest(uint16_t uuid, uint8_t index) {

	_buffer[0] = MSB(uuid);
	_buffer[1] = LSB(uuid);

	_buffer[2] = index;

	_can_com->send(_buffer, 3, _request_id);

}

// multiple value request
// index = start index
// cnt = count of following bytes
void SETTINGS::sendRequest(uint16_t uuid, uint8_t index, uint8_t cnt) {

	_buffer[0] = MSB(uuid);
	_buffer[1] = LSB(uuid);

	_buffer[2] = index;
	_buffer[3] = cnt;

	_can_com->send(_buffer, 4, _request_id);

}


// ================================================
//
// REPLYS

// send info reply
void SETTINGS::sendInfoReply(uint8_t type) {

	// // get packet count (verson + name)
	// uint8_t packets = ceil((float)_setup.name().length() / 6) + 1;

	// // send version, type and name
	// _sendVersion(packets);
	// _sendName(0, _setup.name(), packets, _reply_id);

}



void sendNameReply(void) {

}


// send value reply with one byte
// buffer: buffer of values
void SETTINGS::sendValueReply(uint8_t index) {
	sendValueReply(index, 1);
}


void SETTINGS::sendValueReply(uint8_t index, uint8_t cnt) {

	_i = 0;

	if (cnt > 8) {
		cnt = 8;
	}

	while ((index + _i) < _setting_count && _i < cnt) {
		_buffer[_i] = _setup.getByte(index + _i);
		_i++;
	}

	_can_com->send(_buffer, _i, (_reply_id | (index + 1)) & 0x7F);

}


// ================================================
//
// SET REQUESTS

// send one value
void SETTINGS::setupValue(uint16_t uuid, uint8_t* value, uint8_t index) {
	setupValue(uuid, value, index, 1);
}


void SETTINGS::setupValue(uint16_t uuid, uint8_t* buffer, uint8_t index, uint8_t size) {

	if (index < 0xFF) {

		// add UUID to buffer
		_buffer[0] = MSB(uuid);
		_buffer[1] = LSB(uuid);


		// add data to buffer
		_i = 0;

		while (_i < size && _i < _value_max_size && (index + _i) < _setting_count) {
			_buffer[_i + 2] = _setup.getByte(index + _i);
			_i++;
		}

		// send
		_can_com->send(_buffer, _i + 2, _reply_id | (index + 1));
	}
}


// send name
void SETTINGS::setupName(uint16_t uuid, String name) {

	uint8_t max_packets = (ceil)((float)name.length() / _string_max_size);

	_sendName(uuid, name, max_packets, _setup_id);
}




// ================================================
//
// PRIVATE METHODS

// send version
// cnt = 0
// max_pack_cnt = count for multiple package send
void SETTINGS::_sendVersion(uint8_t max_pack_cnt) {

	_buffer[0] = _setting_count;
	_buffer[1] = _createCnt(0, max_pack_cnt);
	_buffer[2] = MSB(_version);
	_buffer[3] = LSB(_version);
	_buffer[4] = _type;

	_can_com->send(_buffer, 5, _reply_id);

}


// send name
// cnt >= 2
// max_pack_cnt = count for multiple package send
void SETTINGS::_sendName(uint16_t uuid, String name, uint8_t max_pack_cnt, uint16_t can_id) {

	uint8_t pack_cnt;
	uint8_t pos;

	// reset name position
	pos = 0;

	// loop over name packets
	for (pack_cnt = 0; pack_cnt < max_pack_cnt; pack_cnt++) {

		// reset index
		if (uuid > 0) {
			_buffer[0] = MSB(uuid);
			_buffer[1] = LSB(uuid);

			_i = 2;
		}

		else {
			_i = 0;
		}


		// set pack_cnt/max_packets
		_buffer[_i++] = _createCnt(pack_cnt, max_pack_cnt);

		// add name part data
		while (pos < _string_max_size && _i < 8) {
			_buffer[_i++] = name[pos++];
		}

		_can_com->send(_buffer, _i, can_id);
	}
}


// get cnt/max_cnt
uint8_t SETTINGS::_createCnt(uint8_t cnt, uint8_t max_cnt) {
	return (cnt & 0x0F) << 4 | (max_cnt & 0x0F);
}
