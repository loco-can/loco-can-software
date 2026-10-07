/*
 * Switch module current.
 *
 * The analog input is scaled linearly to milliamps. Full scale of the
 * ADC is SWITCH_CURRENT_FULL_SCALE_MA. The EEPROM max_current value is
 * the shutdown limit in the same unit.
 *
 * The published frame follows the 4-byte measure layout in
 * intelliValue.h. The reference is max_current. The percentage is in
 * 0.1 percent (1000 = 100 percent) and saturates at 11 bits.
 */
#pragma once

#ifndef SWITCH_CURRENT_H
#define SWITCH_CURRENT_H

#include <stdint.h>

#include "../../can_protocol.h"

#ifndef SWITCH_CURRENT_FULL_SCALE_MA
#define SWITCH_CURRENT_FULL_SCALE_MA 30000
#endif

#define SWITCH_CURRENT_MAX_DEFAULT_MA 20000
#define SWITCH_CURRENT_PERIOD_MS 100
#define SWITCH_OVERCURRENT_HOLD_MS 1000

#define SWITCH_CURRENT_FRAME 4
#define SWITCH_CURRENT_PERCENT_FULL 1000
#define SWITCH_CURRENT_PERCENT_MAX 2047

/* CAN id used for this module's load current */
#define SWITCH_CURRENT_ID CAN_ID_LIGHT_CURRENT

uint16_t switch_current_from_adc(uint16_t raw, uint16_t resolution, uint16_t full_scale_ma);

/* true when the reading is above the EEPROM limit */
bool switch_current_over(uint16_t milliamp, uint16_t max_milliamp);

void switch_current_pack(uint16_t milliamp, uint16_t max_milliamp, uint8_t out[SWITCH_CURRENT_FRAME]);

/* measured milliamps reconstructed from a packed frame */
uint16_t switch_current_unpack(const uint8_t data[SWITCH_CURRENT_FRAME]);

#endif
