/*
 * Servo parameter image and CAN setup access.
 */

#include "params.h"

#include "../../can_protocol.h"


static void servo_params_copy_name(SERVO_PARAMS &params, const char *name) {

	uint8_t i = 0;

	while (name[i] != 0 && i < SERVO_PARAM_NAME_LEN) {
		params.name[i] = name[i];
		i++;
	}

	params.name[i] = 0;

	while (i < SERVO_PARAM_NAME_LEN) {
		i++;
		params.name[i] = 0;
	}
}


void servo_params_defaults(SERVO_PARAMS &params, uint8_t module_version) {

	for (uint8_t i = 0; i < SERVO_PARAM_BYTES; i++) {
		params.bytes[i] = 0;
	}

	params.bytes[SERVO_PARAM_VERSION] = module_version;

	for (uint8_t i = 0; i < SERVO_OUTPUT_COUNT; i++) {
		params.bytes[SERVO_PARAM_MAP + i] = SERVO_DEFAULT_MAP[i];
	}

	servo_params_copy_name(params, "SERVO");
}


uint8_t servo_params_map(const SERVO_PARAMS &params, uint8_t output) {

	if (output >= SERVO_OUTPUT_COUNT) {
		return SERVO_FUNC_NONE;
	}

	return params.bytes[SERVO_PARAM_MAP + output];
}


static void servo_params_clear(CAN_MESSAGE &message) {

	message.id = 0;
	message.uuid = 0;
	message.size = 0;

	for (uint8_t i = 0; i < 8; i++) {
		message.data[i] = 0;
	}
}


static bool servo_params_uuid_match(const CAN_MESSAGE &message, uint16_t self_uuid) {

	if (message.size < 2) {
		return false;
	}

	uint16_t uuid = ((uint16_t)message.data[0] << 8) | message.data[1];
	return uuid == self_uuid;
}


static void servo_params_info(SERVO_PARAM_RESULT &result, const SERVO_PARAMS &params, uint8_t module_version, uint8_t module_type) {

	CAN_MESSAGE &info = result.reply[0];
	servo_params_clear(info);
	info.id = CAN_ID_REPLY;
	info.size = 5;
	info.data[0] = (uint8_t)(SERVO_PARAM_BYTES - 1);
	info.data[1] = 0x20;
	info.data[2] = 0;
	info.data[3] = module_version;
	info.data[4] = module_type;

	CAN_MESSAGE &name = result.reply[1];
	servo_params_clear(name);
	name.id = CAN_ID_REPLY;
	name.data[0] = 0x21;

	uint8_t n = 0;
	while (n < 7 && params.name[n] != 0) {
		name.data[1 + n] = (uint8_t)params.name[n];
		n++;
	}

	name.size = (uint8_t)(1 + n);
	result.reply_count = 2;
}


static void servo_params_value_reply(SERVO_PARAM_RESULT &result, const SERVO_PARAMS &params, uint8_t index, uint8_t count) {

	if (index >= SERVO_PARAM_BYTES) {
		return;
	}

	if (count == 0) {
		count = 8;
	}
	if (count > 8) {
		count = 8;
	}
	if ((uint16_t)index + count > SERVO_PARAM_BYTES) {
		count = (uint8_t)(SERVO_PARAM_BYTES - index);
	}

	CAN_MESSAGE &reply = result.reply[0];
	servo_params_clear(reply);
	reply.id = (uint32_t)(CAN_ID_REPLY | (index & 0x7F));
	reply.size = count;

	for (uint8_t i = 0; i < count; i++) {
		reply.data[i] = params.bytes[index + i];
	}

	result.reply_count = 1;
}


SERVO_PARAM_RESULT servo_params_on_can(
	SERVO_PARAMS &params,
	const CAN_MESSAGE &message,
	uint16_t self_uuid,
	uint8_t module_version,
	uint8_t module_type
) {

	SERVO_PARAM_RESULT result;
	result.reply_count = 0;
	result.changed = false;
	servo_params_clear(result.reply[0]);
	servo_params_clear(result.reply[1]);

	if (message.id == CAN_ID_REQUEST) {
		bool global = (message.size == 0) || (message.size >= 1 && message.data[0] == 0xFF);
		if (global) {
			servo_params_info(result, params, module_version, module_type);
			return result;
		}

		if (!servo_params_uuid_match(message, self_uuid)) {
			return result;
		}

		if (message.size == 2) {
			servo_params_info(result, params, module_version, module_type);
			return result;
		}

		uint8_t index = message.data[2];
		uint8_t count = 8;
		if (message.size >= 4 && message.data[3] > 0) {
			count = message.data[3];
		}
		servo_params_value_reply(result, params, index, count);
		return result;
	}

	if ((message.id & 0x780) != CAN_ID_SETUP) {
		return result;
	}

	if (!servo_params_uuid_match(message, self_uuid) || message.size < 2) {
		return result;
	}

	uint8_t index = (uint8_t)(message.id & 0x7F);

	if (index == 0) {
		uint8_t from = 2;
		if (message.size >= 4) {
			from = 3;
		}

		uint8_t n = 0;
		while (from < message.size && n < SERVO_PARAM_NAME_LEN) {
			params.name[n] = (char)message.data[from];
			n++;
			from++;
		}
		params.name[n] = 0;
		result.changed = true;
		return result;
	}

	if (index >= SERVO_PARAM_BYTES || message.size < 3) {
		return result;
	}

	uint8_t length = (uint8_t)(message.size - 2);
	if (length > CAN_VALUE_MAX_SIZE) {
		length = CAN_VALUE_MAX_SIZE;
	}
	if ((uint16_t)index + length > SERVO_PARAM_BYTES) {
		length = (uint8_t)(SERVO_PARAM_BYTES - index);
	}

	for (uint8_t i = 0; i < length; i++) {
		uint8_t at = (uint8_t)(index + i);
		if (at == SERVO_PARAM_VERSION && message.data[2 + i] != module_version) {
			return result;
		}
		if (at >= SERVO_PARAM_MAP && at < SERVO_PARAM_BYTES && !servo_map_valid(message.data[2 + i])) {
			return result;
		}
	}

	for (uint8_t i = 0; i < length; i++) {
		params.bytes[index + i] = message.data[2 + i];
	}

	params.bytes[SERVO_PARAM_VERSION] = module_version;
	result.changed = true;
	return result;
}
