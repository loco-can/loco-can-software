/*
 * LOCO-CAN maesure value class
 * 
 * @author: Thomas H Winkler
 * @copyright: 2023
 * @lizence: GG0
 */

#include <Arduino.h>
#include "intelliValue.h"


/*
 * init measure value with reference
 */
bool INTELLIVALUE::begin(uint16_t reference) {

    begin(reference, 0);

    return true;
}


bool INTELLIVALUE::begin(uint16_t reference,  uint8_t index) {

    uint16_t precission;
    uint8_t places = 0;

    // calculate precission of reference value
    // maximal decimal places dependent of reference value
    // the reference is 
    precission = 65535 / reference;

    while (precission > 0) {
        precission /= 10;
        places++;
    }

    // correct places
    if (places > 0) {
        places--;
    }

    // set calculated values
    _value.precission = places;
    _value.reference = reference * pow(10, places);

    // limit lindex to 5 bits
    _value.index = 0;

    if (index < 32) {
        _value.index = index;
        return true;
    }

    return false;
}


// set absolute value in precission
void INTELLIVALUE::set(uint16_t value) {
    _value.percentage = (uint16_t)(((value * pow(10, _value.precission)) / _value.reference));
}


// get absolute value
uint16_t INTELLIVALUE::get(void) {
    return (uint16_t)(((uint32_t)_value.percentage * (uint32_t)_value.reference) / 100);
}


// set values from message package
void INTELLIVALUE::set_package(char* package) {
    _value.percentage = package[1] | ((package[0] & 0x07) >> 8);
    _value.index = package[0] >> 5;
    _value.reference = (package[2] << 8) | package[3];
}


// get package from values
void INTELLIVALUE::get_package(char* data) {

    data[0] = (_value.index << 5) | (_value.percentage >> 8);
    data[1] = _value.percentage & 0xff;
    data[2] = _value.reference >> 8;
    data[3] = _value.reference & 0xff;
}


// get reference value
uint16_t INTELLIVALUE::reference(void) {
    return _value.reference;
}

// set reference value
void INTELLIVALUE::reference(uint16_t val) {
    _value.reference = val;
}


// get precission value
uint16_t INTELLIVALUE::precission(void) {
    return _value.precission;
}

// set precission value
void INTELLIVALUE::precission(uint16_t val) {
    _value.precission = val;
}


// get percentage
uint16_t INTELLIVALUE::percentage(void) {
    return _value.percentage;
}

// set percentage
void INTELLIVALUE::percentage(uint16_t val) {
    _value.percentage = val; // & 0b11111111111;
}


// get index
uint8_t INTELLIVALUE::index(void) {
    return _value.index;
}

// set index
void INTELLIVALUE::index(uint8_t val) {
    _value.index = val;
}