/*
 * Controller parameter image and CAN setup access.
 */

#include "params.h"

#include "../../can_protocol.h"


static uint16_t controller_params_spaced(uint16_t resolution, uint8_t index, uint8_t count) {

	if (count < 2) {
		return 0;
	}

	uint16_t span = 0;
	if (resolution > 0) {
		span = (uint16_t)(resolution - 1);
	}

	return (uint16_t)(((uint32_t)span * index) / (uint8_t)(count - 1));
}


static void controller_params_fill_switch(CONTROLLER_PARAMS &params, uint8_t count_index, uint8_t points, uint16_t resolution) {

	if ((uint16_t)count_index + 1u + (uint16_t)points * 2u > CONTROLLER_PARAM_BYTES) {
		return;
	}

	params.bytes[count_index] = points;

	for (uint8_t i = 0; i < points; i++) {
		controller_params_put16(params, (uint8_t)(count_index + 1 + (i * 2)), controller_params_spaced(resolution, i, points));
	}
}


static void controller_params_copy_name(CONTROLLER_PARAMS &params, const char *name) {

	uint8_t i = 0;

	while (name[i] != 0 && i < CONTROLLER_PARAM_NAME_LEN) {
		params.name[i] = name[i];
		i++;
	}

	params.name[i] = 0;

	while (i < CONTROLLER_PARAM_NAME_LEN) {
		i++;
		params.name[i] = 0;
	}
}


void controller_params_defaults(CONTROLLER_PARAMS &params, uint16_t resolution, bool dual_brake, uint8_t module_version) {

	for (uint8_t i = 0; i < CONTROLLER_PARAM_BYTES; i++) {
		params.bytes[i] = 0;
	}

	params.bytes[CONTROLLER_PARAM_VERSION] = module_version;

	controller_params_fill_switch(params, CONTROLLER_PARAM_MAINS_COUNT, CONTROLLER_PARAM_MAINS_POINTS, resolution);
	controller_params_fill_switch(params, CONTROLLER_PARAM_DIR_COUNT, CONTROLLER_PARAM_DIR_POINTS, resolution);
	controller_params_fill_switch(params, CONTROLLER_PARAM_LIGHT_COUNT, CONTROLLER_PARAM_LIGHT_POINTS, resolution);
	controller_params_fill_switch(params, CONTROLLER_PARAM_LIGHT2_COUNT, CONTROLLER_PARAM_LIGHT2_POINTS, resolution);

	uint16_t full = 0;
	if (resolution > 0) {
		full = (uint16_t)(resolution - 1);
	}

	uint16_t center = (uint16_t)(full / 2);

	if (dual_brake) {
		controller_params_put16(params, CONTROLLER_PARAM_DRIVE_ZERO, 0);
		controller_params_put16(params, CONTROLLER_PARAM_DRIVE_FULL, full);
		controller_params_put16(params, CONTROLLER_PARAM_BRAKE_ZERO, 0);
		controller_params_put16(params, CONTROLLER_PARAM_BRAKE_FULL, full);
	}
	else {
		controller_params_put16(params, CONTROLLER_PARAM_DRIVE_ZERO, center);
		controller_params_put16(params, CONTROLLER_PARAM_DRIVE_FULL, full);
		controller_params_put16(params, CONTROLLER_PARAM_BRAKE_ZERO, center);
		controller_params_put16(params, CONTROLLER_PARAM_BRAKE_FULL, 0);
	}

	params.bytes[CONTROLLER_PARAM_GAUGE_BATT_MODE] = CONTROLLER_GAUGE_MODE_SERVO;
	controller_params_put16(params, CONTROLLER_PARAM_GAUGE_BATT_REF, CONTROLLER_GAUGE_REF_VOLTAGE);
	params.bytes[CONTROLLER_PARAM_GAUGE_MOTOR_V_MODE] = CONTROLLER_GAUGE_MODE_SERVO;
	controller_params_put16(params, CONTROLLER_PARAM_GAUGE_MOTOR_V_REF, CONTROLLER_GAUGE_REF_VOLTAGE);
	params.bytes[CONTROLLER_PARAM_GAUGE_CURRENT_MODE] = CONTROLLER_GAUGE_MODE_SERVO;
	controller_params_put16(params, CONTROLLER_PARAM_GAUGE_CURRENT_REF, CONTROLLER_GAUGE_REF_CURRENT);
	params.bytes[CONTROLLER_PARAM_GAUGE_MOTOR_I_MODE] = CONTROLLER_GAUGE_MODE_SERVO;
	controller_params_put16(params, CONTROLLER_PARAM_GAUGE_MOTOR_I_REF, CONTROLLER_GAUGE_REF_CURRENT);

	controller_params_copy_name(params, "CTRL");
}


uint16_t controller_params_get16(const CONTROLLER_PARAMS &params, uint8_t index) {

	if ((uint16_t)index + 1u >= CONTROLLER_PARAM_BYTES) {
		return 0;
	}

	return (uint16_t)params.bytes[index] | ((uint16_t)params.bytes[index + 1] << 8);
}


void controller_params_put16(CONTROLLER_PARAMS &params, uint8_t index, uint16_t value) {

	if ((uint16_t)index + 1u >= CONTROLLER_PARAM_BYTES) {
		return;
	}

	params.bytes[index] = (uint8_t)(value & 0xFF);
	params.bytes[index + 1] = (uint8_t)((value >> 8) & 0xFF);
}


uint8_t controller_params_points(const CONTROLLER_PARAMS &params, uint8_t count_index, uint8_t max_points, uint16_t *out) {

	if (count_index >= CONTROLLER_PARAM_BYTES || out == 0) {
		return 0;
	}

	uint8_t count = params.bytes[count_index];
	if (count > max_points) {
		count = max_points;
	}

	for (uint8_t i = 0; i < count; i++) {
		out[i] = controller_params_get16(params, (uint8_t)(count_index + 1 + (i * 2)));
	}

	return count;
}


static void controller_params_clear(CAN_MESSAGE &message) {

	message.id = 0;
	message.uuid = 0;
	message.size = 0;

	for (uint8_t i = 0; i < 8; i++) {
		message.data[i] = 0;
	}
}


static bool controller_params_uuid_match(const CAN_MESSAGE &message, uint16_t self_uuid) {

	if (message.size < 2) {
		return false;
	}

	uint16_t uuid = ((uint16_t)message.data[0] << 8) | message.data[1];
	return uuid == self_uuid;
}


static void controller_params_info(CONTROLLER_PARAM_RESULT &result, const CONTROLLER_PARAMS &params, uint8_t module_version, uint8_t module_type) {

	CAN_MESSAGE &info = result.reply[0];
	controller_params_clear(info);
	info.id = CAN_ID_REPLY;
	info.size = 5;
	info.data[0] = (uint8_t)(CONTROLLER_PARAM_BYTES - 1);
	info.data[1] = 0x20;
	info.data[2] = 0;
	info.data[3] = module_version;
	info.data[4] = module_type;

	CAN_MESSAGE &name = result.reply[1];
	controller_params_clear(name);
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


static void controller_params_value_reply(CONTROLLER_PARAM_RESULT &result, const CONTROLLER_PARAMS &params, uint8_t index, uint8_t count) {

	if (index >= CONTROLLER_PARAM_BYTES) {
		return;
	}

	if (count == 0) {
		count = 8;
	}
	if (count > 8) {
		count = 8;
	}
	if ((uint16_t)index + count > CONTROLLER_PARAM_BYTES) {
		count = (uint8_t)(CONTROLLER_PARAM_BYTES - index);
	}

	CAN_MESSAGE &reply = result.reply[0];
	controller_params_clear(reply);
	reply.id = (uint32_t)(CAN_ID_REPLY | (index & 0x7F));
	reply.size = count;

	for (uint8_t i = 0; i < count; i++) {
		reply.data[i] = params.bytes[index + i];
	}

	result.reply_count = 1;
}


CONTROLLER_PARAM_RESULT controller_params_on_can(
	CONTROLLER_PARAMS &params,
	const CAN_MESSAGE &message,
	uint16_t self_uuid,
	uint8_t module_version,
	uint8_t module_type
) {

	CONTROLLER_PARAM_RESULT result;
	result.reply_count = 0;
	result.changed = false;
	controller_params_clear(result.reply[0]);
	controller_params_clear(result.reply[1]);

	if (message.id == CAN_ID_REQUEST) {
		bool global = (message.size == 0) || (message.size >= 1 && message.data[0] == 0xFF);
		if (global) {
			controller_params_info(result, params, module_version, module_type);
			return result;
		}

		if (!controller_params_uuid_match(message, self_uuid)) {
			return result;
		}

		if (message.size == 2) {
			controller_params_info(result, params, module_version, module_type);
			return result;
		}

		uint8_t index = message.data[2];
		uint8_t count = 8;
		if (message.size >= 4 && message.data[3] > 0) {
			count = message.data[3];
		}
		controller_params_value_reply(result, params, index, count);
		return result;
	}

	if ((message.id & 0x780) != CAN_ID_SETUP) {
		return result;
	}

	if (!controller_params_uuid_match(message, self_uuid) || message.size < 2) {
		return result;
	}

	uint8_t index = (uint8_t)(message.id & 0x7F);

	if (index == 0) {
		uint8_t from = 2;
		if (message.size >= 4) {
			from = 3;
		}

		uint8_t n = 0;
		while (from < message.size && n < CONTROLLER_PARAM_NAME_LEN) {
			params.name[n] = (char)message.data[from];
			n++;
			from++;
		}
		params.name[n] = 0;
		result.changed = true;
		return result;
	}

	if (index >= CONTROLLER_PARAM_BYTES || message.size < 3) {
		return result;
	}

	/* byte 0 is the running module version and is not writable */
	if (index == CONTROLLER_PARAM_VERSION) {
		return result;
	}

	uint8_t length = (uint8_t)(message.size - 2);
	if (length > CAN_VALUE_MAX_SIZE) {
		length = CAN_VALUE_MAX_SIZE;
	}
	if ((uint16_t)index + length > CONTROLLER_PARAM_BYTES) {
		length = (uint8_t)(CONTROLLER_PARAM_BYTES - index);
	}

	for (uint8_t i = 0; i < length; i++) {
		if ((uint8_t)(index + i) == CONTROLLER_PARAM_VERSION && message.data[2 + i] != module_version) {
			return result;
		}
	}

	for (uint8_t i = 0; i < length; i++) {
		params.bytes[index + i] = message.data[2 + i];
	}

	params.bytes[CONTROLLER_PARAM_VERSION] = module_version;
	result.changed = true;
	return result;
}
