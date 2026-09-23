/*
 * Host compile check for the Loco-CAN protocol header.
 *
 * This is the same ID set as github.com/loco-can/LocoCAN. A Platform UI
 * or other PC tool can include src/can_protocol.h without Arduino.
 */

#include "can_protocol.h"
#include "module/wifi/now_packet.h"

#include <stdio.h>
#include <string.h>

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

	WIFI_NOW_PACKET pkt;
	memset(&pkt, 0, sizeof(pkt));
	pkt.type = WIFI_NOW_CAN;
	pkt.seq = 7;
	pkt.uuid = 0xA11E;
	pkt.can_id = CAN_ID_DRIVE;
	pkt.can_uuid = 0x1234;
	pkt.size = 3;
	pkt.flags = 0;
	pkt.data[0] = 0x01;
	pkt.data[1] = 0x02;
	pkt.data[2] = 0x03;

	uint8_t wire[WIFI_NOW_WIRE_SIZE];
	REQUIRE(wifi_now_pack(&pkt, wire, sizeof(wire)) == WIFI_NOW_WIRE_SIZE, "wifi_now_pack");

	WIFI_NOW_PACKET out;
	REQUIRE(wifi_now_unpack(wire, WIFI_NOW_WIRE_SIZE, &out), "wifi_now_unpack");
	REQUIRE(out.type == WIFI_NOW_CAN, "wifi now type");
	REQUIRE(out.uuid == 0xA11E, "wifi now uuid");
	REQUIRE(out.can_id == CAN_ID_DRIVE, "wifi now can id");
	REQUIRE(out.can_uuid == 0x1234, "wifi now can uuid");
	REQUIRE(out.size == 3, "wifi now size");
	REQUIRE(out.data[0] == 0x01 && out.data[2] == 0x03, "wifi now payload");
	REQUIRE(wifi_now_should_bridge(true, false), "bridge when no cable");
	REQUIRE(!wifi_now_should_bridge(true, true), "no bridge when cable");
	REQUIRE(!wifi_now_should_bridge(false, false), "no bridge without peer");
	REQUIRE(wifi_now_same_frame(
		out.can_id, out.can_uuid, out.data, out.size,
		CAN_ID_DRIVE, 0x1234, pkt.data, 3
	), "wifi now same frame");
	wire[0] = 0xFF;
	REQUIRE(!wifi_now_unpack(wire, WIFI_NOW_WIRE_SIZE, &out), "wifi now reject version");
	REQUIRE(wifi_now_pack(&pkt, wire, 8) == 0, "wifi now short buffer");

	printf("LocoCAN protocol host check ok (drive=0x%03X wifi-now=%u)\n",
		CAN_ID_DRIVE, WIFI_NOW_WIRE_SIZE);

	int controller_logic_check(void);
	if (controller_logic_check() != 0) {
		return 1;
	}

	return 0;
}
