/*
 * Host compile check for the Loco-CAN protocol header.
 *
 * This is the same ID set as github.com/loco-can/LocoCAN. A Platform UI
 * or other PC tool can include src/can_protocol.h without Arduino.
 */

#include "can_protocol.h"

#include <stdio.h>

#define REQUIRE(cond, name) \
	do { \
		if (!(cond)) { \
			fprintf(stderr, "protocol check failed: %s\n", name); \
			return 1; \
		} \
	} while (0)

int main(void) {

	REQUIRE(CAN_ID_DRIVE == 0x400, "CAN_ID_DRIVE");
	REQUIRE(CAN_ID_LIGHT == 0x410, "CAN_ID_LIGHT");
	REQUIRE(CAN_ID_SWITCH == 0x420, "CAN_ID_SWITCH");
	REQUIRE(CAN_ID_VEHICLE_STATUS == 0x500, "CAN_ID_VEHICLE_STATUS");
	REQUIRE(CAN_ID_MODULE_HEARTBEAT == 0x510, "CAN_ID_MODULE_HEARTBEAT");
	REQUIRE(CAN_ID_DRIVE_HEARTBEAT == 0x010, "CAN_ID_DRIVE_HEARTBEAT");
	REQUIRE(CAN_ID_TRAINEND_HEARTBEAT == 0x020, "CAN_ID_TRAINEND_HEARTBEAT");
	REQUIRE(CAN_ID_LOCO_SETUP == 0x0600, "CAN_ID_LOCO_SETUP");
	REQUIRE(CAN_ID_REQUEST == 0x7FF, "CAN_ID_REQUEST");
	REQUIRE(CAN_ID_REPLY == 0x780, "CAN_ID_REPLY");
	REQUIRE(CAN_ID_SETUP == 0x700, "CAN_ID_SETUP");

	REQUIRE(CONTROL_MAINS_FLAG == 0, "CONTROL_MAINS_FLAG");
	REQUIRE(CONTROL_DRIVE_FLAG == 1, "CONTROL_DRIVE_FLAG");
	REQUIRE(CONTROL_DIR_FLAG == 2, "CONTROL_DIR_FLAG");
	REQUIRE(LIGHT_POSIT == 0, "LIGHT_POSIT");
	REQUIRE(LIGHT_MAIN == 7, "LIGHT_MAIN");
	REQUIRE(CAN_ID_MASK == 0x770, "CAN_ID_MASK");

	printf("LocoCAN protocol host check ok (drive=0x%03X)\n", CAN_ID_DRIVE);
	return 0;
}
