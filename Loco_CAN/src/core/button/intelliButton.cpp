/*
 * intellibutton class file
 * 
 * @author: Thomas H Winkler
 * @copyright: 2023
 * @lizence: GG0
 */

#include "intelliButton.h"


void INTELLIBUTTON::begin(uint8_t port) {
    begin(port, INTELLIBUTTON_DEFAULT_LONG);
}


void INTELLIBUTTON::begin(uint8_t port, uint16_t long_time) {
    _port = port;
    _pressed = false;

    set_long(long_time);
    pinMode(port, INPUT_PULLUP);
}


void INTELLIBUTTON::set_long(uint16_t long_time) {
    _long_push.begin(long_time);
}


// return if button is pushed
bool INTELLIBUTTON::pushed(void) {

    return digitalRead(_port) == 0;
}


uint8_t INTELLIBUTTON::check(void) {

    uint8_t state = 0;

    // button is pressed
    // (not pressed the input is in pullup mode)
    if (pushed() == true) {

        // just pushed => trigger long push timeout
        if (_pressed == false) {
            _long_push.retrigger();
        }

        _pressed = true;
    }

    // button not pressed
    else {

        // just released
        if (_pressed == true) {

            _pressed = false;

            // short push
            if (_long_push.check() == false) {
                state = 2;
            }

            else {
                state = 1;
            }
        }
    }

    return state;
}