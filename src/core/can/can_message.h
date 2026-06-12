/*
 * Loco-CAN can message structure
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 */
#ifndef CAN_MESSAGE_H
#define CAN_MESSAGE_H

#include <inttypes.h>


struct CAN_MESSAGE {
    uint32_t id;
    uint16_t uuid;
    uint8_t data[8];
    uint8_t size;
};

#endif