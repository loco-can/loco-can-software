#pragma once


#include <Arduino.h>


struct CAN_MESSAGE {
    uint32_t id;
    uint16_t uuid;
    uint8_t data[8];
    uint8_t size;
};
