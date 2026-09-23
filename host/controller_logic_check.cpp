/*
 * Host checks for the controller status machine and drive-frame layout.
 */

#include "module/controller/drive_codec.h"
#include "module/controller/gauge.h"
#include "module/controller/params.h"
#include "module/controller/status.h"

#include <string.h>

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

	CONTROLLER_PARAMS params;
	controller_params_defaults(params, 1024, false, 21);
	expect_u8("mains count", params.bytes[CONTROLLER_PARAM_MAINS_COUNT], 3);
	expect_u16("mains low", controller_params_get16(params, CONTROLLER_PARAM_MAINS_POINT), 0);
	expect_u16("mains mid", controller_params_get16(params, (uint8_t)(CONTROLLER_PARAM_MAINS_POINT + 2)), 511);
	expect_u16("mains high", controller_params_get16(params, (uint8_t)(CONTROLLER_PARAM_MAINS_POINT + 4)), 1023);
	expect_u16("single drive zero", controller_params_get16(params, CONTROLLER_PARAM_DRIVE_ZERO), 511);
	expect_u16("single brake full", controller_params_get16(params, CONTROLLER_PARAM_BRAKE_FULL), 0);

	controller_params_defaults(params, 1024, true, 21);
	expect_u16("dual drive zero", controller_params_get16(params, CONTROLLER_PARAM_DRIVE_ZERO), 0);
	expect_u16("dual brake full", controller_params_get16(params, CONTROLLER_PARAM_BRAKE_FULL), 1023);

	expect_u16("axis zero", controller_map_axis(0, 0, 1023), 0);
	expect_u16("axis full", controller_map_axis(1023, 0, 1023), 1023);
	expect_u16("axis mid", controller_map_axis(512, 0, 1023), 512);

	uint16_t drive_cal = 9;
	uint16_t brake_cal = 9;
	controller_split_calibrated(0, 0, 511, 1023, drive_cal, brake_cal);
	expect_u16("cal brake", brake_cal, 1023);
	expect_u16("cal brake drive", drive_cal, 0);
	controller_split_calibrated(1023, 0, 511, 1023, drive_cal, brake_cal);
	expect_u16("cal drive", drive_cal, 1023);
	expect_u16("cal drive brake", brake_cal, 0);

	CAN_MESSAGE request;
	request.id = CAN_ID_REQUEST;
	request.uuid = 1;
	request.size = 1;
	request.data[0] = 0xFF;
	for (uint8_t i = 1; i < 8; i++) request.data[i] = 0;
	CONTROLLER_PARAM_RESULT info = controller_params_on_can(params, request, 0x1234, 21, CONTROLLER_TYPE_ID);
	expect_u8("info packets", info.reply_count, 2);
	expect_u8("info max", info.reply[0].data[0], (uint8_t)(CONTROLLER_PARAM_BYTES - 1));
	expect_u8("info version", info.reply[0].data[3], 21);
	expect_u8("info type", info.reply[0].data[4], CONTROLLER_TYPE_ID);
	expect_u8("info name id", (uint8_t)info.reply[1].id, (uint8_t)CAN_ID_REPLY);

	request.size = 4;
	request.data[0] = 0x12;
	request.data[1] = 0x34;
	request.data[2] = CONTROLLER_PARAM_DRIVE_FULL;
	request.data[3] = 2;
	CONTROLLER_PARAM_RESULT value = controller_params_on_can(params, request, 0x1234, 21, CONTROLLER_TYPE_ID);
	expect_u8("value size", value.reply[0].size, 2);
	expect_u8("value id low", (uint8_t)(value.reply[0].id & 0x7F), CONTROLLER_PARAM_DRIVE_FULL);
	expect_u16("value drive full", (uint16_t)(value.reply[0].data[0] | (value.reply[0].data[1] << 8)), 1023);

	request.id = (uint32_t)(CAN_ID_SETUP | CONTROLLER_PARAM_DRIVE_ZERO);
	request.size = 4;
	request.data[0] = 0x00;
	request.data[1] = 0x01;
	request.data[2] = 0x10;
	request.data[3] = 0x00;
	CONTROLLER_PARAM_RESULT rejected = controller_params_on_can(params, request, 0x1234, 21, CONTROLLER_TYPE_ID);
	if (rejected.changed) {
		fprintf(stderr, "FAIL foreign uuid must not change parameters\n");
		g_fails++;
	}

	request.data[0] = 0x12;
	request.data[1] = 0x34;
	request.data[2] = 0x00;
	request.data[3] = 0x02;
	CONTROLLER_PARAM_RESULT written = controller_params_on_can(params, request, 0x1234, 21, CONTROLLER_TYPE_ID);
	if (!written.changed) {
		fprintf(stderr, "FAIL own uuid write was ignored\n");
		g_fails++;
	}
	expect_u16("stored drive zero", controller_params_get16(params, CONTROLLER_PARAM_DRIVE_ZERO), 0x0200);

	request.id = CAN_ID_SETUP;
	request.size = 7;
	request.data[2] = 0;
	request.data[3] = 'L';
	request.data[4] = 'O';
	request.data[5] = 'C';
	request.data[6] = 'O';
	CONTROLLER_PARAM_RESULT named = controller_params_on_can(params, request, 0x1234, 21, CONTROLLER_TYPE_ID);
	if (!named.changed || params.name[0] != 'L' || params.name[3] != 'O' || params.name[4] != 0) {
		fprintf(stderr, "FAIL module name was not stored\n");
		g_fails++;
	}
	expect_u8("version stays", params.bytes[CONTROLLER_PARAM_VERSION], 21);

	expect_u8("gauge mode", params.bytes[CONTROLLER_PARAM_GAUGE_BATT_MODE], CONTROLLER_GAUGE_MODE_SERVO);
	expect_u16("gauge batt ref", controller_params_get16(params, CONTROLLER_PARAM_GAUGE_BATT_REF), CONTROLLER_GAUGE_REF_VOLTAGE);
	expect_u16("gauge current ref", controller_params_get16(params, CONTROLLER_PARAM_GAUGE_CURRENT_REF), CONTROLLER_GAUGE_REF_CURRENT);
	expect_u8("gauge bytes", CONTROLLER_PARAM_BYTES, 57);

	uint8_t package[4];
	package[0] = (uint8_t)((3 << 5) | (1000 >> 8));
	package[1] = (uint8_t)(1000 & 0xFF);
	package[2] = (uint8_t)(12000 >> 8);
	package[3] = (uint8_t)(12000 & 0xFF);
	uint16_t percentage = 0;
	uint16_t reference = 0;
	uint8_t index = 0;
	if (!controller_gauge_decode(package, 4, percentage, reference, index)) {
		fprintf(stderr, "FAIL gauge package was rejected\n");
		g_fails++;
	}
	expect_u8("gauge index", index, 3);
	expect_u16("gauge percent", percentage, 1000);
	expect_u16("gauge reference", reference, 12000);
	expect_u8("batt channel", controller_gauge_channel(CAN_ID_BATT_VOLTAGE), CONTROLLER_GAUGE_BATT_VOLTAGE);
	expect_u8("motor channel", controller_gauge_channel(CAN_ID_MOTOR_VOLTAGE), CONTROLLER_GAUGE_MOTOR_VOLTAGE);
	expect_u8("current channel", controller_gauge_channel(CAN_ID_CURRENT), CONTROLLER_GAUGE_CURRENT);
	expect_u8("reduce batt", controller_gauge_reduce(CONTROLLER_GAUGE_BATT_VOLTAGE), CONTROLLER_GAUGE_REDUCE_MIN);
	expect_u8("reduce current", controller_gauge_reduce(CONTROLLER_GAUGE_CURRENT), CONTROLLER_GAUGE_REDUCE_MAX);

	expect_u16("12V meter at 9.6V", controller_gauge_output(800, 12000, 12000, CONTROLLER_GAUGE_ANALOG_FULL), 204);
	expect_u16("12V meter saturates", controller_gauge_output(700, 24000, 12000, CONTROLLER_GAUGE_ANALOG_FULL), CONTROLLER_GAUGE_ANALOG_FULL);
	expect_u16("servo at 70 percent", controller_gauge_output(700, 24000, 24000, CONTROLLER_GAUGE_SERVO_FULL), 126);
	expect_u16("gauge reference zero", controller_gauge_output(800, 12000, 0, CONTROLLER_GAUGE_ANALOG_FULL), 0);

	CONTROLLER_GAUGE_SAMPLE slots[CONTROLLER_MAX_VEHICLES];
	memset(slots, 0, sizeof(slots));
	controller_gauge_note(slots, CONTROLLER_MAX_VEHICLES, 0x1111, 800, 12000, 1000, 2000);
	controller_gauge_note(slots, CONTROLLER_MAX_VEHICLES, 0x2222, 700, 24000, 1000, 2000);
	uint16_t picked_pct = 0;
	uint16_t picked_ref = 0;
	if (!controller_gauge_select(slots, CONTROLLER_MAX_VEHICLES, CONTROLLER_GAUGE_REDUCE_MIN, 1500, 2000, picked_pct, picked_ref)) {
		fprintf(stderr, "FAIL gauge select found nothing\n");
		g_fails++;
	}
	expect_u16("lowest charge", picked_pct, 700);
	expect_u16("lowest charge ref", picked_ref, 24000);

	memset(slots, 0, sizeof(slots));
	controller_gauge_note(slots, CONTROLLER_MAX_VEHICLES, 0x1111, 400, 12000, 1000, 2000);
	controller_gauge_note(slots, CONTROLLER_MAX_VEHICLES, 0x2222, 900, 12000, 1200, 2000);
	if (!controller_gauge_select(slots, CONTROLLER_MAX_VEHICLES, CONTROLLER_GAUGE_REDUCE_MAX, 1500, 2000, picked_pct, picked_ref)) {
		fprintf(stderr, "FAIL current select found nothing\n");
		g_fails++;
	}
	expect_u16("highest current", picked_pct, 900);

	if (controller_gauge_select(slots, CONTROLLER_MAX_VEHICLES, CONTROLLER_GAUGE_REDUCE_MAX, 4000, 2000, picked_pct, picked_ref)) {
		fprintf(stderr, "FAIL expired gauge sample was kept\n");
		g_fails++;
	}

	if (g_fails == 0) {
		printf("controller status and drive frame check ok\n");
	}
	return g_fails ? 1 : 0;
}
