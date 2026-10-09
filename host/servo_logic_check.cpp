/*
 * Host checks for servo analog mapping and CAN settings.
 */

#include "module/servo/mapping.h"
#include "module/servo/params.h"

#include <stdio.h>
#include <string.h>

static int g_fails = 0;

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

static void expect_true(const char *name, bool got) {
	if (!got) {
		fprintf(stderr, "FAIL %s\n", name);
		g_fails++;
	}
}

static CAN_MESSAGE analog_frame(uint32_t id, uint16_t value, uint16_t full_scale) {
	CAN_MESSAGE message;
	memset(&message, 0, sizeof(message));
	message.id = id;
	message.uuid = 0x99;
	message.size = 4;
	message.data[0] = (uint8_t)(value & 0xFF);
	message.data[1] = (uint8_t)((value >> 8) & 0xFF);
	message.data[2] = (uint8_t)(full_scale & 0xFF);
	message.data[3] = (uint8_t)((full_scale >> 8) & 0xFF);
	return message;
}

int servo_logic_check(void) {

	expect_u8("count", SERVO_OUTPUT_COUNT, 4);
	expect_u8("drive src", servo_map_source(SERVO_FUNC_DRIVE), SERVO_SRC_DRIVE);
	expect_true("defaults valid", servo_map_valid(SERVO_FUNC_DRIVE) && servo_map_valid(SERVO_FUNC_NONE));
	expect_true("src 13 rejected", !servo_map_valid(13));

	expect_u16("scale half", servo_analog_scale(50, 100), 511);
	expect_u16("scale zero ref", servo_analog_scale(800, 0), 800);
	expect_u16("scale clamp", servo_analog_scale(2000, 0), SERVO_ANALOG_MAX);

	SERVO_BUS bus;
	servo_bus_clear(bus);
	expect_true("cleared dead", !bus.alive);
	expect_u16("none", servo_output_value(SERVO_FUNC_NONE, bus), 0);

	CAN_MESSAGE drive;
	memset(&drive, 0, sizeof(drive));
	drive.id = CAN_ID_DRIVE;
	drive.uuid = 0x11;
	drive.size = 8;
	drive.data[0] = (uint8_t)(1 << CONTROL_DIR_FLAG);
	drive.data[2] = 0x02;
	drive.data[3] = 0x00;
	drive.data[4] = 0x01;
	drive.data[5] = 0x00;
	drive.data[6] = 0x03;
	drive.data[7] = 0xFF;
	servo_bus_apply(bus, drive);
	expect_true("drive alive", bus.alive);
	expect_u16("throttle", bus.drive, 512);
	expect_u16("power", bus.power, 256);
	expect_u16("brake", bus.brake, 1023);
	expect_u16("reverse dir", bus.dir, SERVO_ANALOG_MAX);
	expect_u16("mapped drive", servo_output_value(SERVO_FUNC_DRIVE, bus), 512);
	expect_u16("mapped brake", servo_output_value(SERVO_FUNC_BRAKE, bus), 1023);
	expect_u16("invert drive", servo_output_value(servo_map_make(SERVO_SRC_DRIVE, true), bus), 511);

	servo_bus_apply(bus, analog_frame(CAN_ID_MOTOR_VOLTAGE, 120, 240));
	expect_u16("motor voltage half", bus.motor_voltage, 511);
	expect_u16("mapped motor V", servo_output_value(servo_map_make(SERVO_SRC_MOTOR_VOLTAGE, false), bus), 511);

	CAN_MESSAGE stop;
	memset(&stop, 0, sizeof(stop));
	stop.id = CAN_ID_EMERGENCY;
	servo_bus_apply(bus, stop);
	expect_true("estop dead", !bus.alive);
	expect_u16("estop drive", bus.drive, 0);

	SERVO_PARAMS params;
	servo_params_defaults(params, 21);
	expect_u8("version", params.bytes[SERVO_PARAM_VERSION], 21);
	expect_u8("out1", servo_params_map(params, 0), SERVO_FUNC_DRIVE);
	expect_u8("out2", servo_params_map(params, 1), SERVO_FUNC_BRAKE);
	expect_u8("out3", servo_params_map(params, 2), SERVO_FUNC_POWER);
	expect_u8("out4", servo_params_map(params, 3), SERVO_FUNC_NONE);
	expect_true("default name", params.name[0] == 'S' && params.name[4] == 'O' && params.name[5] == 0);

	CAN_MESSAGE request;
	memset(&request, 0, sizeof(request));
	request.id = CAN_ID_REQUEST;
	request.size = 1;
	request.data[0] = 0xFF;
	SERVO_PARAM_RESULT info = servo_params_on_can(params, request, 0x1234, 21, SERVO_TYPE_ID);
	expect_u8("info packets", info.reply_count, 2);
	expect_u8("info max", info.reply[0].data[0], (uint8_t)(SERVO_PARAM_BYTES - 1));
	expect_u8("info version", info.reply[0].data[3], 21);
	expect_u8("info type", info.reply[0].data[4], SERVO_TYPE_ID);

	request.id = (uint32_t)(CAN_ID_SETUP | SERVO_PARAM_MAP);
	request.size = 6;
	request.data[0] = 0x12;
	request.data[1] = 0x34;
	request.data[2] = servo_map_make(SERVO_SRC_VOLTAGE, false);
	request.data[3] = servo_map_make(SERVO_SRC_SPEED, true);
	request.data[4] = SERVO_FUNC_NONE;
	request.data[5] = servo_map_make(SERVO_SRC_MOTOR_CURRENT, false);
	SERVO_PARAM_RESULT written = servo_params_on_can(params, request, 0x1234, 21, SERVO_TYPE_ID);
	expect_true("maps stored", written.changed);
	expect_u8("map0 voltage", servo_map_source(servo_params_map(params, 0)), SERVO_SRC_VOLTAGE);
	expect_true("map1 invert", servo_map_invert(servo_params_map(params, 1)));
	expect_u8("map2 none", servo_map_source(servo_params_map(params, 2)), SERVO_SRC_NONE);
	expect_u8("map3 motor I", servo_map_source(servo_params_map(params, 3)), SERVO_SRC_MOTOR_CURRENT);

	request.data[2] = 13;
	SERVO_PARAM_RESULT rejected = servo_params_on_can(params, request, 0x1234, 21, SERVO_TYPE_ID);
	expect_true("invalid map rejected", !rejected.changed);

	if (g_fails != 0) {
		fprintf(stderr, "servo logic: %d failed\n", g_fails);
		return 1;
	}

	printf("servo logic check ok\n");
	return 0;
}
