/*
 * LOCO-CAN measure class
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020-2025
 * @lizence: GG0
 */

#include <Arduino.h>
#include "measure.h"

MEASURE::MEASURE(void) {
}


/*
 * begin: init port
 */
void MEASURE::begin(uint8_t port) {
    begin(port, -1);
}


void MEASURE::begin(uint8_t port_1, int8_t port_2) {

    calibrate(1); // set calibration to 1
    setResolution(10); // set to 10 bits as default

    set_filter(0);

    _port_1 = port_1;
    _port_2 = port_2;

    pinMode(port_1, INPUT);

    if (port_2 != -1) {
        pinMode(port_2, INPUT);
    }
}


void MEASURE::begin(uint8_t port_1, uint16_t res, uint16_t cal) {

    begin(port_1);

    setResolution(res);
    calibrate(cal);
}


void MEASURE::begin(uint8_t port_1, int8_t port_2, uint16_t res, uint16_t cal) {

    begin(port_1, port_2);

    setResolution(res);
    calibrate(cal);
}


// set maximal value
void MEASURE::setResolution(uint8_t bits) {
    _max_val = (1 << bits) - 1;
}


// set calibration
void MEASURE::calibrate(float cal) {
    _cal = cal;
}


// set LP filter
void MEASURE::set_filter(float time) {
    _Tf = time;
}

// low pass filtering function
float MEASURE::filter(float input) {

    uint32_t timestamp = micros();

    float dt = (timestamp - _timestamp_prev)*1e-6f;

    // quick fix for strange cases (micros overflow)
    if (dt < 0.0f || dt > 0.5f) dt = 1e-3f;

    // calculate the filtering 
    float alpha = _Tf / (_Tf + dt);
    float y = alpha * _y_prev + (1.0f - alpha) * input;

    // save the variables
    _y_prev = y;
    _timestamp_prev = timestamp;
    
    return y;
}


// get the absolute value from port 1
int32_t MEASURE::get(void) {
    return get(MEASURE_VALUE_1);
}


int32_t MEASURE::get(uint8_t type) {

    int32_t val;
    int32_t v0;
    int32_t v1 = 0;

    switch (type) {
        case MEASURE_VALUE_1:
            val = (int32_t)(analogRead(_port_1) * _cal);
        break;

        case MEASURE_VALUE_2:
            val = (int32_t)(analogRead(_port_2) * _cal);
        break;

        case MEASURE_VALUE_RELATIVE:
            v0 = analogRead(_port_1);
            
            // get relative value
            if (_port_2 != -1) {
                v1 = analogRead(_port_2);
            }

            val = (int32_t)((v1 - v0) * _cal);
        break;

        // no valid type, get absolut value 1
        default:
            val = analogRead(_port_1) * _cal;
        break;
    }

    // filter value
    if (_Tf != 0) {
        val = (int32_t)filter((float)val);
    }

    return val;
}