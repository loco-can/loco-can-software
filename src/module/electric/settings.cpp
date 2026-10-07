/*
 * Electric-module settings image and CAN setup access.
 */

#include "settings.h"

#include "../../can_protocol.h"
#include "plugin.h"


static void electric_settings_copy_name(ELECTRIC_SETTINGS &settings, const char *name) {

	uint8_t i = 0;

	while (name[i] != 0 && i < ELECTRIC_SETTINGS_NAME_LEN) {
		settings.name[i] = name[i];
		i++;
	}

	settings.name[i] = 0;

	while (i < ELECTRIC_SETTINGS_NAME_LEN) {
		i++;
		settings.name[i] = 0;
	}
}


void electric_settings_defaults(ELECTRIC_SETTINGS &settings, uint8_t module_version) {

	for (uint8_t i = 0; i < ELECTRIC_PARAM_BYTES; i++) {
		settings.bytes[i] = 0;
	}

	settings.bytes[ELECTRIC_PARAM_VERSION] = module_version;
	settings.bytes[ELECTRIC_PARAM_PLUGIN] = ELECTRIC_PLUGIN_DIRECT;
	settings.bytes[ELECTRIC_PARAM_BATTERY_COUNT] = 1;
	settings.bytes[ELECTRIC_PARAM_PWM_MODE] = ELECTRIC_PWM_DRIVE_BRAKE;
	settings.bytes[ELECTRIC_PARAM_REVERSE] = 0;
	electric_settings_put16(settings, ELECTRIC_PARAM_VOLTAGE_MIN, ELECTRIC_VOLTAGE_MIN_DEFAULT);

	electric_settings_copy_name(settings, "MOTOR");
}


uint16_t electric_settings_get16(const ELECTRIC_SETTINGS &settings, uint8_t index) {

	if ((uint16_t)index + 1u >= ELECTRIC_PARAM_BYTES) {
		return 0;
	}

	return (uint16_t)settings.bytes[index] | ((uint16_t)settings.bytes[index + 1] << 8);
}


void electric_settings_put16(ELECTRIC_SETTINGS &settings, uint8_t index, uint16_t value) {

	if ((uint16_t)index + 1u >= ELECTRIC_PARAM_BYTES) {
		return;
	}

	settings.bytes[index] = (uint8_t)(value & 0xFF);
	settings.bytes[index + 1] = (uint8_t)((value >> 8) & 0xFF);
}


uint8_t electric_settings_plugin(const ELECTRIC_SETTINGS &settings) {

	uint8_t plugin = settings.bytes[ELECTRIC_PARAM_PLUGIN];

	if (plugin >= ELECTRIC_PLUGIN_COUNT) {
		return ELECTRIC_PLUGIN_DIRECT;
	}

	return plugin;
}


uint8_t electric_settings_battery_count(const ELECTRIC_SETTINGS &settings) {

	uint8_t count = settings.bytes[ELECTRIC_PARAM_BATTERY_COUNT];

	if (count > ELECTRIC_BATT_MAX) {
		return ELECTRIC_BATT_MAX;
	}

	return count;
}


uint8_t electric_settings_pwm_mode(const ELECTRIC_SETTINGS &settings) {

	if (settings.bytes[ELECTRIC_PARAM_PWM_MODE] == ELECTRIC_PWM_DRIVE_BRAKE) {
		return ELECTRIC_PWM_DRIVE_BRAKE;
	}

	return ELECTRIC_PWM_DRIVE;
}


bool electric_settings_reversed(const ELECTRIC_SETTINGS &settings) {

	return settings.bytes[ELECTRIC_PARAM_REVERSE] != 0;
}


uint16_t electric_settings_voltage_min(const ELECTRIC_SETTINGS &settings) {

	return electric_settings_get16(settings, ELECTRIC_PARAM_VOLTAGE_MIN);
}


void electric_settings_set_reversed(ELECTRIC_SETTINGS &settings, bool reversed) {

	settings.bytes[ELECTRIC_PARAM_REVERSE] = reversed ? 1 : 0;
}


static void electric_settings_clear(CAN_MESSAGE &message) {

	message.id = 0;
	message.uuid = 0;
	message.size = 0;

	for (uint8_t i = 0; i < 8; i++) {
		message.data[i] = 0;
	}
}


static bool electric_settings_uuid_match(const CAN_MESSAGE &message, uint16_t self_uuid) {

	if (message.size < 2) {
		return false;
	}

	uint16_t uuid = ((uint16_t)message.data[0] << 8) | message.data[1];
	return uuid == self_uuid;
}


static void electric_settings_info(ELECTRIC_SETTINGS_RESULT &result, const ELECTRIC_SETTINGS &settings, uint8_t module_version, uint8_t module_type) {

	CAN_MESSAGE &info = result.reply[0];
	electric_settings_clear(info);
	info.id = CAN_ID_REPLY;
	info.size = 5;
	info.data[0] = (uint8_t)(ELECTRIC_PARAM_BYTES - 1);
	info.data[1] = 0x20;
	info.data[2] = 0;
	info.data[3] = module_version;
	info.data[4] = module_type;

	CAN_MESSAGE &name = result.reply[1];
	electric_settings_clear(name);
	name.id = CAN_ID_REPLY;
	name.data[0] = 0x21;

	uint8_t n = 0;
	while (n < 7 && settings.name[n] != 0) {
		name.data[1 + n] = (uint8_t)settings.name[n];
		n++;
	}

	name.size = (uint8_t)(1 + n);
	result.reply_count = 2;
}


static void electric_settings_value_reply(ELECTRIC_SETTINGS_RESULT &result, const ELECTRIC_SETTINGS &settings, uint8_t index, uint8_t count) {

	if (index >= ELECTRIC_PARAM_BYTES) {
		return;
	}

	if (count == 0) {
		count = 8;
	}
	if (count > 8) {
		count = 8;
	}
	if ((uint16_t)index + count > ELECTRIC_PARAM_BYTES) {
		count = (uint8_t)(ELECTRIC_PARAM_BYTES - index);
	}

	CAN_MESSAGE &reply = result.reply[0];
	electric_settings_clear(reply);
	reply.id = (uint32_t)(CAN_ID_REPLY | (index & 0x7F));
	reply.size = count;

	for (uint8_t i = 0; i < count; i++) {
		reply.data[i] = settings.bytes[index + i];
	}

	result.reply_count = 1;
}


static uint8_t electric_settings_accept(uint8_t index, uint8_t value, uint8_t previous) {

	if (index == ELECTRIC_PARAM_PLUGIN && value >= ELECTRIC_PLUGIN_COUNT) {
		return previous;
	}

	if (index == ELECTRIC_PARAM_BATTERY_COUNT && value > ELECTRIC_BATT_MAX) {
		return ELECTRIC_BATT_MAX;
	}

	if (index == ELECTRIC_PARAM_PWM_MODE && value > ELECTRIC_PWM_DRIVE_BRAKE) {
		return ELECTRIC_PWM_DRIVE;
	}

	if (index == ELECTRIC_PARAM_REVERSE) {
		return value == 0 ? 0 : 1;
	}

	return value;
}


ELECTRIC_SETTINGS_RESULT electric_settings_on_can(
	ELECTRIC_SETTINGS &settings,
	const CAN_MESSAGE &message,
	uint16_t self_uuid,
	uint8_t module_version,
	uint8_t module_type
) {

	ELECTRIC_SETTINGS_RESULT result;
	result.reply_count = 0;
	result.changed = false;
	electric_settings_clear(result.reply[0]);
	electric_settings_clear(result.reply[1]);

	if (message.id == CAN_ID_REQUEST) {
		bool global = (message.size == 0) || (message.size >= 1 && message.data[0] == 0xFF);
		if (global) {
			electric_settings_info(result, settings, module_version, module_type);
			return result;
		}

		if (!electric_settings_uuid_match(message, self_uuid)) {
			return result;
		}

		if (message.size == 2) {
			electric_settings_info(result, settings, module_version, module_type);
			return result;
		}

		uint8_t index = message.data[2];
		uint8_t count = 8;
		if (message.size >= 4 && message.data[3] > 0) {
			count = message.data[3];
		}
		electric_settings_value_reply(result, settings, index, count);
		return result;
	}

	if ((message.id & 0x780) != CAN_ID_SETUP) {
		return result;
	}

	if (!electric_settings_uuid_match(message, self_uuid) || message.size < 2) {
		return result;
	}

	uint8_t index = (uint8_t)(message.id & 0x7F);

	if (index == 0) {
		uint8_t from = 2;
		if (message.size >= 4) {
			from = 3;
		}

		uint8_t n = 0;
		while (from < message.size && n < ELECTRIC_SETTINGS_NAME_LEN) {
			settings.name[n] = (char)message.data[from];
			n++;
			from++;
		}
		settings.name[n] = 0;
		result.changed = true;
		return result;
	}

	if (index >= ELECTRIC_PARAM_BYTES || message.size < 3) {
		return result;
	}

	if (index == ELECTRIC_PARAM_VERSION) {
		return result;
	}

	uint8_t length = (uint8_t)(message.size - 2);
	if (length > CAN_VALUE_MAX_SIZE) {
		length = CAN_VALUE_MAX_SIZE;
	}
	if ((uint16_t)index + length > ELECTRIC_PARAM_BYTES) {
		length = (uint8_t)(ELECTRIC_PARAM_BYTES - index);
	}

	for (uint8_t i = 0; i < length; i++) {
		if ((uint8_t)(index + i) == ELECTRIC_PARAM_VERSION && message.data[2 + i] != module_version) {
			return result;
		}
	}

	for (uint8_t i = 0; i < length; i++) {
		uint8_t at = (uint8_t)(index + i);
		if (at == ELECTRIC_PARAM_VERSION) {
			continue;
		}
		settings.bytes[at] = electric_settings_accept(at, message.data[2 + i], settings.bytes[at]);
	}

	settings.bytes[ELECTRIC_PARAM_VERSION] = module_version;
	result.changed = true;
	return result;
}
