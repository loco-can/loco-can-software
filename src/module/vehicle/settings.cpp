/*
 * Vehicle-module settings image and CAN setup access.
 */

#include "settings.h"

#include <string.h>

#include "../../can_protocol.h"
#include "plugin.h"


static void vehicle_settings_copy_name(VEHICLE_SETTINGS &settings, const char *name) {

	uint8_t i = 0;

	while (name[i] != 0 && i < VEHICLE_SETTINGS_NAME_LEN) {
		settings.name[i] = name[i];
		i++;
	}

	settings.name[i] = 0;

	while (i < VEHICLE_SETTINGS_NAME_LEN) {
		i++;
		settings.name[i] = 0;
	}
}


void vehicle_settings_defaults(VEHICLE_SETTINGS &settings, uint32_t module_version) {

	for (uint8_t i = 0; i < VEHICLE_PARAM_BYTES; i++) {
		settings.bytes[i] = 0;
	}

	settings.bytes[VEHICLE_PARAM_VERSION] = (uint8_t)LOCO_VERSION_CODE(module_version);
	settings.bytes[VEHICLE_PARAM_PLUGIN] = VEHICLE_PLUGIN_DIRECT;
	settings.bytes[VEHICLE_PARAM_BATTERY_COUNT] = 1;
	settings.bytes[VEHICLE_PARAM_PWM_MODE] = VEHICLE_PWM_DRIVE_BRAKE;
	settings.bytes[VEHICLE_PARAM_REVERSE] = 0;
	vehicle_settings_put16(settings, VEHICLE_PARAM_VOLTAGE_MIN, VEHICLE_VOLTAGE_MIN_DEFAULT);

	vehicle_settings_copy_name(settings, "VEHICLE");
}


uint16_t vehicle_settings_get16(const VEHICLE_SETTINGS &settings, uint8_t index) {

	if ((uint16_t)index + 1u >= VEHICLE_PARAM_BYTES) {
		return 0;
	}

	return (uint16_t)settings.bytes[index] | ((uint16_t)settings.bytes[index + 1] << 8);
}


void vehicle_settings_put16(VEHICLE_SETTINGS &settings, uint8_t index, uint16_t value) {

	if ((uint16_t)index + 1u >= VEHICLE_PARAM_BYTES) {
		return;
	}

	settings.bytes[index] = (uint8_t)(value & 0xFF);
	settings.bytes[index + 1] = (uint8_t)((value >> 8) & 0xFF);
}


uint8_t vehicle_settings_plugin(const VEHICLE_SETTINGS &settings) {

	uint8_t plugin = settings.bytes[VEHICLE_PARAM_PLUGIN];

	if (plugin >= VEHICLE_PLUGIN_COUNT) {
		return VEHICLE_PLUGIN_DIRECT;
	}

	return plugin;
}


uint8_t vehicle_settings_battery_count(const VEHICLE_SETTINGS &settings) {

	uint8_t count = settings.bytes[VEHICLE_PARAM_BATTERY_COUNT];

	if (count > VEHICLE_BATT_MAX) {
		return VEHICLE_BATT_MAX;
	}

	return count;
}


uint8_t vehicle_settings_pwm_mode(const VEHICLE_SETTINGS &settings) {

	if (settings.bytes[VEHICLE_PARAM_PWM_MODE] == VEHICLE_PWM_DRIVE_BRAKE) {
		return VEHICLE_PWM_DRIVE_BRAKE;
	}

	return VEHICLE_PWM_DRIVE;
}


bool vehicle_settings_reversed(const VEHICLE_SETTINGS &settings) {

	return settings.bytes[VEHICLE_PARAM_REVERSE] != 0;
}


uint16_t vehicle_settings_voltage_min(const VEHICLE_SETTINGS &settings) {

	return vehicle_settings_get16(settings, VEHICLE_PARAM_VOLTAGE_MIN);
}


void vehicle_settings_set_reversed(VEHICLE_SETTINGS &settings, bool reversed) {

	settings.bytes[VEHICLE_PARAM_REVERSE] = reversed ? 1 : 0;
}


static void vehicle_settings_clear(CAN_MESSAGE &message) {

	message.id = 0;
	message.uuid = 0;
	message.size = 0;

	for (uint8_t i = 0; i < 8; i++) {
		message.data[i] = 0;
	}
}


static bool vehicle_settings_uuid_match(const CAN_MESSAGE &message, uint16_t self_uuid) {

	if (message.size < 2) {
		return false;
	}

	uint16_t uuid = ((uint16_t)message.data[0] << 8) | message.data[1];
	return uuid == self_uuid;
}


static void vehicle_settings_info(VEHICLE_SETTINGS_RESULT &result, const VEHICLE_SETTINGS &settings, uint32_t module_version, uint8_t module_type) {

	uint8_t nlen = 0;
	while (nlen < VEHICLE_SETTINGS_NAME_LEN && settings.name[nlen] != 0) {
		nlen++;
	}

	uint8_t name_packets = nlen == 0 ? 1 : (uint8_t)((nlen + CAN_NAME_REPLY_SIZE - 1) / CAN_NAME_REPLY_SIZE);
	if (name_packets > CAN_SETUP_REPLY_MAX - 1) {
		name_packets = CAN_SETUP_REPLY_MAX - 1;
	}
	uint8_t total = (uint8_t)(1 + name_packets);

	CAN_MESSAGE &info = result.reply[0];
	vehicle_settings_clear(info);
	info.id = CAN_ID_REPLY;
	info.size = 7;
	info.data[0] = (uint8_t)(VEHICLE_PARAM_BYTES - 1);
	info.data[1] = (uint8_t)(total << 4);
	info.data[2] = 0;
	info.data[3] = (uint8_t)LOCO_VERSION_MAJOR(module_version);
	info.data[4] = (uint8_t)LOCO_VERSION_MINOR(module_version);
	info.data[5] = (uint8_t)LOCO_VERSION_PATCH(module_version);
	info.data[6] = module_type;

	for (uint8_t p = 0; p < name_packets; p++) {
		CAN_MESSAGE &name = result.reply[1 + p];
		vehicle_settings_clear(name);
		name.id = CAN_ID_REPLY;
		name.data[0] = (uint8_t)((total << 4) | (p + 1));

		uint8_t n = 0;
		uint8_t from = (uint8_t)(p * CAN_NAME_REPLY_SIZE);
		while (n < CAN_NAME_REPLY_SIZE && (uint8_t)(from + n) < nlen) {
			name.data[1 + n] = (uint8_t)settings.name[from + n];
			n++;
		}
		name.size = (uint8_t)(1 + n);
	}

	result.reply_count = total;
}


static void vehicle_settings_value_reply(VEHICLE_SETTINGS_RESULT &result, const VEHICLE_SETTINGS &settings, uint8_t index, uint8_t count) {

	if (index >= VEHICLE_PARAM_BYTES) {
		return;
	}

	if (count == 0) {
		count = 8;
	}
	if (count > 8) {
		count = 8;
	}
	if ((uint16_t)index + count > VEHICLE_PARAM_BYTES) {
		count = (uint8_t)(VEHICLE_PARAM_BYTES - index);
	}

	CAN_MESSAGE &reply = result.reply[0];
	vehicle_settings_clear(reply);
	reply.id = (uint32_t)(CAN_ID_REPLY | (index & 0x7F));
	reply.size = count;

	for (uint8_t i = 0; i < count; i++) {
		reply.data[i] = settings.bytes[index + i];
	}

	result.reply_count = 1;
}


static uint8_t vehicle_settings_accept(uint8_t index, uint8_t value, uint8_t previous) {

	if (index == VEHICLE_PARAM_PLUGIN && value >= VEHICLE_PLUGIN_COUNT) {
		return previous;
	}

	if (index == VEHICLE_PARAM_BATTERY_COUNT && value > VEHICLE_BATT_MAX) {
		return VEHICLE_BATT_MAX;
	}

	if (index == VEHICLE_PARAM_PWM_MODE && value > VEHICLE_PWM_DRIVE_BRAKE) {
		return VEHICLE_PWM_DRIVE;
	}

	if (index == VEHICLE_PARAM_REVERSE) {
		return value == 0 ? 0 : 1;
	}

	return value;
}


VEHICLE_SETTINGS_RESULT vehicle_settings_on_can(
	VEHICLE_SETTINGS &settings,
	const CAN_MESSAGE &message,
	uint16_t self_uuid,
	uint32_t module_version,
	uint8_t module_type
) {

	VEHICLE_SETTINGS_RESULT result;
	result.reply_count = 0;
	result.changed = false;
	for (uint8_t i = 0; i < CAN_SETUP_REPLY_MAX; i++) {
		vehicle_settings_clear(result.reply[i]);
	}

	if (message.id == CAN_ID_REQUEST) {
		bool global = (message.size == 0) || (message.size >= 1 && message.data[0] == 0xFF);
		if (global) {
			vehicle_settings_info(result, settings, module_version, module_type);
			return result;
		}

		if (!vehicle_settings_uuid_match(message, self_uuid)) {
			return result;
		}

		if (message.size == 2) {
			vehicle_settings_info(result, settings, module_version, module_type);
			return result;
		}

		uint8_t index = message.data[2];
		uint8_t count = 8;
		if (message.size >= 4 && message.data[3] > 0) {
			count = message.data[3];
		}
		vehicle_settings_value_reply(result, settings, index, count);
		return result;
	}

	if ((message.id & 0x780) != CAN_ID_SETUP) {
		return result;
	}

	if (!vehicle_settings_uuid_match(message, self_uuid) || message.size < 2) {
		return result;
	}

	uint8_t index = (uint8_t)(message.id & 0x7F);

	if (index == 0) {
		uint8_t from = 2;
		uint8_t n = 0;
		if (message.size >= 4) {
			from = 3;
			n = (uint8_t)(message.data[2] * CAN_NAME_MAX_SIZE);
			if (n >= VEHICLE_SETTINGS_NAME_LEN) {
				return result;
			}
		}

		if (n == 0) {
			memset(settings.name, 0, VEHICLE_SETTINGS_NAME_LEN + 1);
		}

		while (from < message.size && n < VEHICLE_SETTINGS_NAME_LEN) {
			settings.name[n] = (char)message.data[from];
			n++;
			from++;
		}
		settings.name[n] = 0;
		result.changed = true;
		return result;
	}

	if (index >= VEHICLE_PARAM_BYTES || message.size < 3) {
		return result;
	}

	if (index == VEHICLE_PARAM_VERSION) {
		return result;
	}

	uint8_t length = (uint8_t)(message.size - 2);
	if (length > CAN_VALUE_MAX_SIZE) {
		length = CAN_VALUE_MAX_SIZE;
	}
	if ((uint16_t)index + length > VEHICLE_PARAM_BYTES) {
		length = (uint8_t)(VEHICLE_PARAM_BYTES - index);
	}

	for (uint8_t i = 0; i < length; i++) {
		if ((uint8_t)(index + i) == VEHICLE_PARAM_VERSION && message.data[2 + i] != (uint8_t)LOCO_VERSION_CODE(module_version)) {
			return result;
		}
	}

	for (uint8_t i = 0; i < length; i++) {
		uint8_t at = (uint8_t)(index + i);
		if (at == VEHICLE_PARAM_VERSION) {
			continue;
		}
		settings.bytes[at] = vehicle_settings_accept(at, message.data[2 + i], settings.bytes[at]);
	}

	settings.bytes[VEHICLE_PARAM_VERSION] = (uint8_t)LOCO_VERSION_CODE(module_version);
	result.changed = true;
	return result;
}
