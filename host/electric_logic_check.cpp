/*
 * Host checks for the electric-module plugins, sensors and settings.
 */

#include "module/controller/drive_codec.h"
#include "module/electric/plugin.h"
#include "module/electric/sensors.h"
#include "module/electric/settings.h"

#include <stdio.h>
#include <string.h>

static int g_fails = 0;

static void expect_u8(const char *name, unsigned got, unsigned want) {
	if (got != want) {
		fprintf(stderr, "FAIL %s: %u, want %u\n", name, got, want);
		g_fails++;
	}
}

static void expect_u16(const char *name, unsigned got, unsigned want) {
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

static ELECTRIC_COMMAND stopped(void) {
	ELECTRIC_COMMAND command;
	memset(&command, 0, sizeof(command));
	return command;
}

static ELECTRIC_COMMAND driving(bool reverse, uint16_t drive, uint16_t brake) {
	ELECTRIC_COMMAND command = stopped();
	command.present = true;
	command.mains = true;
	command.motor = true;
	command.reverse = reverse;
	command.drive = drive;
	command.brake = brake;
	command.power = 1023;
	return command;
}

static ELECTRIC_PLUGIN_INPUT input_with(uint8_t mode, bool power_relay, uint16_t voltage_min, int32_t motor_voltage, const ELECTRIC_COMMAND &command) {
	ELECTRIC_PLUGIN_INPUT in;
	memset(&in, 0, sizeof(in));
	in.pwm_mode = mode;
	in.power_relay_fitted = power_relay;
	in.voltage_min = voltage_min;
	in.motor_voltage = motor_voltage;
	in.reversed = false;
	in.command = command;
	return in;
}

static void expect_off(const char *name, const ELECTRIC_OUTPUT &out) {
	expect_u8(name, out.drive_pwm, 0);
	if (out.brake_pwm != 0 || out.forward || out.reverse || out.power || out.direction != ELECTRIC_DIR_NONE) {
		fprintf(stderr, "FAIL %s: output not fully off\n", name);
		g_fails++;
	}
}

int electric_logic_check(void) {

	CONTROLLER_DRIVE packed;
	memset(&packed, 0, sizeof(packed));
	packed.mains = true;
	packed.motor = true;
	packed.reverse = true;
	packed.multi = true;
	packed.loco = 0xABCD;
	packed.drive = 0x155;
	packed.power = 0x200;
	packed.brake = 0x3FF;

	uint8_t frame[8];
	controller_pack_drive(packed, frame);

	ELECTRIC_COMMAND unpacked = stopped();
	expect_true("unpack drive frame", electric_unpack_drive(frame, 8, unpacked));
	expect_true("unpack mains", unpacked.mains);
	expect_true("unpack motor", unpacked.motor);
	expect_true("unpack reverse", unpacked.reverse);
	expect_true("unpack multi", unpacked.multi);
	expect_u16("unpack loco", unpacked.loco, 0xABCD);
	expect_u16("unpack drive", unpacked.drive, 0x155);
	expect_u16("unpack power", unpacked.power, 0x200);
	expect_u16("unpack brake", unpacked.brake, 0x3FF);
	expect_true("short frame rejected", !electric_unpack_drive(frame, 7, unpacked));

	ELECTRIC_PLUGIN_STATE state;
	electric_plugin_reset(state);

	ELECTRIC_OUTPUT out = electric_plugin_apply(
		ELECTRIC_PLUGIN_DIRECT,
		input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 0, stopped()),
		state
	);
	expect_off("direct idle", out);

	electric_plugin_reset(state);
	out = electric_plugin_apply(
		ELECTRIC_PLUGIN_DIRECT,
		input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 0, driving(false, 1023, 0)),
		state
	);
	expect_u8("direct forward pwm", out.drive_pwm, 255);
	expect_u8("direct forward brake", out.brake_pwm, 0);
	expect_true("direct forward relay", out.forward && !out.reverse);
	expect_true("direct power closed", out.power);
	expect_u8("direct direction", out.direction, ELECTRIC_DIR_FORWARD);
	expect_true("direct not blocked", !out.direction_blocked);

	out = electric_plugin_apply(
		ELECTRIC_PLUGIN_DIRECT,
		input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 400, driving(true, 800, 0)),
		state
	);
	expect_true("direct change blocked", out.direction_blocked);
	expect_u8("direct holds forward", out.direction, ELECTRIC_DIR_FORWARD);
	expect_true("direct holds forward relay", out.forward && !out.reverse);
	expect_u8("direct cuts drive", out.drive_pwm, 0);
	expect_true("direct opens power", !out.power);

	ELECTRIC_PLUGIN_INPUT swap = input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 19, driving(true, 1023, 200));
	out = electric_plugin_apply(ELECTRIC_PLUGIN_DIRECT, swap, state);
	expect_true("direct opens relays before swap", out.direction_blocked && out.direction == ELECTRIC_DIR_NONE && !out.forward && !out.reverse && !out.power);

	swap.gap_elapsed = true;
	out = electric_plugin_apply(ELECTRIC_PLUGIN_DIRECT, swap, state);
	expect_true("direct change allowed", !out.direction_blocked);
	expect_u8("direct now reverse", out.direction, ELECTRIC_DIR_REVERSE);
	expect_true("direct reverse relay only", out.reverse && !out.forward);
	expect_u8("direct reverse pwm", out.drive_pwm, 255);
	expect_true("direct brake pwm on", out.brake_pwm > 0);
	expect_true("direct power restored", out.power);

	electric_plugin_reset(state);
	ELECTRIC_PLUGIN_INPUT single = input_with(ELECTRIC_PWM_DRIVE, true, 20, 0, driving(false, 1023, 1023));
	out = electric_plugin_apply(ELECTRIC_PLUGIN_DIRECT, single, state);
	expect_u8("drive-only pwm", out.drive_pwm, 0);
	expect_u8("drive-only brake pin", out.brake_pwm, 0);

	single.command.brake = 0;
	out = electric_plugin_apply(ELECTRIC_PLUGIN_DIRECT, single, state);
	expect_u8("drive-only full", out.drive_pwm, 255);

	ELECTRIC_PLUGIN_INPUT limited = input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 0, driving(false, 1023, 0));
	limited.command.power = 0;
	out = electric_plugin_apply(ELECTRIC_PLUGIN_DIRECT, limited, state);
	expect_u8("power limit zeros drive", out.drive_pwm, 0);

	limited.command.power = 512;
	out = electric_plugin_apply(ELECTRIC_PLUGIN_DIRECT, limited, state);
	expect_u8("power limit half", out.drive_pwm, (uint8_t)((512UL * 255UL) / 1023UL));

	ELECTRIC_PLUGIN_INPUT bare = input_with(ELECTRIC_PWM_DRIVE_BRAKE, false, 20, 0, driving(false, 1023, 0));
	out = electric_plugin_apply(ELECTRIC_PLUGIN_DIRECT, bare, state);
	expect_true("missing power relay stays open", !out.power);
	expect_true("direction still engages", out.forward);

	ELECTRIC_PLUGIN_INPUT fault = input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 500, driving(true, 1023, 0));
	fault.command.error = true;
	out = electric_plugin_apply(ELECTRIC_PLUGIN_DIRECT, fault, state);
	expect_off("controller error stops", out);

	electric_plugin_reset(state);
	ELECTRIC_PLUGIN_INPUT reversed = input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 0, driving(false, 100, 0));
	reversed.reversed = true;
	out = electric_plugin_apply(ELECTRIC_PLUGIN_DIRECT, reversed, state);
	expect_u8("logic reverse", out.direction, ELECTRIC_DIR_REVERSE);

	out = electric_plugin_apply(
		ELECTRIC_PLUGIN_DIRECT,
		input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 800, stopped()),
		state
	);
	expect_off("mains off releases at speed", out);

	electric_plugin_reset(state);
	out = electric_plugin_apply(
		ELECTRIC_PLUGIN_DIRECT,
		input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 100, driving(false, 1023, 0)),
		state
	);
	expect_true("first engage waits for low voltage", out.direction_blocked && out.direction == ELECTRIC_DIR_NONE && !out.power);

	electric_plugin_reset(state);
	out = electric_plugin_apply(
		ELECTRIC_PLUGIN_4QD,
		input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 0, driving(false, 1023, 0)),
		state
	);
	expect_true("4qd enable", out.forward && !out.reverse && out.power);
	expect_u8("4qd throttle", out.drive_pwm, 255);

	ELECTRIC_PLUGIN_INPUT qd_reverse = input_with(ELECTRIC_PWM_DRIVE, true, 20, 0, driving(true, 1023, 100));
	out = electric_plugin_apply(ELECTRIC_PLUGIN_4QD, qd_reverse, state);
	expect_true("4qd drops direction before swap", out.direction_blocked && !out.forward && !out.reverse);

	qd_reverse.gap_elapsed = true;
	out = electric_plugin_apply(ELECTRIC_PLUGIN_4QD, qd_reverse, state);
	expect_true("4qd reverse switch", out.forward && out.reverse);
	expect_u8("4qd brake drops throttle", out.drive_pwm, 0);
	expect_u8("4qd drive-only brake pin", out.brake_pwm, 0);

	out = electric_plugin_apply(
		ELECTRIC_PLUGIN_4QD,
		input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 300, driving(false, 1023, 0)),
		state
	);
	expect_true("4qd change inhibited", out.direction_blocked && !out.forward && !out.power);
	expect_true("4qd holds reverse switch", out.reverse);

	electric_plugin_reset(state);
	ELECTRIC_PLUGIN_INPUT curtis_in = input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 0, driving(false, 900, 0));
	out = electric_plugin_apply(ELECTRIC_PLUGIN_CURTIS, curtis_in, state);
	expect_true("curtis hpd holds throttle", out.drive_pwm == 0 && out.forward && out.power);
	expect_true("curtis direction selected", !out.direction_blocked);

	curtis_in.command.drive = 0;
	curtis_in.command.power = 1023;
	out = electric_plugin_apply(ELECTRIC_PLUGIN_CURTIS, curtis_in, state);
	expect_u8("curtis hpd clears", out.drive_pwm, 0);
	expect_true("curtis still enabled", out.forward && !state.hpd_locked);

	curtis_in.command.drive = 1023;
	out = electric_plugin_apply(ELECTRIC_PLUGIN_CURTIS, curtis_in, state);
	expect_u8("curtis throttle after hpd", out.drive_pwm, 255);
	expect_true("curtis relays exclusive", out.forward && !out.reverse);

	ELECTRIC_PLUGIN_INPUT curtis_reverse = input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 0, driving(true, 700, 0));
	out = electric_plugin_apply(ELECTRIC_PLUGIN_CURTIS, curtis_reverse, state);
	expect_true("curtis opens direction before swap", out.direction_blocked && state.hpd_locked && out.drive_pwm == 0 && !out.forward && !out.reverse);

	curtis_reverse.gap_elapsed = true;
	out = electric_plugin_apply(ELECTRIC_PLUGIN_CURTIS, curtis_reverse, state);
	expect_true("curtis direction rearms hpd", state.hpd_locked && out.drive_pwm == 0 && out.reverse && !out.forward);

	electric_plugin_reset(state);
	out = electric_plugin_apply(
		99,
		input_with(ELECTRIC_PWM_DRIVE_BRAKE, true, 20, 0, driving(false, 1023, 0)),
		state
	);
	expect_off("unknown plugin", out);

	ELECTRIC_SETTINGS settings;
	electric_settings_defaults(settings, 21);
	expect_u8("default plugin", electric_settings_plugin(settings), ELECTRIC_PLUGIN_DIRECT);
	expect_u8("default batteries", electric_settings_battery_count(settings), 1);
	expect_u8("default pwm", electric_settings_pwm_mode(settings), ELECTRIC_PWM_DRIVE_BRAKE);
	expect_u16("default voltage min", electric_settings_voltage_min(settings), ELECTRIC_VOLTAGE_MIN_DEFAULT);
	expect_true("default name", settings.name[0] == 'M' && settings.name[4] == 'R' && settings.name[5] == 0);

	CAN_MESSAGE request;
	memset(&request, 0, sizeof(request));
	request.id = CAN_ID_REQUEST;
	request.uuid = 1;
	request.size = 1;
	request.data[0] = 0xFF;
	ELECTRIC_SETTINGS_RESULT info = electric_settings_on_can(settings, request, 0x1234, 21, ELECTRIC_TYPE_ID);
	expect_u8("info packets", info.reply_count, 2);
	expect_u8("info max", info.reply[0].data[0], (uint8_t)(ELECTRIC_PARAM_BYTES - 1));
	expect_u8("info version", info.reply[0].data[3], 21);
	expect_u8("info type", info.reply[0].data[4], ELECTRIC_TYPE_ID);

	request.id = (uint32_t)(CAN_ID_SETUP | ELECTRIC_PARAM_PLUGIN);
	request.size = 5;
	request.data[0] = 0x12;
	request.data[1] = 0x34;
	request.data[2] = ELECTRIC_PLUGIN_CURTIS;
	request.data[3] = 9;
	request.data[4] = 5;
	ELECTRIC_SETTINGS_RESULT written = electric_settings_on_can(settings, request, 0x1234, 21, ELECTRIC_TYPE_ID);
	expect_true("settings stored", written.changed);
	expect_u8("plugin curtis", electric_settings_plugin(settings), ELECTRIC_PLUGIN_CURTIS);
	expect_u8("battery count clamped", electric_settings_battery_count(settings), ELECTRIC_BATT_MAX);
	expect_u8("invalid pwm becomes drive only", electric_settings_pwm_mode(settings), ELECTRIC_PWM_DRIVE);

	request.data[0] = 0x00;
	request.data[1] = 0x01;
	request.data[2] = ELECTRIC_PLUGIN_4QD;
	ELECTRIC_SETTINGS_RESULT foreign = electric_settings_on_can(settings, request, 0x1234, 21, ELECTRIC_TYPE_ID);
	expect_true("foreign uuid ignored", !foreign.changed);
	expect_u8("plugin unchanged", electric_settings_plugin(settings), ELECTRIC_PLUGIN_CURTIS);

	request.data[0] = 0x12;
	request.data[1] = 0x34;
	request.data[2] = 9;
	request.size = 3;
	written = electric_settings_on_can(settings, request, 0x1234, 21, ELECTRIC_TYPE_ID);
	expect_true("invalid plugin write accepted as no change of id", written.changed);
	expect_u8("invalid plugin kept", settings.bytes[ELECTRIC_PARAM_PLUGIN], ELECTRIC_PLUGIN_CURTIS);

	request.id = CAN_ID_SETUP;
	request.size = 4;
	request.data[2] = 0;
	request.data[3] = 'L';
	ELECTRIC_SETTINGS_RESULT named = electric_settings_on_can(settings, request, 0x1234, 21, ELECTRIC_TYPE_ID);
	expect_true("name stored", named.changed && settings.name[0] == 'L' && settings.name[1] == 0);
	expect_u8("version stays", settings.bytes[ELECTRIC_PARAM_VERSION], 21);

	request.id = CAN_ID_REQUEST;
	request.size = 4;
	request.data[0] = 0x12;
	request.data[1] = 0x34;
	request.data[2] = ELECTRIC_PARAM_PWM_MODE;
	request.data[3] = 1;
	ELECTRIC_SETTINGS_RESULT value = electric_settings_on_can(settings, request, 0x1234, 21, ELECTRIC_TYPE_ID);
	expect_u8("pwm readback", value.reply[0].data[0], ELECTRIC_PWM_DRIVE);
	expect_u8("pwm readback id", (uint8_t)(value.reply[0].id & 0x7F), ELECTRIC_PARAM_PWM_MODE);

	uint16_t batteries[2] = { 400, 410 };
	ELECTRIC_SENSOR_FRAME sensors[ELECTRIC_SENSOR_FRAME_MAX];
	uint8_t n = electric_sensor_frames(-20, batteries, 2, 1023, sensors, ELECTRIC_SENSOR_FRAME_MAX);
	expect_u8("sensor frame count", n, 4);
	expect_u16("motor id", sensors[0].id, CAN_ID_MOTOR_VOLTAGE);
	expect_u16("motor low", sensors[0].data[0], (uint16_t)(uint16_t)-20 & 0xFF);
	expect_u16("motor high", sensors[0].data[1], ((uint16_t)(uint16_t)-20 >> 8) & 0xFF);
	expect_u16("battery 0 id", sensors[1].id, CAN_ID_BATT_VOLTAGE);
	expect_u16("battery 1 id", sensors[2].id, CAN_ID_BATT_1_VOLTAGE);
	expect_u16("main id", sensors[3].id, CAN_ID_VOLTAGE);
	expect_u16("main sum", (uint16_t)(sensors[3].data[0] | (sensors[3].data[1] << 8)), 810);
	expect_u16("reference", (uint16_t)(sensors[0].data[2] | (sensors[0].data[3] << 8)), 1023);

	n = electric_sensor_frames(0, batteries, 0, 1023, sensors, ELECTRIC_SENSOR_FRAME_MAX);
	expect_u8("no batteries", n, 1);

	ELECTRIC_OUTPUT moving;
	memset(&moving, 0, sizeof(moving));
	moving.direction = ELECTRIC_DIR_REVERSE;
	moving.reverse = true;
	moving.drive_pwm = 255;
	moving.power = true;

	ELECTRIC_STATUS_BITS bits = electric_vehicle_status(
		driving(true, 1023, 0),
		moving,
		0,
		20,
		true,
		false,
		false
	);
	uint8_t status = electric_status_byte(bits);
	expect_true("status ready", (status & (uint8_t)(1 << READY_FLAG)) != 0);
	expect_true("status moving", (status & (uint8_t)(1 << MOVING_FLAG)) != 0);
	expect_true("status reverse logic", (status & (uint8_t)(1 << REVERSE)) != 0);
	expect_true("status direction", (status & (uint8_t)(1 << DIR_FLAG)) != 0);

	moving.direction_blocked = true;
	bits = electric_vehicle_status(driving(true, 1023, 0), moving, 400, 20, false, false, false);
	expect_true("blocked is not ready", !bits.ready && bits.moving);

	bits = electric_vehicle_status(driving(false, 0, 0), out, 0, 20, false, true, false);
	expect_true("link fault is error", bits.error && !bits.ready && !bits.mains);

	bits = electric_vehicle_status(driving(false, 0, 0), out, 0, 20, false, false, true);
	expect_true("setup idle report", bits.mains && bits.drive && bits.ready && !bits.moving);

	bits = electric_vehicle_status(driving(false, 0, 0), out, 0, 20, false, true, true);
	expect_true("fault overrides setup idle", bits.error && !bits.ready);

	if (g_fails == 0) {
		printf("electric plugin and settings check ok\n");
	}
	return g_fails ? 1 : 0;
}
