/*
 * Loco-CAN module board configuration
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 */

/*
 * This file contains all hardware and software settings for the different
 * versions of the hardware. To compile a certain software version for a
 * specific module, set the corresponding board definition string in the
 * config.h file.
 *
 * Each module section consists of the hardware IO definitions and the
 * a section for the software settings.
 *
 * The selected board version is available via BOARD_VERSION.
 */

#ifndef BOARDS_H
#define BOARDS_H

#include "config.h"


/* ******************************************
 * universal board settings
 */
#if BOARD_VERSION == UNIVERSAL_BOARD_V_2_0
	#include "module_universal.h"
	#define BOARD_PLATFORM ATMEGA

#endif

#if BOARD_VERSION == UNIVERSAL_BOARD_V_2_1
	#include "module_universal.h"
	#define BOARD_PLATFORM ATMEGA

#endif


/* ******************************************
 * controller board settings
 */
#if BOARD_VERSION == CONTROLLER_BOARD_V_2_0
	#include "module_controller.h"
	#define BOARD_PLATFORM ATMEGA

#endif

#if BOARD_VERSION == CONTROLLER_BOARD_V_2_1
	#include "module_controller.h"
	#define BOARD_PLATFORM ATMEGA

#endif

#if BOARD_VERSION == CONTROLLER_BOARD_V_3_0
	#include "module_controller.h"
	#define BOARD_PLATFORM ATMEGA

#endif


/* ******************************************
 * switch board settings
 */
#if BOARD_VERSION == SWITCH_BOARD_V_2_0
	#include "module_switch.h"
	#define BOARD_PLATFORM ATMEGA

#endif

#if BOARD_VERSION == SWITCH_BOARD_V_2_1
	#include "module_switch.h"
	#define BOARD_PLATFORM ATMEGA

#endif


/* ******************************************
 * sensor board settings
 */
#if BOARD_VERSION == SENSOR_BOARD_V_2_0
	#include "module_sensor.h"
	#define BOARD_PLATFORM ATMEGA

#endif


/* ******************************************
 * servo board settings
 */
#if BOARD_VERSION == SERVO_BOARD_V_2_0
	#include "module_servo.h"
	#define BOARD_PLATFORM ATMEGA

#endif


/* ******************************************
 * motor board settings
 */
#if BOARD_VERSION == MOTOR_BOARD_V_2_0
	#include "module_motor.h"
	#define BOARD_PLATFORM ATMEGA

#endif


/* ******************************************
 * LED board settings
 */
#if BOARD_VERSION == LED_BOARD_V_3_0
	#include "module_led.h"
	#define BOARD_PLATFORM ATMEGA

#endif


/*
 * Atmega standard port for CAN controller
 */
#if BOARD_PLATFORM == ATMEGA

	// CAN ports (CS, INT)
	#define CAN_RX 10
	#define CAN_TX 2
#endif


/* ******************************************
 * drive board settings
 ****************************************** */
#if BOARD_VERSION == DRIVE_BOARD_V_3_0

	#include "module_drive.h"
	#define BOARD_PLATFORM ESP32

	/* ******************************************
	 * hardware settings
	 ****************************************** */

	// CAN ports
	#define CAN_RX 17
	#define CAN_TX 18

	// LOCAL STATUS
	#define LED_CAN 46
	#define LED_STATUS_RED 41
	#define LED_STATUS_GREEN 42

	// 4Q DRIVER
	#define PWM 15
	#define FORW 21
	#define REV 2
	#define BREAK 3

	// MEASUREMENT
	#define BATTERY_1 9
	#define BATTERY_2 10

	#define MOTOR_VOLTAGE_PLUS 4
	#define MOTOR_VOLTAGE_MINUS 5

	// SENSORS
	#define TEMP_SDA 7
	#define TEMP_SCK 8
	#define IN_PULSE 45 // without pullup (pin must bo GND on boot)

	// LOCAL CONTROLS
	#define CONTROLLER_CONTROL 11
	#define CONTROLLER_MAINS 12
	#define CONTROLLER_DIR 13
	#define CONTROLLER_HORN 14

	// LOCAL OUTPUT
	#define OUT_LIGHT 38
	#define OUT_HORN 39

	// LOCAL MEASUREMENT
	#define VOLTMETER 40

	/* ******************************************
	 * software settings
	 ****************************************** */

	/*
	 * controller function settings
	 */
	// ANALOG SWITCH SETTINGS
	#define CONTROLLER_ANALOG_RESOLUTION 4095
	#define CONTROLLER_CONTROLER_RESOLUTION 2048

	#define CONTROLLER_MAINS_OFF_VAL 0
	#define CONTROLLER_MAINS_ON_VAL 2048
	#define CONTROLLER_MAINS_ON_ON_VAL 4095

	#define CONTROLLER_DIR_OFF 4095
	#define CONTROLLER_DIR_FORW 2048
	#define CONTROLLER_DIR_REV 0

	#define CONTROLLER_HORN_OFF 4095
	#define CONTROLLER_HORN_LOW 2048
	#define CONTROLLER_HORN_HIGH 0

	#define CONTROLLER_CONTROLER_MID_LOW 1690 // border to driving
	#define CONTROLLER_CONTROLER_MID_HIGH 3100 // border to breaking

	#define LEDC_TIMER              LEDC_TIMER_0
	#define LEDC_MODE               LEDC_LOW_SPEED_MODE
	#define LEDC_CHANNEL            LEDC_CHANNEL_0
	#define LEDC_DUTY_RES           LEDC_TIMER_10_BIT // Set duty resolution to 13 bits
	#define LEDC_FREQUENCY          (16000) // Frequency in Hertz. Set frequency at 4 kHz

#endif

#if BOARD_VERSION == DRIVE_BOARD_V_3_1

	#include "module_drive.h"
	#define BOARD_PLATFORM ESP32S3

	/* ******************************************
	 * hardware settings
	 ****************************************** */

	// CAN ports
	#define CAN_RX 17
	#define CAN_TX 18

	// LOCAL STATUS
	#define LED_CAN 46
	#define LED_STATUS_RED 41
	#define LED_STATUS_GREEN 42

	// 4Q DRIVER
	#define PWM 15
	#define FORW 21
	#define REV 2
	#define BREAK 3
	#define FAN 45

	// MEASUREMENT
	#define BATTERY_1 9
	#define BATTERY_2 10

	#define MOTOR_VOLTAGE_PLUS 4
	#define MOTOR_VOLTAGE_MINUS 5

	// SENSORS
	#define TEMP_SDA 7
	#define TEMP_SCK 8
	#define IN_PULSE 47

	// LOCAL CONTROLS
	#define CONTROLLER_CONTROL 11
	#define CONTROLLER_MAINS 12
	#define CONTROLLER_DIR 13
	#define CONTROLLER_HORN 14

	// LOCAL OUTPUT
	#define OUT_LIGHT 38
	#define OUT_HORN 39

	// LOCAL MEASUREMENT
	#define VOLTMETER 40

	/* ******************************************
	 * software settings
	 ****************************************** */


#endif


#endif