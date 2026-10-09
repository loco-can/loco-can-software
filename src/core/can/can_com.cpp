/*
 * Loco-CAN can communication class
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020
 * @lizence: GG0
 */

/*
 * create can communication
 */

#include "../../config.h"

#include "../uniqueID/ArduinoUniqueID.h"
#include "../hash/RokkitHash.h"
#include "../timeout/intellitimeout.h"

#include "can_com.h"
#include "../../can_protocol.h"

// CAN_MESSAGE can_message;


/* ************************************************
* CONSTRUCTOR
************************************************ */
/*
 * create CAN communication
 * user standard CS (10) and INT (2) ports
 */
CAN_COM::CAN_COM() {
	create_uuid();
	_alive = false;
	_int = -1;
}


/*
 * create CAN communication
 * Arduino: MCP2515 SS and INT
 * ESP32: RX and TX pins
 */
CAN_COM::CAN_COM(uint8_t ss, uint8_t irq) {

	setPorts(ss, irq);
	create_uuid();
	_alive = false;
}


void CAN_COM::setPorts(uint8_t ss) {
	_cs = ss;
	_int = -1;
}


void CAN_COM::setPorts(uint8_t ss, uint8_t irq) {
	_cs = ss;
	_int = irq;
}



/* ************************************************
* BEGIN
************************************************ */
// use one led for status of can activity
bool CAN_COM::begin(long speed, uint8_t led_port) {

	_led_r.begin(led_port);

	return _begin(speed);
}


// begin can_com
// use separate leds for read and write status of can 
bool CAN_COM::begin(long speed, uint8_t led_port1, uint8_t led_port2) {

	_led_r.begin(led_port1);
	_led_w.begin(led_port2);

	return _begin(speed);
}


bool CAN_COM::_begin(long speed) {

	#ifdef DEBUG
	Serial.println("*******************");
	Serial.println("start core/can_com");
	#endif

	// status led[s] on
	_led_r.on();

	if (_led_w.available()) {
		_led_w.on();
	}

	// start the CAN bus
	#ifdef DEBUG
	Serial.print("> Start CAN at ");
	Serial.print(speed);
	Serial.println(" bps");

	#ifdef MODULE_ARCH_ESP32
		Serial.print("  > using TWAI RX ");
		Serial.print(_cs);
		Serial.print(" - TX ");
		Serial.println(_int);
	#else
		Serial.print("  > using MCP2515 SS ");
		Serial.print(_cs);
		if (_int >= 0) {
			Serial.print(" - INT ");
			Serial.println(_int);
		}
		else {
			Serial.println(" - INT not connected");
		}
	#endif
	#endif
	
	uint8_t tries = 0;

	while (!_can_handler.begin(speed, _cs, _int)) {

		tries++;

		#ifdef DEBUG
			#ifdef MODULE_ARCH_ESP32
				Serial.print("*** CAN controller not answering (TWAI RX ");
				Serial.print(_cs);
				Serial.print(" TX ");
				Serial.print(_int);
			#else
				Serial.print("*** CAN controller not answering (MCP2515 SS ");
				Serial.print(_cs);
				if (_int >= 0) {
					Serial.print(" INT ");
					Serial.print(_int);
				}
			#endif
			Serial.print(") try ");
			Serial.print(tries);
			Serial.print('/');
			Serial.println(CAN_BEGIN_TRIES);
		#endif

		_flash_can_led();

		if (tries >= CAN_BEGIN_TRIES) {
			#ifdef DEBUG
				Serial.println("*** CAN controller failed, aborting");
			#endif

			for (;;) {
				_flash_can_led();
			}
		}
	}


	#ifdef DEBUG
		Serial.print("> CAN status led port ");

		if (_led_w.available()) {
			Serial.print("r on port ");
			Serial.print(_led_r.port());
			Serial.print(", w on port ");
			Serial.println(_led_w.port());
		}

		else {
			Serial.print("r/w on port ");
			Serial.println(_led_r.port());
		}

		Serial.print("> Device UUID: ");
		Serial.println(uuid(), HEX);
	#endif

	_filter_count = 0;
	
	set_alive(CAN_ALIVE_TIMEOUT);

	delay(150);

	// status led[s] off
	_led_r.off();

	if (_led_w.available()) {
		_led_w.off();
	}


	#ifdef DEBUG
	Serial.println("CAN is up and running!");
	Serial.println("**********************");
	Serial.println();
	#endif

	return true;
}


void CAN_COM::_flash_can_led(void) {

	for (uint8_t i = 0; i < CAN_ERROR_FLASHES; i++) {
		_led_r.on();
		if (_led_w.available()) {
			_led_w.on();
		}
		delay(100);

		_led_r.off();
		if (_led_w.available()) {
			_led_w.off();
		}
		delay(100);
	}
}


void CAN_COM::create_uuid(void) {
	_uuid = rokkit((char*) UniqueID8, 8) & 0xFFFF; // 0x3FFFF
}


long CAN_COM::uuid(void) {
	return _uuid;
}


/*
 * set alive timeout
 */
void CAN_COM::set_alive(uint16_t alive_timeout) {
	_alive_timeout.begin(alive_timeout);
	_alive = false;
}


/*
 * check if CAN communication alive
 * timeout 
 */
bool CAN_COM::alive(void) {
	return _alive;
}


/* ************************************************
***************************************************
 * send data package
***************************************************
************************************************ */


void CAN_COM::print_message(CAN_MESSAGE message) {

	uint8_t j;

	Serial.print("id=");
	Serial.print(message.id, HEX);
	Serial.print(", uuid=");
	Serial.print(message.uuid, HEX);
	Serial.print(", size=");
	Serial.print(message.size);
	Serial.print(", data: ");

	for (j = 0; j < message.size; j++) {
		Serial.print(message.data[j], HEX);
		Serial.print(".");
	}

	Serial.println();
}


/*
 * check for new message from can controller
 * add to fifo if available
 * return entry from fifo buffer
 *
 * if no message is in buffer, return message with uuid = 0
 */
uint16_t CAN_COM::read(CAN_MESSAGE &message) {

	uint16_t filter;

	// ===============================
	// get message from can controller
	filter = _read(message);

	// valid message received
	if (filter) {
		// add(message);
	}

	// no frame, or a frame that did not match a filter
	else {
		message.id = 0;
		message.uuid = 0;
		message.size = 0;
	}

	return filter;
}



/* ************************************************
* SEND MESSAGE TO CAN CONTROLLER
************************************************ */
// uint8_t* data, uint8_t length, uint32_t id
bool CAN_COM::send(CAN_MESSAGE message) {

	// blink write status led if exists, else read
	if (_led_w.available()) {
		_led_w.on();
	}
	else {
		_led_r.on();
	}

	// begin packet
	// use 29 bit identifier
	// 11 bit: id
	// 18 bit: board uuid
	message.uuid = uuid();
	const bool sent = _can_handler.send(message);

	
	// LEDs off
	if (_led_w.available()) {
		_led_w.off();
	}
	else {
		_led_r.off();
	}

	return sent;
}


/*
 * send a frame that already belongs to another node
 * used by the WIFI bridge so the original UUID stays in the identifier
 */
bool CAN_COM::forward(CAN_MESSAGE message) {

	if (_led_w.available()) {
		_led_w.on();
	}
	else {
		_led_r.on();
	}

	const bool sent = _can_handler.send(message);

	if (_led_w.available()) {
		_led_w.off();
	}
	else {
		_led_r.off();
	}

	return sent;
}

// uint8_t* data, uint8_t length, uint32_t id
bool CAN_COM::send(uint32_t id, uint8_t* data, uint8_t size) {
	CAN_MESSAGE message;

	message = data2message(id, uuid(), data, size);

	return send(message);
}


/* ************************************************
* READ MESSAGE FROM CAN CONTROLLER
************************************************ */
/*
 * read data, return true if filter
 */
bool CAN_COM::fetch(CAN_MESSAGE &message) {

	uint8_t i;
	uint32_t can_id;

	_alive = !_alive_timeout.check();

	/*
	 * A heartbeat is a valid frame with DLC 0. parsePacket() returns that
	 * length, which is also what an empty mailbox used to look like.
	 * packetId() < 0 is the empty mailbox (set by the platform driver).
	 */
	_can_handler.parsePacket();

	if (_can_handler.packetId() < 0) {
		message.id = 0;
		message.uuid = 0;
		message.size = 0;
		return false;
	}

	_led_r.on();
	_alive_timeout.retrigger();

	i = 0;
	while (_can_handler.available() && i < 8) {
		message.data[i++] = (uint8_t)_can_handler.read();
	}

	message.size = i;

	if (_can_handler.packetExtended()) {
		can_id = _can_handler.packetId();
		message.id = can_id >> 18;
		message.uuid = can_id & 0x3FFFF;
	}
	else {
		message.id = _can_handler.packetId();
		message.uuid = 0;
	}

	_led_r.off();
	return true;
}


uint16_t CAN_COM::_read(CAN_MESSAGE &message) {

	uint8_t i;

	if (!fetch(message)) {
		return 0;
	}

	// check for filter criteriy
	if (_filter_count == 0) {
		return message.id ? message.id : 1;
	}

	// check for registered filters
	i = 0;
	while (i < _filter_count) {

		// filter found
		if ((message.id & _masks[i]) == _filters[i]) {
			return _filters[i] ? _filters[i] : 1;
		}

		i++;
	}

	return false;
}


void CAN_COM::clear_filter() {
	_filter_count = 0;

}


/*
 * filter packet id
 true if valid
 */
bool CAN_COM::register_filter(uint16_t mask, uint16_t filter) {

	// has free filter slots
	if (_filter_count < CAN_MAX_FILTER) {

	_masks[_filter_count] = mask;
	_filters[_filter_count] = filter;

	_filter_count++;

	}

	return _filter_count;
}


// return can_message struct from data
CAN_MESSAGE CAN_COM::data2message(uint32_t id, uint16_t uuid, uint8_t* data, uint8_t size) {

    can_message.id = id;
    can_message.uuid = uuid & 0x3FFFF;

    if (size > 8) {
        size = 8;
    }

    for (uint8_t i = 0; i < size; i++) {
        can_message.data[i] = data[i];
    }

    can_message.size = size;

    return can_message;
}