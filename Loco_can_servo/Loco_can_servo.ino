/*
 * LOCO-CAN Sensor Module 
 * @author: Thomas H Winkler
 * @copyright: 2021
 * @lizence: GG0
 */

/*
 * The Servo-Module has four outputs to directly drive
 * model servo motors.
 *
 * Four voltage inputs can be configured to measure absolute
 * values to GND or relative values between two inputs.
 *
 * The pulse input can be used to get speed or rotation values.
 */

#include "config.h"
#include "module.h"


MODULE module;

CAN_COM can_com;


void setup(void) {

    #ifdef DEBUG
        Serial.begin(115200);
        Serial.println("Start Loco-Servo");
    #endif

    can_com.begin(500E3, CAN_STATUS_LED);
    can_com.register_filter(CAN_ID_MASK, CAN_ID_VOLTAGE);
    can_com.register_filter(CAN_ID_MASK, CAN_ID_DRIVE_STATUS);

    module.register_message(CAN_ID_MASK, CAN_ID_VOLTAGE, 0, 2);
    module.register_message(CAN_ID_MASK, CAN_ID_DRIVE_STATUS, 0, 1);

    module.begin();
}


void loop(void) {

    module.update();
}