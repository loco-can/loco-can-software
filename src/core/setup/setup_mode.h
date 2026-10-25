/*
 * Shared module setup-mode session.
 *
 * Enter is refused while a controller drive heartbeat is fresh.
 * A later drive heartbeat aborts an active session.
 */

#pragma once

#ifndef LOCO_SETUP_MODE_H
#define LOCO_SETUP_MODE_H

#include "../../can_protocol.h"
#include "../can/can_message.h"

#include <string.h>

#define LOCO_SETUP_EV_NONE 0
#define LOCO_SETUP_EV_ENTER 1
#define LOCO_SETUP_EV_REFUSE 2
#define LOCO_SETUP_EV_LEAVE 3
#define LOCO_SETUP_EV_ABORT 4

struct LOCO_SETUP_SESSION {
	bool active;
	bool saw_controller;
	uint32_t controller_ms;
	uint32_t setup_ms;
};

extern LOCO_SETUP_SESSION loco_setup;

inline void loco_setup_clear(LOCO_SETUP_SESSION &session) {
	session.active = false;
	session.saw_controller = false;
	session.controller_ms = 0;
	session.setup_ms = 0;
}

inline bool loco_setup_is_range(uint32_t id) {
	return id >= CAN_ID_SETUP && id <= CAN_ID_REQUEST;
}

inline bool loco_setup_is_enter(uint32_t id, uint8_t size, uint8_t first) {
	return id == CAN_ID_REQUEST && size == 1 && first == CAN_SETUP_MODE_ENTER;
}

inline bool loco_setup_is_leave(uint32_t id, uint8_t size, uint8_t first) {
	return id == CAN_ID_REQUEST && size == 1 && first == CAN_SETUP_MODE_LEAVE;
}

inline void loco_setup_note_controller(LOCO_SETUP_SESSION &session, uint32_t now) {
	session.controller_ms = now;
	session.saw_controller = true;
}

inline bool loco_setup_controller_present(const LOCO_SETUP_SESSION &session, uint32_t now) {
	if (!session.saw_controller) {
		return false;
	}
	return (uint32_t)(now - session.controller_ms) < (uint32_t)CAN_SETUP_MODE_HOLD_MS;
}

inline bool loco_setup_expired(const LOCO_SETUP_SESSION &session, uint32_t now) {
	if (!session.active) {
		return false;
	}
	return (uint32_t)(now - session.setup_ms) >= (uint32_t)CAN_SETUP_MODE_TIMEOUT_MS;
}

inline void loco_setup_touch(LOCO_SETUP_SESSION &session, uint32_t now) {
	if (session.active) {
		session.setup_ms = now;
	}
}

inline uint8_t loco_setup_on_frame(
	LOCO_SETUP_SESSION &session,
	uint32_t id,
	uint8_t size,
	uint8_t first,
	uint32_t now
) {
	if (id == CAN_ID_DRIVE_HEARTBEAT) {
		loco_setup_note_controller(session, now);
		if (session.active) {
			/*
			 * A cab heartbeat that was already on the wire when ENTER
			 * was sent must not abort the session. Live heartbeats
			 * after the hold window still do.
			 */
			if ((uint32_t)(now - session.setup_ms) < (uint32_t)CAN_SETUP_MODE_HOLD_MS) {
				return LOCO_SETUP_EV_NONE;
			}
			session.active = false;
			return LOCO_SETUP_EV_ABORT;
		}
		return LOCO_SETUP_EV_NONE;
	}

	if (loco_setup_is_enter(id, size, first)) {
		if (loco_setup_controller_present(session, now)) {
			return LOCO_SETUP_EV_REFUSE;
		}
		session.active = true;
		session.setup_ms = now;
		return LOCO_SETUP_EV_ENTER;
	}

	if (loco_setup_is_leave(id, size, first)) {
		session.active = false;
		return LOCO_SETUP_EV_LEAVE;
	}

	if (session.active && loco_setup_is_range(id)) {
		session.setup_ms = now;
	}

	return LOCO_SETUP_EV_NONE;
}

inline void loco_setup_make_ack(
	CAN_MESSAGE &out,
	uint16_t uuid,
	uint8_t op,
	uint8_t type,
	uint32_t version,
	uint8_t status
) {
	memset(&out, 0, sizeof(out));
	out.id = CAN_ID_SETUP_MODE;
	out.uuid = uuid;
	out.size = 6;
	out.data[0] = op;
	out.data[1] = type;
	out.data[2] = status;
	out.data[3] = (uint8_t)LOCO_VERSION_MAJOR(version);
	out.data[4] = (uint8_t)LOCO_VERSION_MINOR(version);
	out.data[5] = (uint8_t)LOCO_VERSION_PATCH(version);
}

#endif
