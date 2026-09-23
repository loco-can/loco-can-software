/*
 * Loco-CAN WIFI ESP-NOW wire format
 *
 * @author: Thomas H Winkler
 * @copyright: 2026
 * @lizence: GG0
 *
 * Packed so the same header can be checked on a host compiler.
 */
#pragma once

#ifndef WIFI_NOW_PACKET_H
#define WIFI_NOW_PACKET_H

#include <stdint.h>
#include <string.h>

#define WIFI_NOW_VERSION 1
#define WIFI_NOW_HELLO 1
#define WIFI_NOW_CAN 2

#define WIFI_NOW_FLAG_CABLE 0x01
#define WIFI_NOW_WIRE_SIZE 21

struct WIFI_NOW_PACKET {
	uint8_t type;
	uint8_t seq;
	uint16_t uuid;
	uint32_t can_id;
	uint16_t can_uuid;
	uint8_t size;
	uint8_t flags;
	uint8_t data[8];
};

static inline bool wifi_now_should_bridge(bool peer_ok, bool cable) {
	return peer_ok && !cable;
}

static inline bool wifi_now_same_frame(
	uint32_t id_a, uint16_t uuid_a, const uint8_t *data_a, uint8_t size_a,
	uint32_t id_b, uint16_t uuid_b, const uint8_t *data_b, uint8_t size_b
) {
	if (id_a != id_b || uuid_a != uuid_b || size_a != size_b) {
		return false;
	}
	if (size_a > 8) {
		return false;
	}
	if (size_a == 0) {
		return true;
	}
	return memcmp(data_a, data_b, size_a) == 0;
}

static inline uint8_t wifi_now_pack(const WIFI_NOW_PACKET *pkt, uint8_t *out, uint8_t out_max) {

	if (pkt == 0 || out == 0 || out_max < WIFI_NOW_WIRE_SIZE) {
		return 0;
	}
	if (pkt->size > 8) {
		return 0;
	}

	out[0] = WIFI_NOW_VERSION;
	out[1] = pkt->type;
	out[2] = pkt->seq;
	out[3] = (uint8_t)(pkt->uuid & 0xFF);
	out[4] = (uint8_t)(pkt->uuid >> 8);
	out[5] = (uint8_t)(pkt->can_id & 0xFF);
	out[6] = (uint8_t)((pkt->can_id >> 8) & 0xFF);
	out[7] = (uint8_t)((pkt->can_id >> 16) & 0xFF);
	out[8] = (uint8_t)((pkt->can_id >> 24) & 0xFF);
	out[9] = (uint8_t)(pkt->can_uuid & 0xFF);
	out[10] = (uint8_t)(pkt->can_uuid >> 8);
	out[11] = pkt->size;
	out[12] = pkt->flags;
	memset(out + 13, 0, 8);
	if (pkt->size > 0) {
		memcpy(out + 13, pkt->data, pkt->size);
	}
	return WIFI_NOW_WIRE_SIZE;
}

static inline bool wifi_now_unpack(const uint8_t *in, uint8_t len, WIFI_NOW_PACKET *pkt) {

	if (in == 0 || pkt == 0 || len < WIFI_NOW_WIRE_SIZE) {
		return false;
	}
	if (in[0] != WIFI_NOW_VERSION) {
		return false;
	}
	if (in[1] != WIFI_NOW_HELLO && in[1] != WIFI_NOW_CAN) {
		return false;
	}
	if (in[11] > 8) {
		return false;
	}

	memset(pkt, 0, sizeof(*pkt));
	pkt->type = in[1];
	pkt->seq = in[2];
	pkt->uuid = (uint16_t)in[3] | ((uint16_t)in[4] << 8);
	pkt->can_id = (uint32_t)in[5]
		| ((uint32_t)in[6] << 8)
		| ((uint32_t)in[7] << 16)
		| ((uint32_t)in[8] << 24);
	pkt->can_uuid = (uint16_t)in[9] | ((uint16_t)in[10] << 8);
	pkt->size = in[11];
	pkt->flags = in[12];
	memcpy(pkt->data, in + 13, 8);
	return true;
}

#endif
