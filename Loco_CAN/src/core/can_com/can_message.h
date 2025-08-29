#ifndef CAN_MESSAGE_H
#define CAN_MESSAGE_H

#include <Arduino.h>


struct CAN_MESSAGE {
    uint32_t id;
    uint16_t uuid;
    uint8_t data[8];
    uint8_t size;
};

#endif