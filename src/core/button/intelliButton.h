/*
 * intellibutton class header file
 * 
 * @author: Thomas H Winkler
 * @copyright: 2023
 * @lizence: GG0
 */

/*
 * Check for pressed putton on port
 * The input is pulled up, so the pressed button must be connected to GND
 *
 * A long push timeout is used to detect, if the button is pressed longer than
 * the set time. The program execution is not interrupted while the button is pressed.
 *
 * return values:
 *      0: not pressed
 *      1: short push
 *      2: long push
 */ 

#ifndef INTELLIBUTTON_H
#define INTELLIBUTTON_H


#include <Arduino.h>
#include "../timeout/intelliTimeout.h"

// default value for long push detection = 1000ms
#define INTELLIBUTTON_DEFAULT_LONG 1000


class INTELLIBUTTON {

    public:
        void begin(uint8_t port);
        void begin(uint8_t port, uint16_t long_time);
        
        void set_long(uint16_t long_time);

        // check if the button is pushed
        bool pushed(void);

        // check if button is pushed long
        uint8_t check(void);

    private:
        uint8_t _port;
        INTELLITIMEOUT _long_push;
        bool _pressed;

};

#endif