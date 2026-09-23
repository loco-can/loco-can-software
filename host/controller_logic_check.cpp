/*
 * Host checks for the controller status machine and drive-frame layout.
 */

#include "module/controller/drive_codec.h"
#include "module/controller/status.h"

#include <stdio.h>

static int g_fails = 0;

static void expect_status(const char *name, uint8_t got, uint8_t want) {
	if (got != want) {
		fprintf(stderr, "FAIL %s: status %u, want %u\n", name, got, want);
		g_fails++;
	}
}

static void expect_u8(const char *name, uint8_t got, uint8_t want) {
	if (got != want) {
		fprintf(stderr, "FAIL %s: %u, want %u\n", name, got, want);
		g_fails++;
	}
}

static void expect_u16(const char *name, uint16_t got, uint16_t want) {
	if (got != want) {
		fprintf(stderr, "FAIL %s: %u, want %u\n", name, got, want);
		g_fails++;
	}
}

static CONTROLLER_INPUTS idle(void) {
	CONTROLLER_INPUTS in = {};
	in.mains = CONTROLLER_MAINS_OFF;
	in.dir = CONTROLLER_DIR_NEUTRAL;
	in.drive_zero = true;
	in.bus_free = true;
	in.foreign = false;
	in.bus_moving = false;
	in.system_error = false;
	in.horn = false;
	in.horn2 = false;
	return in;
}

static uint8_t step(uint8_t status, CONTROLLER_INPUTS in, bool &nulled) {
	return controller_next_status(status, in, nulled);
}

int controller_logic_check(void) {

	bool nulled = false;
	CONTROLLER_INPUTS in = idle();

	expect_status("off stays off", step(CONTROLLER_STATUS_OFF, in, nulled), CONTROLLER_STATUS_OFF);

	in.mains = CONTROLLER_MAINS_ON;
	in.bus_free = false;
	in.foreign = false;
	expect_status("listen before first standby", step(CONTROLLER_STATUS_OFF, in, nulled), CONTROLLER_STATUS_OFF);
	in.foreign = true;
	expect_status("foreign heartbeat locks", step(CONTROLLER_STATUS_OFF, in, nulled), CONTROLLER_STATUS_LOCKED);

	in.bus_free = true;
	in.horn = true;
	in.bus_moving = true;
	expect_status("setup blocked while moving", step(CONTROLLER_STATUS_OFF, in, nulled), CONTROLLER_STATUS_STANDBY);

	in.bus_moving = false;
	expect_status("horn enters setup", step(CONTROLLER_STATUS_OFF, in, nulled), CONTROLLER_STATUS_SETUP);

	in = idle();
	in.mains = CONTROLLER_MAINS_ON;
	expect_status("mains on to standby", step(CONTROLLER_STATUS_OFF, in, nulled), CONTROLLER_STATUS_STANDBY);

	in.mains = CONTROLLER_MAINS_OFF;
	expect_status("standby mains off", step(CONTROLLER_STATUS_STANDBY, in, nulled), CONTROLLER_STATUS_OFF);

	in = idle();
	in.mains = CONTROLLER_MAINS_ON;
	in.system_error = true;
	expect_status("standby system error", step(CONTROLLER_STATUS_STANDBY, in, nulled), CONTROLLER_STATUS_OFF);

	in = idle();
	in.mains = CONTROLLER_MAINS_MOTOR;
	in.drive_zero = false;
	nulled = false;
	expect_status("motor before zero is nulled", step(CONTROLLER_STATUS_STANDBY, in, nulled), CONTROLLER_STATUS_NULLED);

	in.drive_zero = true;
	expect_status("zero then motor is on", step(CONTROLLER_STATUS_NULLED, in, nulled), CONTROLLER_STATUS_ON);
	if (!nulled) {
		fprintf(stderr, "FAIL nulled flag was not set\n");
		g_fails++;
	}

	in.dir = CONTROLLER_DIR_FORWARD;
	expect_status("direction selects ready", step(CONTROLLER_STATUS_ON, in, nulled), CONTROLLER_STATUS_READY);

	in.bus_moving = true;
	expect_status("bus moving", step(CONTROLLER_STATUS_READY, in, nulled), CONTROLLER_STATUS_MOVING);

	in.dir = CONTROLLER_DIR_NEUTRAL;
	expect_status("keep moving until bus stops", step(CONTROLLER_STATUS_MOVING, in, nulled), CONTROLLER_STATUS_MOVING);

	in.bus_moving = false;
	expect_status("stopped and neutral is on", step(CONTROLLER_STATUS_MOVING, in, nulled), CONTROLLER_STATUS_ON);

	in.dir = CONTROLLER_DIR_REVERSE;
	in.bus_moving = false;
	uint8_t ready = step(CONTROLLER_STATUS_ON, in, nulled);
	expect_status("reverse is ready", ready, CONTROLLER_STATUS_READY);
	in.mains = CONTROLLER_MAINS_ON;
	expect_status("motor off from ready", step(ready, in, nulled), CONTROLLER_STATUS_STANDBY);

	in = idle();
	in.mains = CONTROLLER_MAINS_MOTOR;
	in.dir = CONTROLLER_DIR_FORWARD;
	in.system_error = true;
	expect_status("ready fault to off", step(CONTROLLER_STATUS_READY, in, nulled), CONTROLLER_STATUS_OFF);

	in = idle();
	in.mains = CONTROLLER_MAINS_ON;
	in.bus_free = false;
	expect_status("locked waits", step(CONTROLLER_STATUS_LOCKED, in, nulled), CONTROLLER_STATUS_LOCKED);
	in.bus_free = true;
	expect_status("locked releases", step(CONTROLLER_STATUS_LOCKED, in, nulled), CONTROLLER_STATUS_STANDBY);

	in = idle();
	in.mains = CONTROLLER_MAINS_ON;
	in.horn2 = true;
	expect_status("second horn enters setup", step(CONTROLLER_STATUS_OFF, in, nulled), CONTROLLER_STATUS_SETUP);
	in.mains = CONTROLLER_MAINS_OFF;
	expect_status("setup ends with mains", step(CONTROLLER_STATUS_SETUP, in, nulled), CONTROLLER_STATUS_OFF);

	if (controller_is_commanding(CONTROLLER_STATUS_OFF) || controller_is_commanding(CONTROLLER_STATUS_LOCKED)) {
		fprintf(stderr, "FAIL off/locked must not command the bus\n");
		g_fails++;
	}
	if (!controller_is_commanding(CONTROLLER_STATUS_STANDBY) || !controller_is_commanding(CONTROLLER_STATUS_SETUP)) {
		fprintf(stderr, "FAIL standby/setup must command the bus\n");
		g_fails++;
	}
	if (!controller_motor_on(CONTROLLER_STATUS_ON) || controller_motor_on(CONTROLLER_STATUS_STANDBY)) {
		fprintf(stderr, "FAIL motor flag\n");
		g_fails++;
	}
	if (!controller_transmits_speed(CONTROLLER_STATUS_MOVING) || controller_transmits_speed(CONTROLLER_STATUS_ON)) {
		fprintf(stderr, "FAIL speed transmit flag\n");
		g_fails++;
	}

	CONTROLLER_DRIVE drive = {};
	drive.mains = true;
	drive.motor = true;
	drive.reverse = true;
	drive.up = true;
	drive.multi = true;
	drive.loco = 0xABCD;
	drive.drive = 0x155;
	drive.power = 0x200;
	drive.brake = 0x3FF;

	uint8_t frame[8];
	controller_pack_drive(drive, frame);
	expect_u8("flags", frame[0], (uint8_t)((1 << CONTROL_MAINS_FLAG) | (1 << CONTROL_DRIVE_FLAG) | (1 << CONTROL_DIR_FLAG) | (1 << CONTROL_UP_FLAG)));
	expect_u8("drive high", frame[2], 0xD5);
	expect_u8("drive low", frame[3], 0x55);
	expect_u8("power high", frame[4], 0x3E);
	expect_u8("power low", frame[5], 0x00);
	expect_u8("brake high", frame[6], 0x37);
	expect_u8("brake low", frame[7], 0xFF);

	expect_u16("scale zero", controller_scale_10bit(0, 1024), 0);
	expect_u16("scale full", controller_scale_10bit(1023, 1024), 1023);
	expect_u16("scale esp full", controller_scale_10bit(2047, 2048), 1023);

	uint16_t drive_val = 1;
	uint16_t brake_val = 1;
	controller_split_single_pot(512, drive_val, brake_val);
	expect_u16("center drive", drive_val, 0);
	expect_u16("center brake", brake_val, 0);
	controller_split_single_pot(1023, drive_val, brake_val);
	expect_u16("full drive", drive_val, 1021);
	expect_u16("full drive brake", brake_val, 0);
	controller_split_single_pot(0, drive_val, brake_val);
	expect_u16("full brake drive", drive_val, 0);
	expect_u16("full brake", brake_val, 1023);

	if (g_fails == 0) {
		printf("controller status and drive frame check ok\n");
	}
	return g_fails ? 1 : 0;
}
