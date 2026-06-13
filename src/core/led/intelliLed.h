/*
 * INTELLILED class
 * 
 * @autor: Thomas Winkler
 * @copyright: 2019-11-17
 * @lizence: CC0
 */

/*
 * This software is open source under the creative commons CC0 license
 */
 
/*
 * The INTELLILED class controlles an output for display.
 * With set_blink a blink style can be added, updated with the blink() method.
 * 
 * blink(uint16_t): set the blink time in ms
 * flash(): the INTELLILED only flashes 10 ms in the speed set by time()
 *
 * on(): switch the led on
 * off(): switch the led off
 *
 * color(COLOR): set a color when two ports are defined
 * offColor(COLOR): set a color instead of off state
 * 
 * forceFlash(time): set a time to flash, even at on state
 */
#pragma once

#ifndef INTELLILED_H
#define INTELLILED_H


#include <Arduino.h>


#define INTELLILED_RED 0
#define INTELLILED_GREEN 1
#define INTELLILED_YELLOW 2


class INTELLILED {

  public:
    INTELLILED(void);
    INTELLILED(uint16_t port);
    INTELLILED(uint16_t port, uint16_t port1);

    void begin(uint16_t port);
    void begin(uint16_t port, uint16_t port1);

    uint8_t available(void);

    uint16_t port(void);
    uint16_t port1(void);

    void setBlink(uint16_t);
    void forceBlink(uint16_t);
    void blink(uint16_t);
    void flash(uint16_t);
    void set(bool);
    void on(void);
    void off(void);
    void color(uint16_t);
    void color(uint16_t, uint16_t);
    void offColor(uint16_t);
    void toggle(void);
    void update(void);

  private:
    void _begin(uint16_t, uint16_t);
    void _set_led(uint16_t);
    void _clear_led(void);
    void _on(void);
    void _off(void);
    void _flash_on(void);
    void _flash_off(void);
    void _reset(void);

    bool _is_on;
    uint16_t _force_blink;
    bool _flash_status;

    uint16_t _led_port;
    uint16_t _led_port1;

    uint8_t _led_count;
    
    uint16_t _led_color;
    uint16_t _led_color1;
    bool _off_color;
    
    double _timeout;
    uint16_t _blink_time;
};

#endif