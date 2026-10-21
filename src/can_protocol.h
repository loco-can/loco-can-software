// =====================================
//
// CAN-communication
//
// IDs and flags stay aligned with the loco-can/LocoCAN Arduino library
// (https://github.com/loco-can/LocoCAN) so host tools and the programmer
// Platform UI share one protocol.
//
// =====================================
#pragma once

#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H


#define CAN_BUS_SPEED 500E3
#define CAN_ALIVE_TIMEOUT 500

// MASK: bit=0 > bit of packetId is ignored
// MASK: bit=1 > bit is compared
#define CAN_ID_MASK 0x770

// emergency message
#define CAN_ID_EMERGENCY 0x000

// high priority values
#define CAN_ID_MODULE_CURRENT 0x100
#define CAN_ID_LIGHT_CURRENT CAN_ID_MODULE_CURRENT
#define CAN_ID_MOTOR_CURRENT 0x110
#define CAN_ID_BATT_CURRENT 0x120

// mid priority values 
#define CAN_ID_SPEED 0x200
#define CAN_ID_DIR 0x210
#define CAN_ID_SIGNAL 0x220
#define CAN_ID_TACHO 0x230

// low priority values
#define CAN_ID_VOLTAGE 0x300 // main supply voltage
#define CAN_ID_MOTOR_VOLTAGE 0x310
#define CAN_ID_BATT_VOLTAGE 0x320
#define CAN_ID_BATT_1_VOLTAGE 0x321
#define CAN_ID_BATT_2_VOLTAGE 0x322

// command values
#define CAN_ID_DRIVE 0x400
#define CAN_ID_LIGHT 0x410
#define CAN_ID_SWITCH 0x420

// vehicle status values
#define CAN_ID_VEHICLE_STATUS 0x500

// Module version is major.minor.patch, for example 2.1.12.
// LOCO_VERSION(2, 1, 12) packs that into one integer. Hardware
// configuration follows major.minor, so 2.1.0 and 2.1.12 share a board.
// The settings byte keeps the older major*10+minor code (2.1 -> 21).
#define LOCO_VERSION(major, minor, patch) \
	((((major) * 1UL) << 16) | (((minor) * 1UL) << 8) | ((patch) * 1UL))
#define LOCO_VERSION_MAJOR(v) ((((v) * 1UL) >> 16) & 0xFFUL)
#define LOCO_VERSION_MINOR(v) ((((v) * 1UL) >> 8) & 0xFFUL)
#define LOCO_VERSION_PATCH(v) (((v) * 1UL) & 0xFFUL)
#define LOCO_VERSION_IS(version, major, minor) \
	(LOCO_VERSION_MAJOR(version) == (major) && LOCO_VERSION_MINOR(version) == (minor))
#define LOCO_VERSION_CODE(v) ((LOCO_VERSION_MAJOR(v) * 10UL) + LOCO_VERSION_MINOR(v))

/*
 * 8-bit module type (doc/SETTINGS.md):
 *   bit 7     1 = wireless
 *   bits 6-4  group
 *   bits 3-0  type inside the group
 *
 *   Controller                          0x1n
 *   Actuator (electric, servo, switch)  0x2n
 *   Sensor                              0x3n
 */
#define LOCO_TYPE_WIRELESS         0x80
#define LOCO_TYPE_GROUP_CONTROLLER 0x10
#define LOCO_TYPE_GROUP_ACTUATOR   0x20
#define LOCO_TYPE_GROUP_SENSOR     0x30
#define LOCO_TYPE_GROUP_PROGRAMMER 0x70

#define LOCO_TYPE(group, n) ((group) | ((n) & 0x0F))

#define TYPE_CONTROLLER LOCO_TYPE(LOCO_TYPE_GROUP_CONTROLLER, 0)
#define TYPE_ELECTRIC   LOCO_TYPE(LOCO_TYPE_GROUP_ACTUATOR, 0)
#define TYPE_SERVO      LOCO_TYPE(LOCO_TYPE_GROUP_ACTUATOR, 1)
#define TYPE_SWITCH     LOCO_TYPE(LOCO_TYPE_GROUP_ACTUATOR, 2)
#define TYPE_DRIVE      LOCO_TYPE(LOCO_TYPE_GROUP_ACTUATOR, 3)
#define TYPE_SENSOR     LOCO_TYPE(LOCO_TYPE_GROUP_SENSOR, 0)
#define TYPE_PROGRAMMER LOCO_TYPE(LOCO_TYPE_GROUP_PROGRAMMER, 0)
#define TYPE_WIFI       (LOCO_TYPE_WIRELESS | 0x01)

#define CONTROLLER_TYPE_ID TYPE_CONTROLLER
#define ELECTRIC_TYPE_ID   TYPE_ELECTRIC
#define SERVO_TYPE_ID      TYPE_SERVO
#define SWITCH_TYPE_ID     TYPE_SWITCH
#define SENSOR_TYPE_ID     TYPE_SENSOR
#define PROGRAMMER_TYPE_ID TYPE_PROGRAMMER
#define WIFI_MODULE_TYPE   TYPE_WIFI

// message heartbeat
// Every module sends this every MODULE_HEARTBEAT_TIMEOUT ms.
//   data[0]  module type (*_TYPE_ID)
//   data[1]  major
//   data[2]  minor
//   data[3]  patch (2.1.12 -> 2, 1, 12)
#define CAN_ID_MODULE_HEARTBEAT 0x510

#define MODULE_HEARTBEAT_TIMEOUT 1000

// heardbeat values
// The heartbeat is sent from the active controller and used
// to monitor the connection between controller and motor modules.
// The train end latern signals are registered in the controller
// when starting to drive. A change in the latern signal list while
// driving leads to a emergency stop (switch off the heartbeat signal)
#define CAN_ID_DRIVE_HEARTBEAT 0x010
#define CAN_ID_DRIVE_HEARTBEAT_TIME 50 // ms between heartbeat packages
#define CAN_ID_DRIVE_TIME 100 // ms between drive packages
#define CAN_ID_TRAINEND_HEARTBEAT 0x020 // sent from train end laterns


// =====================================
// 
// controller message CAN_ID_DRIVE
// 
// =====================================

/*
 * The controller message are sent to all vehicles, when it is activated (mains = on).
 *
 * If a multi traktion is recognised by the controller when activated, it 15 of byte 2
 * is set to 1 and the uuid of the main locomotive is sent in bytes 2, 4 and 6.
 * All other locomotives are running in slave mode with no lights or pantographs.
 */

/*
 * 8 byte package sent from the controller
 * the buttons for horn-h and horn-l are used for up and down signals
 *
 * byte 0:   7      6      5      4      3      2      1      0
 *         error  aux-1   aux    down   up     dir   motor  mains
 * byte 1:   7      6      5      4      3      2      1      0
 *
 * mains: train main switch
 * motor: pantograph up, start combustion motor
 * dir: direction 0=forward
 * aux: special function
 * aux-1: special function 1
 * error: controller error   
 *
 * DRIVE VALUE: 10-bit value of drive voltage
 * byte 2: bits 10-14 = multi traktion main UUID bits 11-15
 *         bit 16 = 1: multi traktion
 *         drive bit 8-9
 * byte 3: drive bit 0-7
 *
 * POWER VALUE: 10-bit value of drive max power
 * byte 4: bits 10-15 = multi traktion main UUID bits 6-10
 *         power bit 8-9
 * byte 5: power bit 0-7
 *
 * BREAK VALUE: 10-bit value of break intensity
 * byte 6: bits 10-15 = multi traktion main UUID bits 0-5
 *         break bit 8-9
 * byte 7: break bit 0-7
 */

// =====================================
// CONTROLLER
#define CONTROL_ERROR_FLAG 7

#define CONTROL_AUX_1_FLAG 6
#define CONTROL_AUX_FLAG 5

#define CONTROL_DOWN_FLAG 4
#define CONTROL_UP_FLAG 3

#define CONTROL_DIR_FLAG 2
#define CONTROL_DRIVE_FLAG 1
#define CONTROL_MAINS_FLAG 0


// =====================================
//
// SIGNAL DATA
//
// =====================================

/*
 * 1 byte package
 *
 * SIGNAL STATUS
 * byte 0:   7      6      5      4      3      2      1      0
 *                                             bell   low   high
 *
 * high:    hight frequence signal
 * low:     low frequence signal
 * bell:    signal bell
 */

// SIGNAL
#define SIGNAL_HIGH 0
#define SIGNAL_LOW 1
#define SIGNAL_BELL 2 // not yet impemented


// =====================================
// 
// vehicle messages
// 
// =====================================

/*
 * The vehicle message are sent from each vehicle when a mains on is received.
 */

/*
 * 3 byte package sent from the vehicle
 *
 * VEHICLE STATUS
 * byte 0:   7      6      5      4      3      2      1      0
 *         error  ready  moving         revs   dir   drive  mains
 * byte 1: LSB of UUID
 * byte 2: MSB of UUID
 *
 * error:   motor drive reports an error
 * ready:   ready to drive
 * moving:  loco is moving
 *
 * reverse: if true, reverse dir signal
 * dir:     drive direction (0=forward, 1=reverse)
 * drive:   drive is activated
 * mains:   main switch state
 */

// =====================================
// VEHICLE

#define ERROR_FLAG 7
#define READY_FLAG 6
#define MOVING_FLAG 5

#define MULTI_FLAG 4

#define REVERSE 3
#define DIR_FLAG 2
#define DRIVE_FLAG 1
#define MAINS_FLAG 0


// =====================================
//
// LIGHT DATA
//
// =====================================

/*
 * 1 byte package
 *
 * LIGHT_STATUS
 * byte 0:   7      6      5      4      3      2      1      0
 *          main  train   cab   instr   back    high   low   posit
 *
 * main:    main light switch
 * train:   train lights
 * cab:     cabine light
 * instr:   instrument light
 * back:    back light
 * high:    bright spot light
 * low:     low spot light
 * posit:   position light
 */

// LIGHT
#define LIGHT_MAIN 7
#define LIGHT_TRAIN 6
#define LIGHT_CAB 5
#define LIGHT_INSTR 4
#define LIGHT_BACK 3
#define LIGHT_HIGH 2
#define LIGHT_LOW 1
#define LIGHT_POSIT 0


// =====================================
// 
// loco setup
// 
// =====================================

/*
 * 3 byte package
 *
 * LOCO_SETUP
 * byte 0:   7      6      5      4      3      2      1      0
 *                                                  disable  status
 *
 * status:      loco setup status
                0 = select: the master loco is selected
                1 = direction: reverse the direction in the loco
 * disable:     0: loco is active
                1: loco is deactivated
                    sends mains = on, drive = on, ready = true, moving = false
 * byte 1:  LSB of the locomotive UUID to setup
 * byte 2:  MSB of the locomotive UUID to setup
 */

#define CAN_ID_LOCO_SETUP 0x0600
#define CAN_LOCO_SETUP_MASK 0x06FF


// =====================================
// 
// setup
// 
// =====================================

// the setup command is used to send and receive module setup data
//		0x7FF		request setup info from module (0 byte message length)
//					the identifier uuid is ignored and the info is sent any way

//		0x7nn		returns info packages (nn = data id)
//					+ 8 bytes text description
//					adding the own uuid in the extended identifier
//					so no other module can interpret it as a write command

// write commands are sent with the target uuid in the first two bytes
//		0x600		set module name

// 		0x6nn		set data
//					nn => data id + max 8 bytes of data

// message  bytes 0, 1 	=> UUID of module
//			byte 2		=> message id
//				0xFF = module name
//			bytes 3-7 	=> data

#define CAN_REQUEST_MASK 0x7FF
#define CAN_ID_REQUEST 0x7FF

#define CAN_REPLY_MASK 0x7FF
#define CAN_ID_REPLY 0x780

#define CAN_SETUP_MASK 0x700
#define CAN_ID_SETUP 0x700

/*
 * Module setup mode (programmer session)
 *
 * Started only when no CAN_ID_DRIVE_HEARTBEAT has been seen recently.
 * The programmer broadcasts enter/leave; every module ACKs.
 * While active, modules send nothing on their own and accept only
 * identifiers in 0x700-0x7FF.
 *
 *   0x7FF  [CAN_SETUP_MODE_ENTER]   programmer → all
 *   0x7FF  [CAN_SETUP_MODE_LEAVE]   programmer → all
 *   0x7F8  [op] [type] [status] [maj] [min] [patch]
 *          status 1 = ok, 0 = refused (controller heartbeat)
 */
#define CAN_ID_SETUP_MODE 0x7F8
#define CAN_SETUP_MODE_ENTER 0xFE
#define CAN_SETUP_MODE_LEAVE 0xFD
#define CAN_SETUP_MODE_OK 1
#define CAN_SETUP_MODE_BUSY 0
#define CAN_SETUP_MODE_HOLD_MS 250
#define CAN_SETUP_MODE_TIMEOUT_MS 30000
#define CAN_SETUP_MODE_KEEPALIVE_MS 5000

#define CAN_NAME_MAX_SIZE 5
#define CAN_NAME_REPLY_SIZE 7
#define CAN_SETUP_REPLY_MAX 4
#define CAN_VALUE_MAX_SIZE 6

#endif