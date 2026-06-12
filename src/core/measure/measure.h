/*
 * measure class header file
 * 
 * @author: Thomas H Winkler
 * @copyright: 2020-2025
 * @lizence: GG0
 */

#ifndef MEASURE_H
#define MEASURE_H


#include <Arduino.h>

// get types
#define MEASURE_VALUE_1 1
#define MEASURE_VALUE_2 2
#define MEASURE_VALUE_RELATIVE 3


class MEASURE {

  public:
    MEASURE(); // create voltage class

    void begin(uint8_t); // absolute voltage to gnd
    void begin(uint8_t, uint16_t, uint16_t); // absolute voltage to gnd with resolution and calibration

    void begin(uint8_t, int8_t); // relative voltage between two ports
    void begin(uint8_t, int8_t, uint16_t, uint16_t); // relative voltage between two ports with resolution and calibration

    void setResolution(uint8_t); // set the ADC resolution in bits
    void calibrate(float); // calibrate

    void set_filter(float);
    float filter(float);

    int32_t get(void); // get the value from port 1
    int32_t get(uint8_t); // get the value from port by type

  private:
    uint8_t _port_1;
    int8_t _port_2;

    uint16_t _max_val; // maximal DAC value
    float _cal; // calibration value

    float _Tf;
    float _y_prev;
    uint32_t _timestamp_prev;

};

#endif