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

#include "../../../config.h"

#include "../uniqueID/ArduinoUniqueID.h"
#include "../hash/RokkitHash.h"
#include "../timeout/intelliTimeout.h"

#include "can_com.h"


#define DEBUG

/*
 * create CAN communication
 * user standard CS (10) and INT (2) ports
 */
CAN_COM::CAN_COM() {
  create_uuid();
}


/*
 * create CAN communication
 * Arduino set CS and INT ports MCP2515
 * ESP32 set RX and TX pins
 */
CAN_COM::CAN_COM(uint8_t CS, uint8_t INT) {

  _cs = CS;
  _int = INT;

  create_uuid();
}


// begin can_com
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

  // status led[s] on
  _led_r.on();

  if (_led_w.available()) {
    _led_w.on();
  }

  // start the CAN bus
  #ifdef DEBUG
    Serial.print("Start CAN at ");
    Serial.print(speed);
    Serial.println(" bps");

    #ifdef ARDUINO_ARCH_ESP32
      Serial.print("SJA1000 RX ");
      Serial.print(_cs);
      Serial.print(" - TX ");
      Serial.println(_int);
    #else
      Serial.print("MCP2551 CS ");
      Serial.print(_cs);
      Serial.print(" - INT ");
      Serial.println(_int);
    #endif
  #endif
  
  while (!_can_handler.begin(speed, _cs, _int)) {

    #ifdef DEBUG
      Serial.println("Starting CAN failed!");
    #endif

    // flash status light[s]
    _led_r.on();
    if (_led_w.available()) {
      _led_w.on();
    }
    delay(250);

    _led_r.off();
    if (_led_w.available()) {
      _led_w.off();
    }

    delay(1000);
  }


  #ifdef DEBUG
    Serial.print("CAN status led port ");

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

    Serial.print("Device UUID: ");
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
    Serial.println("CAN startup OK!");
    Serial.println();
  #endif

  return true;
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
}


/*
 * check if CAN communication alive
 * timeout 
 */
bool CAN_COM::alive(void) {
  return _alive;
}

/*
 * send data package
 */
bool CAN_COM::send(uint8_t* data, uint8_t length, uint32_t id) {

  uint8_t i = 0;
  CAN_MESSAGE message;

  delay(10);

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
  message.id = id;
  message.uuid = _uuid;
  message.size = length;

  while (i < length && i < 8) {
    message.data[i] = data[i];
    i++;  
  }

  _can_handler.send(message);


  // LEDs off
  if (_led_w.available()) {
    _led_w.off();
  }
  else {
    _led_r.off();
  }

  return true;
}


/*
 * read data, return true if filter
 */
uint16_t CAN_COM::read(CAN_MESSAGE* message) {

  uint8_t i;
  uint8_t size;
  uint32_t can_id;

  // check and connection and update alive status
  _alive = !_alive_timeout.check();


  // ===============================================
  // check for package
  size = _can_handler.parsePacket();

  // received a package
  if (size) {

    _led_r.on();

    // retrigger connection timeout
    _alive_timeout.retrigger();


    // fetch data
    i = 0;
    while (_can_handler.available() && i < 8) {
      message->data[i++] = (uint8_t)_can_handler.read();
    }

    // add size
    message->size = i;
    
    // get packet id
    // is extended
    // split in group id (11 bit) and uuid (18 bit)
    if (_can_handler.packetExtended()) {

      can_id = _can_handler.packetId();

      message->id = can_id >> 18;
      message->uuid = can_id & 0x3FFFF;
    }


    else {
      message->id = _can_handler.packetId();
      message->uuid = 0;
    }
    
    _led_r.off();

    // check for filter criteriy
    if (_filter_count == 0) {
      return message->id;
    }

    // check for registered filters
    i = 0;
    while (i < _filter_count) {

      // filter found
      if ((message->id & _masks[i]) == _filters[i]) {
        return _filters[i];
      }

      i++;
    }

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
