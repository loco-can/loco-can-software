/*
 * LOCO-CAN can class _header file
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020-2025
 * @lizence: GG0
 * 
 * using can library:
 * https://github.com/sandeepmistry/arduino-CAN
 * https://github.com/ricaun/ArduinoUniqueID
 * https://github.com/SukkoPera/Arduino-Rokkit-Hash
 */

#pragma once


#include <Arduino.h>
#include "../../../config.h"
#include "can_message.h"


// select platform
#ifdef MODULE_PLATFORM_ATMEGA
  #include "atmega_can.h"
#endif

#ifdef MODULE_PLATFORM_ESP32
  #include "ESP32_can.h"
#endif

#include "../timeout/intelliTimeout.h"
#include "../led/intelliLed.h"


/*
 * CAN speeds
 * 
 * 1000 bps = 1000E3
 * 500 bps = 500E3
 * ...
 * 
 * valid speed settings
 * 1000, 500, 250, 200, 125, 100, 80, 50, 40, 20, 10 ,5
 */


#define CAN_ALIVE_TIMEOUT 500

// #define CAN_COM_CS_DEFAULT 10
// #define CAN_COM_INT_DEFAULT 2

class CAN_COM {

  public:
    CAN_COM(void); // construct with default CS and INT port
    CAN_COM(uint8_t CS, uint8_t INT); // user CS and INT ports

    void setPorts(uint8_t CS, uint8_t INT); // set user CS and INT ports

    bool begin(long speed, uint8_t led_port); // start communication with speed setting and status LED
    bool begin(long speed, uint8_t led_port1, uint8_t led_port2); // start communication with speed setting and r/w status LEDs

    bool alive(void);
    void set_alive(uint16_t alive_timeout); // set alive timeout
    
    void buffer_reset(void); // reset fifo buffer
    void print_buffer(void); // print buffer to serial
    void print_message(CAN_MESSAGE message); // print message to serial

    bool add(CAN_MESSAGE message); // add a message to the fifo buffer

    uint16_t read(CAN_MESSAGE message); // get message from buffer
    bool send(void); // send message from biffer

    bool fetch(CAN_MESSAGE &message); // fetch message from buffer; false if empty
    uint8_t buffer_size(void); // get size of buffer

    void clear_filter();
    bool register_filter(uint16_t mask, uint16_t filter); // add mask and filter

    long uuid(void);

  private:

    CAN_HANDLER _can_handler; // handler depending on platform

    CAN_MESSAGE _buffer[CAN_BUFFER_SIZE]; // fifo buffer of can messages
    uint8_t _buffer_count; // count of messages in buffer
    uint8_t _head;
    uint8_t _tail;

    void create_uuid(void);
    bool _begin(long speed);
    bool _send(CAN_MESSAGE message); // send data
    uint16_t _read(CAN_MESSAGE message); // receive data > true if no filter or filter match

    bool isEmpty(void);
    bool isFull(void);

    long _uuid;

    uint8_t _cs;
    uint8_t _int;
    
    uint16_t _masks[CAN_MAX_FILTER];
    uint16_t _filters[CAN_MAX_FILTER];

    uint8_t _filter_count;

    INTELLITIMEOUT _alive_timeout;
    bool _alive;

    INTELLILED _led_r;
    INTELLILED _led_w;
};
