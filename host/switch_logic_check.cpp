/*
 * Host checks for switch output functions and CAN settings.
 */

#include "module/switch/current.h"
#include "module/switch/function.h"
#include "module/switch/params.h"

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

static CAN_MESSAGE frame(uint32_t id, uint8_t data0) {
	CAN_MESSAGE message;
	memset(&message, 0, sizeof(message));
	message.id = id;
	message.uuid = 0x99;
	message.size = 1;
	message.data[0] = data0;
	return message;
}

static uint8_t light_bits(void) {
	return (uint8_t)((1 << LIGHT_LOW) | (1 << LIGHT_BACK));
}

int switch_logic_check(void) {

	expect_u8("default count", SWITCH_OUTPUT_COUNT, 6);
	expect_u8("low front message", switch_map_message(SWITCH_FUNC_LIGHT_LOW_FRONT), SWITCH_MSG_LIGHT);
	expect_u8("low front bit", switch_map_bit(SWITCH_FUNC_LIGHT_LOW_FRONT), LIGHT_LOW);
	expect_u8("low front dir", switch_map_dir(SWITCH_FUNC_LIGHT_LOW_FRONT), SWITCH_DIR_FORWARD);
	expect_u8("low back dir", switch_map_dir(SWITCH_FUNC_LIGHT_LOW_BACK), SWITCH_DIR_REVERSE);
	expect_u8("tail front bit", switch_map_bit(SWITCH_FUNC_LIGHT_BACK_FRONT), LIGHT_BACK);
	expect_u8("tail front dir", switch_map_dir(SWITCH_FUNC_LIGHT_BACK_FRONT), SWITCH_DIR_REVERSE);
	expect_u8("tail back dir", switch_map_dir(SWITCH_FUNC_LIGHT_BACK_BACK), SWITCH_DIR_FORWARD);
	expect_u8("horn low message", switch_map_message(SWITCH_FUNC_HORN_LOW), SWITCH_MSG_SIGNAL);
	expect_u8("horn low bit", switch_map_bit(SWITCH_FUNC_HORN_LOW), SIGNAL_LOW);
	expect_u8("horn low dir", switch_map_dir(SWITCH_FUNC_HORN_LOW), SWITCH_DIR_ANY);
	expect_u8("horn high bit", switch_map_bit(SWITCH_FUNC_HORN_HIGH), SIGNAL_HIGH);
	expect_true("default maps valid", switch_map_valid(SWITCH_FUNC_LIGHT_LOW_FRONT) && switch_map_valid(SWITCH_FUNC_HORN_HIGH));
	expect_true("dir 3 is rejected", !switch_map_valid((uint8_t)(3 << SWITCH_MAP_DIR_SHIFT)));

	SWITCH_PARAMS params;
	switch_params_defaults(params, 21);
	expect_u8("version", params.bytes[SWITCH_PARAM_VERSION], 21);
	expect_u8("out1", switch_params_map(params, 0), SWITCH_FUNC_LIGHT_LOW_FRONT);
	expect_u8("out2", switch_params_map(params, 1), SWITCH_FUNC_LIGHT_LOW_BACK);
	expect_u8("out3", switch_params_map(params, 2), SWITCH_FUNC_LIGHT_BACK_FRONT);
	expect_u8("out4", switch_params_map(params, 3), SWITCH_FUNC_LIGHT_BACK_BACK);
	expect_u8("out5", switch_params_map(params, 4), SWITCH_FUNC_HORN_LOW);
	expect_u8("out6", switch_params_map(params, 5), SWITCH_FUNC_HORN_HIGH);
	expect_u16("default max current", switch_params_max_current(params), SWITCH_CURRENT_MAX_DEFAULT_MA);
	expect_true("default name", params.name[0] == 'S' && params.name[5] == 'H' && params.name[6] == 0);

	expect_u16("adc zero", switch_current_from_adc(0, 1024, 30000), 0);
	expect_u16("adc full", switch_current_from_adc(1023, 1024, 30000), 30000);
	expect_true("at limit stays on", !switch_current_over(20000, 20000));
	expect_true("above limit trips", switch_current_over(20001, 20000));

	uint8_t packed[SWITCH_CURRENT_FRAME];
	switch_current_pack(10000, 20000, packed);
	expect_u8("half percent high", packed[0], 0x01);
	expect_u8("half percent low", packed[1], 0xF4);
	expect_u8("limit high", packed[2], 0x4E);
	expect_u8("limit low", packed[3], 0x20);
	expect_u16("unpack half", switch_current_unpack(packed), 10000);
	switch_current_pack(60000, 1000, packed);
	expect_u16("percent saturates", (uint16_t)(((packed[0] & 0x07) << 8) | packed[1]), SWITCH_CURRENT_PERCENT_MAX);

	SWITCH_BUS bus;
	switch_bus_clear(bus);
	expect_u8("idle outputs", switch_output_mask(&params.bytes[SWITCH_PARAM_MAP], SWITCH_OUTPUT_COUNT, bus), 0);

	switch_bus_apply(bus, frame(CAN_ID_LIGHT, light_bits()));
	expect_u8("forward lamps", switch_output_mask(&params.bytes[SWITCH_PARAM_MAP], SWITCH_OUTPUT_COUNT, bus), 0x09);

	switch_bus_apply(bus, frame(CAN_ID_DRIVE, (uint8_t)(1 << CONTROL_DIR_FLAG)));
	expect_u8("reverse lamps", switch_output_mask(&params.bytes[SWITCH_PARAM_MAP], SWITCH_OUTPUT_COUNT, bus), 0x06);
	expect_true("reverse flag", !bus.forward);

	switch_bus_apply(bus, frame(CAN_ID_DRIVE, (uint8_t)(1 << CONTROL_MAINS_FLAG)));
	expect_true("forward again", bus.forward);
	expect_u8("forward after drive", switch_output_mask(&params.bytes[SWITCH_PARAM_MAP], SWITCH_OUTPUT_COUNT, bus), 0x09);

	switch_bus_apply(bus, frame(CAN_ID_SIGNAL, (uint8_t)((1 << SIGNAL_LOW) | (1 << SIGNAL_HIGH))));
	expect_u8("lamps and horns", switch_output_mask(&params.bytes[SWITCH_PARAM_MAP], SWITCH_OUTPUT_COUNT, bus), 0x39);

	switch_bus_apply(bus, frame(CAN_ID_EMERGENCY, 0));
	expect_u8("emergency off", switch_output_mask(&params.bytes[SWITCH_PARAM_MAP], SWITCH_OUTPUT_COUNT, bus), 0);

	uint8_t mains = switch_map_make(SWITCH_MSG_DRIVE, CONTROL_MAINS_FLAG, SWITCH_DIR_ANY);
	params.bytes[SWITCH_PARAM_MAP] = mains;
	switch_bus_apply(bus, frame(CAN_ID_DRIVE, (uint8_t)(1 << CONTROL_MAINS_FLAG)));
	expect_true("mains bit", switch_output_level(mains, bus));
	expect_true("output 0 follows mains", (switch_output_mask(&params.bytes[SWITCH_PARAM_MAP], SWITCH_OUTPUT_COUNT, bus) & 0x01) != 0);

	CAN_MESSAGE request;
	memset(&request, 0, sizeof(request));
	request.id = CAN_ID_REQUEST;
	request.uuid = 1;
	request.size = 1;
	request.data[0] = 0xFF;
	SWITCH_PARAM_RESULT info = switch_params_on_can(params, request, 0x1234, 21, SWITCH_TYPE_ID);
	expect_u8("info packets", info.reply_count, 2);
	expect_u8("info max", info.reply[0].data[0], (uint8_t)(SWITCH_PARAM_BYTES - 1));
	expect_u8("info version", info.reply[0].data[3], 21);
	expect_u8("info type", info.reply[0].data[4], SWITCH_TYPE_ID);
	expect_u8("info name 0", info.reply[1].data[1], 'S');

	request.size = 4;
	request.data[0] = 0x12;
	request.data[1] = 0x34;
	request.data[2] = SWITCH_PARAM_MAP;
	request.data[3] = SWITCH_OUTPUT_COUNT;
	SWITCH_PARAM_RESULT value = switch_params_on_can(params, request, 0x1234, 21, SWITCH_TYPE_ID);
	expect_u8("value size", value.reply[0].size, SWITCH_OUTPUT_COUNT);
	expect_u8("value id", (uint8_t)(value.reply[0].id & 0x7F), SWITCH_PARAM_MAP);
	expect_u8("stored mains still there", value.reply[0].data[0], mains);

	request.id = (uint32_t)(CAN_ID_SETUP | SWITCH_PARAM_MAP);
	request.size = 3;
	request.data[0] = 0x00;
	request.data[1] = 0x01;
	request.data[2] = SWITCH_FUNC_HORN_HIGH;
	SWITCH_PARAM_RESULT rejected = switch_params_on_can(params, request, 0x1234, 21, SWITCH_TYPE_ID);
	expect_true("foreign uuid ignored", !rejected.changed);
	expect_u8("map unchanged", switch_params_map(params, 0), mains);

	request.data[0] = 0x12;
	request.data[1] = 0x34;
	request.data[2] = (uint8_t)(7 << SWITCH_MAP_DIR_SHIFT);
	SWITCH_PARAM_RESULT invalid = switch_params_on_can(params, request, 0x1234, 21, SWITCH_TYPE_ID);
	expect_true("invalid map ignored", !invalid.changed);
	expect_u8("invalid did not store", switch_params_map(params, 0), mains);

	request.data[2] = SWITCH_FUNC_HORN_HIGH;
	SWITCH_PARAM_RESULT written = switch_params_on_can(params, request, 0x1234, 21, SWITCH_TYPE_ID);
	expect_true("own uuid stored", written.changed);
	expect_u8("output 0 is horn high", switch_params_map(params, 0), SWITCH_FUNC_HORN_HIGH);
	expect_u8("version stays", params.bytes[SWITCH_PARAM_VERSION], 21);

	switch_bus_clear(bus);
	switch_bus_apply(bus, frame(CAN_ID_SIGNAL, (uint8_t)(1 << SIGNAL_HIGH)));
	expect_true("remapped horn", switch_output_level(switch_params_map(params, 0), bus));
	expect_true("low horn still separate", !switch_output_level(switch_params_map(params, 4), bus));

	request.id = CAN_ID_SETUP;
	request.size = 8;
	request.data[2] = 0;
	request.data[3] = 'L';
	request.data[4] = 'A';
	request.data[5] = 'M';
	request.data[6] = 'P';
	request.data[7] = 0;
	SWITCH_PARAM_RESULT named = switch_params_on_can(params, request, 0x1234, 21, SWITCH_TYPE_ID);
	expect_true("name stored", named.changed && params.name[0] == 'L' && params.name[3] == 'P' && params.name[4] == 0);

	request.id = (uint32_t)(CAN_ID_SETUP | SWITCH_PARAM_MAX_CURRENT);
	request.size = 4;
	request.data[0] = 0x12;
	request.data[1] = 0x34;
	request.data[2] = 0x88;
	request.data[3] = 0x13;
	SWITCH_PARAM_RESULT limit = switch_params_on_can(params, request, 0x1234, 21, SWITCH_TYPE_ID);
	expect_true("max current stored", limit.changed);
	expect_u16("max current value", switch_params_max_current(params), 5000);
	expect_u8("map survives limit write", switch_params_map(params, 0), SWITCH_FUNC_HORN_HIGH);

	if (g_fails == 0) {
		printf("switch function and settings check ok\n");
	}
	return g_fails ? 1 : 0;
}
