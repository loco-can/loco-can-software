/*
 * Switch parameter image and CAN setup access.
 */

#include "params.h"

#include <string.h>

#include "../../can_protocol.h"


static void switch_params_copy_name(SWITCH_PARAMS &params, const char *name) {

	uint8_t i = 0;

	while (name[i] != 0 && i < SWITCH_PARAM_NAME_LEN) {
		params.name[i] = name[i];
		i++;
	}

	params.name[i] = 0;

	while (i < SWITCH_PARAM_NAME_LEN) {
		i++;
		params.name[i] = 0;
	}
}


void switch_params_defaults(SWITCH_PARAMS &params, uint32_t module_version) {

	for (uint8_t i = 0; i < SWITCH_PARAM_BYTES; i++) {
		params.bytes[i] = 0;
	}

	params.bytes[SWITCH_PARAM_VERSION] = (uint8_t)LOCO_VERSION_CODE(module_version);

	for (uint8_t i = 0; i < SWITCH_OUTPUT_COUNT; i++) {
		params.bytes[SWITCH_PARAM_MAP + i] = SWITCH_DEFAULT_FUNCTION[i];
	}

	switch_params_set_max_current(params, SWITCH_CURRENT_MAX_DEFAULT_MA);
	switch_params_copy_name(params, "SWITCH");
}


uint8_t switch_params_map(const SWITCH_PARAMS &params, uint8_t output) {

	if (output >= SWITCH_OUTPUT_COUNT) {
		return SWITCH_FUNC_NONE;
	}

	return params.bytes[SWITCH_PARAM_MAP + output];
}


uint16_t switch_params_max_current(const SWITCH_PARAMS &params) {

	return (uint16_t)params.bytes[SWITCH_PARAM_MAX_CURRENT]
		| ((uint16_t)params.bytes[SWITCH_PARAM_MAX_CURRENT + 1] << 8);
}


void switch_params_set_max_current(SWITCH_PARAMS &params, uint16_t milliamp) {

	params.bytes[SWITCH_PARAM_MAX_CURRENT] = (uint8_t)(milliamp & 0xFF);
	params.bytes[SWITCH_PARAM_MAX_CURRENT + 1] = (uint8_t)((milliamp >> 8) & 0xFF);
}


static void switch_params_clear(CAN_MESSAGE &message) {

	message.id = 0;
	message.uuid = 0;
	message.size = 0;

	for (uint8_t i = 0; i < 8; i++) {
		message.data[i] = 0;
	}
}


static bool switch_params_uuid_match(const CAN_MESSAGE &message, uint16_t self_uuid) {

	if (message.size < 2) {
		return false;
	}

	uint16_t uuid = ((uint16_t)message.data[0] << 8) | message.data[1];
	return uuid == self_uuid;
}


static void switch_params_info(SWITCH_PARAM_RESULT &result, const SWITCH_PARAMS &params, uint32_t module_version, uint8_t module_type) {

	uint8_t nlen = 0;
	while (nlen < SWITCH_PARAM_NAME_LEN && params.name[nlen] != 0) {
		nlen++;
	}

	uint8_t name_packets = nlen == 0 ? 1 : (uint8_t)((nlen + CAN_NAME_REPLY_SIZE - 1) / CAN_NAME_REPLY_SIZE);
	if (name_packets > CAN_SETUP_REPLY_MAX - 1) {
		name_packets = CAN_SETUP_REPLY_MAX - 1;
	}
	uint8_t total = (uint8_t)(1 + name_packets);

	CAN_MESSAGE &info = result.reply[0];
	switch_params_clear(info);
	info.id = CAN_ID_REPLY;
	info.size = 7;
	info.data[0] = (uint8_t)(SWITCH_PARAM_BYTES - 1);
	info.data[1] = (uint8_t)(total << 4);
	info.data[2] = 0;
	info.data[3] = (uint8_t)LOCO_VERSION_MAJOR(module_version);
	info.data[4] = (uint8_t)LOCO_VERSION_MINOR(module_version);
	info.data[5] = (uint8_t)LOCO_VERSION_PATCH(module_version);
	info.data[6] = module_type;

	for (uint8_t p = 0; p < name_packets; p++) {
		CAN_MESSAGE &name = result.reply[1 + p];
		switch_params_clear(name);
		name.id = CAN_ID_REPLY;
		name.data[0] = (uint8_t)((total << 4) | (p + 1));

		uint8_t n = 0;
		uint8_t from = (uint8_t)(p * CAN_NAME_REPLY_SIZE);
		while (n < CAN_NAME_REPLY_SIZE && (uint8_t)(from + n) < nlen) {
			name.data[1 + n] = (uint8_t)params.name[from + n];
			n++;
		}
		name.size = (uint8_t)(1 + n);
	}

	result.reply_count = total;
}


static void switch_params_value_reply(SWITCH_PARAM_RESULT &result, const SWITCH_PARAMS &params, uint8_t index, uint8_t count) {

	if (index >= SWITCH_PARAM_BYTES) {
		return;
	}

	if (count == 0) {
		count = 8;
	}
	if (count > 8) {
		count = 8;
	}
	if ((uint16_t)index + count > SWITCH_PARAM_BYTES) {
		count = (uint8_t)(SWITCH_PARAM_BYTES - index);
	}

	CAN_MESSAGE &reply = result.reply[0];
	switch_params_clear(reply);
	reply.id = (uint32_t)(CAN_ID_REPLY | (index & 0x7F));
	reply.size = count;

	for (uint8_t i = 0; i < count; i++) {
		reply.data[i] = params.bytes[index + i];
	}

	result.reply_count = 1;
}


SWITCH_PARAM_RESULT switch_params_on_can(
	SWITCH_PARAMS &params,
	const CAN_MESSAGE &message,
	uint16_t self_uuid,
	uint32_t module_version,
	uint8_t module_type
) {

	SWITCH_PARAM_RESULT result;
	result.reply_count = 0;
	result.changed = false;
	for (uint8_t i = 0; i < CAN_SETUP_REPLY_MAX; i++) {
		switch_params_clear(result.reply[i]);
	}

	if (message.id == CAN_ID_REQUEST) {
		bool global = (message.size == 0) || (message.size >= 1 && message.data[0] == 0xFF);
		if (global) {
			switch_params_info(result, params, module_version, module_type);
			return result;
		}

		if (!switch_params_uuid_match(message, self_uuid)) {
			return result;
		}

		if (message.size == 2) {
			switch_params_info(result, params, module_version, module_type);
			return result;
		}

		uint8_t index = message.data[2];
		uint8_t count = 8;
		if (message.size >= 4 && message.data[3] > 0) {
			count = message.data[3];
		}
		switch_params_value_reply(result, params, index, count);
		return result;
	}

	if ((message.id & 0x780) != CAN_ID_SETUP) {
		return result;
	}

	if (!switch_params_uuid_match(message, self_uuid) || message.size < 2) {
		return result;
	}

	uint8_t index = (uint8_t)(message.id & 0x7F);

	if (index == 0) {
		uint8_t from = 2;
		uint8_t n = 0;
		if (message.size >= 4) {
			from = 3;
			n = (uint8_t)(message.data[2] * CAN_NAME_MAX_SIZE);
			if (n >= SWITCH_PARAM_NAME_LEN) {
				return result;
			}
		}

		if (n == 0) {
			memset(params.name, 0, SWITCH_PARAM_NAME_LEN + 1);
		}

		while (from < message.size && n < SWITCH_PARAM_NAME_LEN) {
			params.name[n] = (char)message.data[from];
			n++;
			from++;
		}
		params.name[n] = 0;
		result.changed = true;
		return result;
	}

	if (index >= SWITCH_PARAM_BYTES || message.size < 3) {
		return result;
	}

	uint8_t length = (uint8_t)(message.size - 2);
	if (length > CAN_VALUE_MAX_SIZE) {
		length = CAN_VALUE_MAX_SIZE;
	}
	if ((uint16_t)index + length > SWITCH_PARAM_BYTES) {
		length = (uint8_t)(SWITCH_PARAM_BYTES - index);
	}

	for (uint8_t i = 0; i < length; i++) {
		uint8_t at = (uint8_t)(index + i);
		if (at == SWITCH_PARAM_VERSION && message.data[2 + i] != (uint8_t)LOCO_VERSION_CODE(module_version)) {
			return result;
		}
		if (at >= SWITCH_PARAM_MAP && at < SWITCH_PARAM_MAX_CURRENT && !switch_map_valid(message.data[2 + i])) {
			return result;
		}
	}

	for (uint8_t i = 0; i < length; i++) {
		params.bytes[index + i] = message.data[2 + i];
	}

	params.bytes[SWITCH_PARAM_VERSION] = (uint8_t)LOCO_VERSION_CODE(module_version);
	result.changed = true;
	return result;
}
