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
	REQUIRE(MODULE_HEARTBEAT_TIMEOUT == 1000, "MODULE_HEARTBEAT_TIMEOUT");
	REQUIRE(CAN_ID_DRIVE_HEARTBEAT == 0x010, "CAN_ID_DRIVE_HEARTBEAT");
	REQUIRE(CAN_ID_TRAINEND_HEARTBEAT == 0x020, "CAN_ID_TRAINEND_HEARTBEAT");
	REQUIRE(CAN_ID_LOCO_SETUP == 0x0600, "CAN_ID_LOCO_SETUP");
	REQUIRE(CAN_ID_REQUEST == 0x7FF, "CAN_ID_REQUEST");
	REQUIRE(CAN_ID_REPLY == 0x780, "CAN_ID_REPLY");
	REQUIRE(CAN_ID_SETUP == 0x700, "CAN_ID_SETUP");
	REQUIRE(CAN_ID_SETUP_MODE == 0x7F8, "CAN_ID_SETUP_MODE");
	REQUIRE(CAN_SETUP_MODE_ENTER == 0xFE, "CAN_SETUP_MODE_ENTER");
	REQUIRE(CAN_SETUP_MODE_LEAVE == 0xFD, "CAN_SETUP_MODE_LEAVE");

	REQUIRE(CONTROL_MAINS_FLAG == 0, "CONTROL_MAINS_FLAG");
	REQUIRE(CONTROL_DRIVE_FLAG == 1, "CONTROL_DRIVE_FLAG");
	REQUIRE(CONTROL_DIR_FLAG == 2, "CONTROL_DIR_FLAG");
	REQUIRE(LIGHT_POSIT == 0, "LIGHT_POSIT");
	REQUIRE(LIGHT_MAIN == 7, "LIGHT_MAIN");
	REQUIRE(CAN_ID_MASK == 0x770, "CAN_ID_MASK");
	REQUIRE(LOCO_VERSION_MAJOR(LOCO_VERSION(2, 1, 12)) == 2, "version major");
	REQUIRE(LOCO_VERSION_MINOR(LOCO_VERSION(2, 1, 12)) == 1, "version minor");
	REQUIRE(LOCO_VERSION_PATCH(LOCO_VERSION(2, 1, 12)) == 12, "version patch");
	REQUIRE(LOCO_VERSION_CODE(LOCO_VERSION(2, 1, 12)) == 21, "version settings code");
	REQUIRE(LOCO_VERSION_IS(LOCO_VERSION(2, 1, 12), 2, 1), "patch keeps hardware");
	REQUIRE(!LOCO_VERSION_IS(LOCO_VERSION(2, 1, 12), 2, 0), "minor still selects hardware");

	REQUIRE(TYPE_CONTROLLER == 0x10, "TYPE_CONTROLLER");
	REQUIRE(TYPE_VEHICLE == 0x20, "TYPE_VEHICLE");
	REQUIRE(TYPE_SERVO == 0x21, "TYPE_SERVO");
	REQUIRE(TYPE_SWITCH == 0x22, "TYPE_SWITCH");
	REQUIRE(TYPE_DRIVE == 0x23, "TYPE_DRIVE");
	REQUIRE(TYPE_SENSOR == 0x30, "TYPE_SENSOR");
	REQUIRE(TYPE_PROGRAMMER == 0x70, "TYPE_PROGRAMMER");
	REQUIRE(TYPE_WIFI == 0x81, "TYPE_WIFI");
	REQUIRE(CONTROLLER_TYPE_ID == TYPE_CONTROLLER, "CONTROLLER_TYPE_ID");
	REQUIRE(VEHICLE_TYPE_ID == TYPE_VEHICLE, "VEHICLE_TYPE_ID");
	REQUIRE(SERVO_TYPE_ID == TYPE_SERVO, "SERVO_TYPE_ID");
	REQUIRE(SWITCH_TYPE_ID == TYPE_SWITCH, "SWITCH_TYPE_ID");
	REQUIRE(SENSOR_TYPE_ID == TYPE_SENSOR, "SENSOR_TYPE_ID");
	REQUIRE(PROGRAMMER_TYPE_ID == TYPE_PROGRAMMER, "PROGRAMMER_TYPE_ID");
	REQUIRE(WIFI_MODULE_TYPE == TYPE_WIFI, "WIFI_MODULE_TYPE");

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

	int vehicle_logic_check(void);
	if (vehicle_logic_check() != 0) {
		return 1;
	}

	int switch_logic_check(void);
	if (switch_logic_check() != 0) {
		return 1;
	}

	int servo_logic_check(void);
	if (servo_logic_check() != 0) {
		return 1;
	}

	int setup_mode_check(void);
	if (setup_mode_check() != 0) {
		return 1;
	}

	return 0;
}
