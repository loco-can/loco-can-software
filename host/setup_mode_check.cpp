/*
 * Host checks for the shared module setup-mode session.
 */

#include "can_protocol.h"
#include "core/setup/setup_mode.h"

#include <stdio.h>

static int g_fails = 0;

static void expect_u8(const char *name, uint8_t got, uint8_t want) {
	if (got != want) {
		fprintf(stderr, "FAIL %s: %u, want %u\n", name, got, want);
		g_fails++;
	}
}

static void expect_true(const char *name, bool cond) {
	if (!cond) {
		fprintf(stderr, "FAIL %s\n", name);
		g_fails++;
	}
}

int setup_mode_check(void) {

	LOCO_SETUP_SESSION session;
	loco_setup_clear(session);

	expect_true("range request", loco_setup_is_range(CAN_ID_REQUEST));
	expect_true("range write", loco_setup_is_range(CAN_ID_SETUP));
	expect_true("range reply", loco_setup_is_range(CAN_ID_REPLY));
	expect_true("drive not range", !loco_setup_is_range(CAN_ID_DRIVE));
	expect_true("enter opcode", loco_setup_is_enter(CAN_ID_REQUEST, 1, CAN_SETUP_MODE_ENTER));
	expect_true("leave opcode", loco_setup_is_leave(CAN_ID_REQUEST, 1, CAN_SETUP_MODE_LEAVE));
	expect_true("global ping not enter", !loco_setup_is_enter(CAN_ID_REQUEST, 1, 0xFF));

	uint8_t ev = loco_setup_on_frame(
		session, CAN_ID_REQUEST, 1, CAN_SETUP_MODE_ENTER, 1000
	);
	expect_u8("enter ok", ev, LOCO_SETUP_EV_ENTER);
	expect_true("active after enter", session.active);

	ev = loco_setup_on_frame(
		session, CAN_ID_REQUEST, 1, CAN_SETUP_MODE_LEAVE, 1100
	);
	expect_u8("leave ok", ev, LOCO_SETUP_EV_LEAVE);
	expect_true("inactive after leave", !session.active);

	loco_setup_on_frame(session, CAN_ID_DRIVE_HEARTBEAT, 0, 0, 2000);
	ev = loco_setup_on_frame(
		session, CAN_ID_REQUEST, 1, CAN_SETUP_MODE_ENTER, 2050
	);
	expect_u8("refuse while controller", ev, LOCO_SETUP_EV_REFUSE);
	expect_true("stay out after refuse", !session.active);

	ev = loco_setup_on_frame(
		session, CAN_ID_REQUEST, 1, CAN_SETUP_MODE_ENTER, 2000 + CAN_SETUP_MODE_HOLD_MS + 1
	);
	expect_u8("enter after hold", ev, LOCO_SETUP_EV_ENTER);

	ev = loco_setup_on_frame(session, CAN_ID_DRIVE_HEARTBEAT, 0, 0, 4000);
	expect_u8("abort on controller", ev, LOCO_SETUP_EV_ABORT);
	expect_true("inactive after abort", !session.active);

	loco_setup_clear(session);
	loco_setup_on_frame(session, CAN_ID_REQUEST, 1, CAN_SETUP_MODE_ENTER, 8000);
	ev = loco_setup_on_frame(
		session, CAN_ID_DRIVE_HEARTBEAT, 0, 0, 8000 + CAN_SETUP_MODE_HOLD_MS - 1
	);
	expect_u8("leftover heartbeat ignored", ev, LOCO_SETUP_EV_NONE);
	expect_true("stay in after leftover", session.active);
	ev = loco_setup_on_frame(
		session, CAN_ID_DRIVE_HEARTBEAT, 0, 0, 8000 + CAN_SETUP_MODE_HOLD_MS
	);
	expect_u8("late heartbeat aborts", ev, LOCO_SETUP_EV_ABORT);
	expect_true("inactive after late heartbeat", !session.active);

	loco_setup_clear(session);
	loco_setup_on_frame(session, CAN_ID_REQUEST, 1, CAN_SETUP_MODE_ENTER, 10);
	expect_true(
		"expired",
		loco_setup_expired(session, 10 + CAN_SETUP_MODE_TIMEOUT_MS)
	);
	loco_setup_on_frame(session, CAN_ID_REQUEST, 2, 0x12, 20);
	expect_true(
		"touch keeps alive",
		!loco_setup_expired(session, 20 + CAN_SETUP_MODE_TIMEOUT_MS - 1)
	);

	CAN_MESSAGE ack;
	loco_setup_make_ack(ack, 0x1234, CAN_SETUP_MODE_ENTER, TYPE_SWITCH, LOCO_VERSION(2, 1, 0), CAN_SETUP_MODE_OK);
	expect_u8("ack id high", (uint8_t)(ack.id >> 8), 0x07);
	expect_u8("ack id low", (uint8_t)ack.id, 0xF8);
	expect_u8("ack op", ack.data[0], CAN_SETUP_MODE_ENTER);
	expect_u8("ack type", ack.data[1], TYPE_SWITCH);
	expect_u8("ack status", ack.data[2], CAN_SETUP_MODE_OK);
	expect_u8("ack major", ack.data[3], 2);

	if (g_fails != 0) {
		fprintf(stderr, "setup mode check failed (%d)\n", g_fails);
		return 1;
	}

	printf("setup mode check ok\n");
	return 0;
}
