/*
 * Loco-CAN can class for ESP32
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 */

#include "../../config.h"

// use twai if is ESP32
#ifdef MODULE_ARCH_ESP32

	#include "ESP32_can.h"
	#include "ESP32-TWAI-CAN.hpp"
	#include "driver/gpio.h"
	#include "driver/rtc_io.h"
	#include "driver/rmt.h"
	#include "esp_log.h"
	#include "soc/gpio_sig_map.h"
	#include "soc/gpio_periph.h"
	#include "soc/gpio_struct.h"
	#include "soc/io_mux_reg.h"
	#include "soc/twai_struct.h"


	CAN_HANDLER::CAN_HANDLER() :
		_rxId(-1),
		_rxExtended(false),
		_rxLength(0),
		_rxIndex(0)
	{
		memset(_rxData, 0, sizeof(_rxData));
	}


	static bool s_listen_only = false;
	static bool s_twai_tx_ok = false;
	static int8_t s_rx_pin = -1;
	static int8_t s_tx_pin = -1;

	static void twai_rx_isolate(void);
	static void twai_bind_tx(int8_t tx);
	static void twai_recover_if_needed(void);
	static bool twai_install(long speed, int8_t tx, int8_t rx, twai_mode_t mode);

#ifdef PROGRAMMER_MODULE_VERSION
	/*
	 * ESP32-D0WD rev1 TWAI never accepts the live 500 kbit stream
	 * (rec sticks at 128, buserr ~15 k/s) even with GPIO matrix confirmed
	 * on CAN_RX. Decode the same pad with RMT edge timing instead.
	 */
	static const rmt_channel_t RMT_CAN_CH = RMT_CHANNEL_2;
	static const rmt_channel_t RMT_CAN_TX_CH = RMT_CHANNEL_0;
	static const uint32_t RMT_TICKS_BIT = 20; /* clk_div 8 → 100 ns, 500 kbit */
	static uint32_t s_rx_bit_ticks = RMT_TICKS_BIT;
	static RingbufHandle_t s_rmt_rb = NULL;
	static const uint8_t RMT_FRAME_Q = 24;
	static CanFrame s_rmt_q[RMT_FRAME_Q];
	static uint8_t s_rmt_q_head = 0;
	static uint8_t s_rmt_q_n = 0;
	static portMUX_TYPE s_rmt_mux = portMUX_INITIALIZER_UNLOCKED;
	static TaskHandle_t s_rmt_task = NULL;
	static bool s_rmt_tx_ok = false;
	static volatile uint32_t s_watch_id = 0;
	static volatile int s_watch_nstuffed = 0;
	static volatile int s_watch_ack = -1;

#ifndef GPIO_MATRIX_CONST_ONE_INPUT
	#define GPIO_MATRIX_CONST_ONE_INPUT 0x38
#endif

	static uint16_t can_crc15(const uint8_t *bits, int n) {
		uint16_t crc = 0;
		for (int i = 0; i < n; i++) {
			const uint16_t msb = (crc >> 14) & 1;
			crc = (uint16_t)((crc << 1) & 0x7FFF);
			if (msb ^ (bits[i] & 1)) {
				crc ^= 0x4599;
			}
		}
		return crc;
	}

	static bool rmt_items_to_frame(const rmt_item32_t *items, size_t nitem, CanFrame *out) {
		uint32_t dur[160];
		uint8_t lvl[160];
		size_t np = 0;
		uint32_t min_d = 0xFFFFFFFFu;

		for (size_t i = 0; i < nitem && np + 2 < 160; i++) {
			if (items[i].duration0) {
				dur[np] = items[i].duration0;
				lvl[np] = (uint8_t)items[i].level0;
				if (dur[np] > 4 && dur[np] < min_d) {
					min_d = dur[np];
				}
				np++;
			}
			if (items[i].duration1) {
				dur[np] = items[i].duration1;
				lvl[np] = (uint8_t)items[i].level1;
				if (dur[np] > 4 && dur[np] < min_d) {
					min_d = dur[np];
				}
				np++;
			}
		}
		if (np < 8 || min_d < 5 || min_d == 0xFFFFFFFFu) {
			return false;
		}

		uint8_t raw[200];
		int nraw = 0;
		for (size_t i = 0; i < np && nraw < 190; i++) {
			int nb = (int)((dur[i] + min_d / 2) / min_d);
			if (nb < 1) {
				nb = 1;
			}
			if (nb > 12) {
				nb = 12;
			}
			const uint8_t b = lvl[i] ? 1 : 0;
			while (nb-- && nraw < 190) {
				raw[nraw++] = b;
			}
		}
		if (nraw < 30) {
			return false;
		}

		int start = 0;
		while (start < nraw && raw[start] == 1) {
			start++;
		}
		if (start >= nraw || raw[start] != 0) {
			return false;
		}

		uint8_t dest[160];
		int nd = 0;
		int run = 0;
		int prev = -1;
		bool skip_stuff = false;
		for (int i = start; i < nraw && nd < 150; i++) {
			const int b = raw[i];
			if (skip_stuff) {
				if (b == prev) {
					/* Stuffing ends at the CRC delimiter (unstuffed 1s). */
					break;
				}
				skip_stuff = false;
				prev = b;
				run = 1;
				continue;
			}
			dest[nd++] = (uint8_t)b;
			if (b == prev) {
				run++;
			} else {
				run = 1;
				prev = b;
			}
			if (run == 5) {
				skip_stuff = true;
			}
		}
		if (nd < 32 || dest[0] != 0) {
			return false;
		}

		const int ide = dest[13];
		uint32_t id = 0;
		int rtr;
		int pos;
		if (ide) {
			for (int i = 1; i <= 11; i++) {
				id = (id << 1) | dest[i];
			}
			id <<= 18;
			uint32_t id18 = 0;
			for (int i = 14; i <= 31; i++) {
				id18 = (id18 << 1) | dest[i];
			}
			id |= id18;
			rtr = dest[32];
			pos = 35;
		} else {
			for (int i = 1; i <= 11; i++) {
				id = (id << 1) | dest[i];
			}
			rtr = dest[12];
			pos = 15;
		}

		if (pos + 4 > nd) {
			return false;
		}
		int dlc = 0;
		for (int i = 0; i < 4; i++) {
			dlc = (dlc << 1) | dest[pos++];
		}
		if (dlc > 8) {
			return false;
		}
		if (rtr) {
			dlc = 0;
		}

		uint8_t data[8];
		memset(data, 0, sizeof(data));
		for (int i = 0; i < dlc; i++) {
			if (pos + 8 > nd) {
				return false;
			}
			uint8_t v = 0;
			for (int j = 0; j < 8; j++) {
				v = (uint8_t)((v << 1) | dest[pos++]);
			}
			data[i] = v;
		}

		if (pos + 15 > nd) {
			return false;
		}
		const int crc_bits = pos;
		uint16_t rx_crc = 0;
		for (int i = 0; i < 15; i++) {
			rx_crc = (uint16_t)((rx_crc << 1) | dest[pos++]);
		}
		if (can_crc15(dest, crc_bits) != rx_crc) {
			return false;
		}

		if (min_d >= 8 && min_d <= 80) {
			s_rx_bit_ticks = min_d;
		}

		out->identifier = id;
		out->extd = ide ? 1 : 0;
		out->rtr = rtr ? 1 : 0;
		out->data_length_code = (uint8_t)dlc;
		memcpy(out->data, data, 8);
		return true;
	}

	static void rmt_q_push(const CanFrame &frame) {
		portENTER_CRITICAL(&s_rmt_mux);
		if (s_rmt_q_n >= RMT_FRAME_Q) {
			s_rmt_q_head = (uint8_t)((s_rmt_q_head + 1) % RMT_FRAME_Q);
			s_rmt_q_n--;
		}
		s_rmt_q[(s_rmt_q_head + s_rmt_q_n) % RMT_FRAME_Q] = frame;
		s_rmt_q_n++;
		portEXIT_CRITICAL(&s_rmt_mux);
	}

	static bool rmt_q_pop(CanFrame *out) {
		bool ok = false;
		portENTER_CRITICAL(&s_rmt_mux);
		if (s_rmt_q_n > 0) {
			*out = s_rmt_q[s_rmt_q_head];
			s_rmt_q_head = (uint8_t)((s_rmt_q_head + 1) % RMT_FRAME_Q);
			s_rmt_q_n--;
			ok = true;
		}
		portEXIT_CRITICAL(&s_rmt_mux);
		return ok;
	}

	static int rmt_expand_fixed(const rmt_item32_t *items, size_t nitem, uint8_t *raw, int max_raw) {
		int nraw = 0;
		for (size_t i = 0; i < nitem && nraw < max_raw; i++) {
			if (items[i].duration0) {
				int nb = (int)((items[i].duration0 + RMT_TICKS_BIT / 2) / RMT_TICKS_BIT);
				if (nb < 1) {
					nb = 1;
				}
				if (nb > 16) {
					nb = 16;
				}
				while (nb-- && nraw < max_raw) {
					raw[nraw++] = (uint8_t)items[i].level0;
				}
			}
			if (items[i].duration1 && nraw < max_raw) {
				int nb = (int)((items[i].duration1 + RMT_TICKS_BIT / 2) / RMT_TICKS_BIT);
				if (nb < 1) {
					nb = 1;
				}
				if (nb > 16) {
					nb = 16;
				}
				while (nb-- && nraw < max_raw) {
					raw[nraw++] = (uint8_t)items[i].level1;
				}
			}
		}
		return nraw;
	}

	static uint32_t rmt_peek_ext_id(const uint8_t *raw, int nraw, int start) {
		uint8_t dest[40];
		int nd = 0;
		int run = 0;
		int prev = -1;
		bool skip_stuff = false;
		for (int i = start; i < nraw && nd < 32; i++) {
			const int b = raw[i];
			if (skip_stuff) {
				if (b != prev) {
					skip_stuff = false;
					prev = b;
					run = 1;
					continue;
				}
				skip_stuff = false;
			}
			dest[nd++] = (uint8_t)b;
			if (b == prev) {
				run++;
			} else {
				run = 1;
				prev = b;
			}
			if (run == 5) {
				skip_stuff = true;
			}
		}
		if (nd < 32 || dest[0] != 0 || dest[13] == 0) {
			return 0;
		}
		uint32_t id = 0;
		for (int i = 1; i <= 11; i++) {
			id = (id << 1) | dest[i];
		}
		id <<= 18;
		for (int i = 14; i <= 31; i++) {
			id = (id << 1) | dest[i];
		}
		return id;
	}

	static void rmt_take_item(rmt_item32_t *items, size_t nitem) {
		rmt_item32_t copy[160];
		const size_t ncopy = nitem < 160 ? nitem : 160;
		memcpy(copy, items, ncopy * sizeof(rmt_item32_t));
		vRingbufferReturnItem(s_rmt_rb, items);

		if (s_watch_id != 0 && s_watch_nstuffed > 0) {
			uint8_t raw[200];
			const int nraw = rmt_expand_fixed(copy, ncopy, raw, 200);
			int start = 0;
			while (start < nraw && raw[start] == 1) {
				start++;
			}
			if (start < nraw && raw[start] == 0 && rmt_peek_ext_id(raw, nraw, start) == s_watch_id) {
				const int ack_i = start + s_watch_nstuffed + 1;
				if (ack_i < nraw) {
					s_watch_ack = raw[ack_i];
				}
			}
		}

		CanFrame tmp = { 0 };
		if (rmt_items_to_frame(copy, ncopy, &tmp)) {
			if (s_watch_id != 0 && tmp.identifier == s_watch_id && s_watch_nstuffed > 0) {
				uint8_t raw[200];
				const int nraw = rmt_expand_fixed(copy, ncopy, raw, 200);
				int start = 0;
				while (start < nraw && raw[start] == 1) {
					start++;
				}
				const int ack_i = start + s_watch_nstuffed + 1;
				if (start < nraw && ack_i < nraw) {
					s_watch_ack = raw[ack_i];
				}
			}
			rmt_q_push(tmp);
		}
	}

	static void rmt_drain_task(void *arg) {
		(void)arg;
		for (;;) {
			if (!s_rmt_rb) {
				vTaskDelay(1);
				continue;
			}
			size_t len = 0;
			rmt_item32_t *items = (rmt_item32_t *)xRingbufferReceive(s_rmt_rb, &len, pdMS_TO_TICKS(20));
			if (!items) {
				continue;
			}
			rmt_take_item(items, len / sizeof(rmt_item32_t));
			while (s_rmt_rb) {
				len = 0;
				items = (rmt_item32_t *)xRingbufferReceive(s_rmt_rb, &len, 0);
				if (!items) {
					break;
				}
				rmt_take_item(items, len / sizeof(rmt_item32_t));
			}
		}
	}

	static bool rmt_poll_frame(CanFrame *out) {
		return rmt_q_pop(out);
	}

	static bool rmt_can_start(int8_t rx) {
		(void)rmt_driver_uninstall(RMT_CAN_CH);
		s_rmt_rb = NULL;
		s_rmt_q_head = 0;
		s_rmt_q_n = 0;

		rmt_config_t cfg;
		memset(&cfg, 0, sizeof(cfg));
		cfg.rmt_mode = RMT_MODE_RX;
		cfg.channel = RMT_CAN_CH;
		cfg.gpio_num = (gpio_num_t)rx;
		cfg.clk_div = 8;
		cfg.mem_block_num = 4;
		cfg.rx_config.filter_en = true;
		cfg.rx_config.filter_ticks_thresh = 8;
		cfg.rx_config.idle_threshold = 300;

		if (rmt_config(&cfg) != ESP_OK) {
			return false;
		}
		esp_log_level_set("rmt", ESP_LOG_NONE);
		if (rmt_driver_install(RMT_CAN_CH, 32768, 0) != ESP_OK) {
			return false;
		}
		esp_log_level_set("rmt", ESP_LOG_NONE);
		if (rmt_get_ringbuf_handle(RMT_CAN_CH, &s_rmt_rb) != ESP_OK) {
			s_rmt_rb = NULL;
			return false;
		}
		if (rmt_rx_start(RMT_CAN_CH, true) != ESP_OK) {
			return false;
		}
		if (s_rmt_task == NULL) {
			xTaskCreatePinnedToCore(
				rmt_drain_task,
				"rmtcan",
				4096,
				NULL,
				configMAX_PRIORITIES - 3,
				&s_rmt_task,
				0
			);
		}
		return true;
	}

	static void rmt_bind_tx_pin(int8_t tx);

	static bool rmt_can_tx_start(int8_t tx) {
		if (tx < 0) {
			return false;
		}
		(void)rmt_driver_uninstall(RMT_CAN_TX_CH);

		rmt_config_t cfg;
		memset(&cfg, 0, sizeof(cfg));
		cfg.rmt_mode = RMT_MODE_TX;
		cfg.channel = RMT_CAN_TX_CH;
		cfg.gpio_num = (gpio_num_t)tx;
		cfg.clk_div = 8;
		cfg.mem_block_num = 2;
		cfg.tx_config.loop_en = false;
		cfg.tx_config.carrier_en = false;
		cfg.tx_config.idle_output_en = true;
		cfg.tx_config.idle_level = RMT_IDLE_LEVEL_HIGH;

		if (rmt_config(&cfg) != ESP_OK) {
			return false;
		}
		if (rmt_driver_install(RMT_CAN_TX_CH, 0, 0) != ESP_OK) {
			return false;
		}
		rmt_bind_tx_pin(tx);
		return true;
	}

	static void rmt_bind_tx_pin(int8_t tx) {
		if (tx < 0) {
			return;
		}
		const gpio_num_t pin = (gpio_num_t)tx;
		if (rtc_gpio_is_valid_gpio(pin)) {
			rtc_gpio_deinit(pin);
		}
		gpio_pad_select_gpio(pin);
		if (GPIO_PIN_MUX_REG[pin]) {
			PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin], PIN_FUNC_GPIO);
		}
		gpio_set_direction(pin, GPIO_MODE_OUTPUT);
		/* Matrix last: gpio_set_direction drops a prior RMT route. */
		gpio_matrix_out(
			(uint32_t)pin,
			(uint32_t)(RMT_SIG_OUT0_IDX + RMT_CAN_TX_CH),
			false,
			false
		);
	}

	/* Force transceiver TXD recessive if CAN_TX is wired to it. */
	static void analyzer_hold_tx_recessive(int8_t tx) {
		if (tx < 0) {
			return;
		}
		const gpio_num_t pin = (gpio_num_t)tx;
		gpio_reset_pin(pin);
		if (rtc_gpio_is_valid_gpio(pin)) {
			rtc_gpio_deinit(pin);
		}
		gpio_pad_select_gpio(pin);
		if (GPIO_PIN_MUX_REG[pin]) {
			PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin], PIN_FUNC_GPIO);
		}
		gpio_set_direction(pin, GPIO_MODE_OUTPUT);
		gpio_set_level(pin, 1);
		/* Matrix last: GPIO 17 defaults to U2TXD; GPIO.out only reaches the pad after this. */
		gpio_matrix_out((uint32_t)pin, SIG_GPIO_OUT_IDX, false, false);
		gpio_set_level(pin, 1);
	}

	static void stuff_bits(const uint8_t *in, int nin, uint8_t *out, int *nout) {
		int run = 0;
		int prev = -1;
		*nout = 0;
		for (int i = 0; i < nin && *nout < 250; i++) {
			const int bit = in[i];
			out[(*nout)++] = (uint8_t)bit;
			if (bit == prev) {
				run++;
			} else {
				run = 1;
				prev = bit;
			}
			if (run == 5) {
				const int stuff = 1 - bit;
				out[(*nout)++] = (uint8_t)stuff;
				run = 1;
				prev = stuff;
			}
		}
	}

	static int bits_to_rmt(const uint8_t *wire, int nwire, rmt_item32_t *items, int max_items) {
		int n = 0;
		int i = 0;
		while (i < nwire && n < max_items) {
			const uint8_t lvl0 = wire[i];
			int run0 = 0;
			while (i < nwire && wire[i] == lvl0) {
				run0++;
				i++;
			}
			items[n].level0 = lvl0;
			items[n].duration0 = (uint32_t)run0 * RMT_TICKS_BIT;
			if (i < nwire) {
				const uint8_t lvl1 = wire[i];
				int run1 = 0;
				while (i < nwire && wire[i] == lvl1) {
					run1++;
					i++;
				}
				items[n].level1 = lvl1;
				items[n].duration1 = (uint32_t)run1 * RMT_TICKS_BIT;
			} else {
				items[n].level1 = 1;
				items[n].duration1 = 0;
			}
			n++;
		}
		return n;
	}

	static bool rmt_q_has_frame(uint32_t identifier, const uint8_t *data, uint8_t dlc) {
		bool found = false;
		portENTER_CRITICAL(&s_rmt_mux);
		for (uint8_t i = 0; i < s_rmt_q_n; i++) {
			const CanFrame &frame = s_rmt_q[(s_rmt_q_head + i) % RMT_FRAME_Q];
			if (frame.identifier != identifier || frame.data_length_code != dlc) {
				continue;
			}
			uint8_t b = 0;
			while (b < dlc && frame.data[b] == data[b]) {
				b++;
			}
			if (b == dlc) {
				found = true;
				break;
			}
		}
		portEXIT_CRITICAL(&s_rmt_mux);
		return found;
	}

	static bool rmt_q_has_reply(uint32_t our_ext) {
		const uint32_t our_uuid = our_ext & 0xFFFFu;
		bool found = false;
		portENTER_CRITICAL(&s_rmt_mux);
		for (uint8_t i = 0; i < s_rmt_q_n; i++) {
			const CanFrame &frame = s_rmt_q[(s_rmt_q_head + i) % RMT_FRAME_Q];
			const uint32_t id11 = (frame.identifier >> 18) & 0x7FFu;
			const uint32_t uuid = frame.identifier & 0xFFFFu;
			if (id11 >= 0x780 && id11 <= 0x7FE && uuid != our_uuid) {
				found = true;
				break;
			}
		}
		portEXIT_CRITICAL(&s_rmt_mux);
		return found;
	}

	static bool programmer_wait_idle(int8_t rx) {
		const uint32_t need_us = 28;
		const uint32_t t0 = micros();
		uint32_t idle_from = t0;
		while ((uint32_t)(micros() - t0) < 2000) {
			if (gpio_get_level((gpio_num_t)rx) == 0) {
				idle_from = micros();
			} else if ((uint32_t)(micros() - idle_from) >= need_us) {
				return true;
			}
		}
		return gpio_get_level((gpio_num_t)rx) != 0;
	}

	static inline IRAM_ATTR uint32_t programmer_ccount(void) {
		uint32_t c;
		__asm__ __volatile__("rsr.ccount %0" : "=a"(c));
		return c;
	}

	/*
	 * RMT is shifting the frame; wait for SOF on RX then sample the ACK
	 * slot. Returns 1 if ACK dominant, 0 if SOF seen but ACK recessive,
	 * -1 if SOF never appeared.
	 */
	static IRAM_ATTR int programmer_sample_ack(
		uint32_t rx_mask,
		int idle_bits,
		int nstuffed,
		uint32_t cpb
	) {
		portDISABLE_INTERRUPTS();
		const uint32_t give_up = programmer_ccount() + cpb * (uint32_t)(idle_bits + 16);
		while ((int32_t)(programmer_ccount() - give_up) < 0 && (GPIO.in & rx_mask) == 0) {
		}
		while ((int32_t)(programmer_ccount() - give_up) < 0 && (GPIO.in & rx_mask) != 0) {
		}
		if ((GPIO.in & rx_mask) != 0) {
			portENABLE_INTERRUPTS();
			return -1;
		}
		const uint32_t t_sof = programmer_ccount();
		int seen_ack = 0;
		for (int i = 0; i <= 2; i++) {
			const uint32_t slot = (uint32_t)(nstuffed + i);
			const uint32_t sample =
				t_sof + slot * cpb + (cpb * 3) / 4;
			const uint32_t end = t_sof + (slot + 1) * cpb;
			while ((int32_t)(programmer_ccount() - sample) < 0) {
			}
			if ((GPIO.in & rx_mask) == 0) {
				if (i >= 1) {
					seen_ack = 1;
				}
			}
			while ((int32_t)(programmer_ccount() - end) < 0) {
				if (i >= 1 && (GPIO.in & rx_mask) == 0) {
					seen_ack = 1;
				}
			}
		}
		portENABLE_INTERRUPTS();
		return seen_ack;
	}

	static IRAM_ATTR bool programmer_gpio_tx(
		const uint8_t *wire,
		int nwire,
		uint32_t tx_mask,
		uint32_t rx_mask,
		uint32_t cpb,
		int ack_index,
		bool *loopback,
		bool *acked
	) {
		*loopback = false;
		*acked = false;
		portDISABLE_INTERRUPTS();
		uint32_t next = programmer_ccount();
		for (int i = 0; i < nwire; i++) {
			if (wire[i]) {
				GPIO.out_w1ts = tx_mask;
			} else {
				GPIO.out_w1tc = tx_mask;
			}
			next += cpb;
			const uint32_t mid = next - (cpb / 2);
			const uint32_t q3 = next - (cpb / 4);
			while ((int32_t)(programmer_ccount() - mid) < 0) {
			}
			if (!wire[i] && (GPIO.in & rx_mask) == 0) {
				*loopback = true;
			}
			while ((int32_t)(programmer_ccount() - q3) < 0) {
			}
			if (i == ack_index && (GPIO.in & rx_mask) == 0) {
				*acked = true;
			}
			while ((int32_t)(programmer_ccount() - next) < 0) {
				if (i == ack_index && (GPIO.in & rx_mask) == 0) {
					*acked = true;
				}
			}
		}
		GPIO.out_w1ts = tx_mask;
		portENABLE_INTERRUPTS();
		return true;
	}

	static bool programmer_bitbang_tx(const CAN_MESSAGE &message) {
		if (s_tx_pin < 0 || s_tx_pin >= 32 || s_rx_pin < 0 || s_rx_pin >= 32) {
			return false;
		}
		if (!s_rmt_tx_ok) {
			analyzer_hold_tx_recessive(s_tx_pin);
		}

		uint8_t raw[200];
		int nraw = 0;
		const uint32_t ext = ((uint32_t)message.id << 18) | (message.uuid & 0x3FFFF);
		const uint8_t dlc = message.size > 8 ? 8 : message.size;

		auto push = [&](int bit) {
			if (nraw < 200) {
				raw[nraw++] = (uint8_t)bit;
			}
		};

		push(0);
		for (int i = 10; i >= 0; i--) {
			push((int)((ext >> (18 + i)) & 1));
		}
		push(1);
		push(1);
		for (int i = 17; i >= 0; i--) {
			push((int)((ext >> i) & 1));
		}
		push(0);
		push(0);
		push(0);
		for (int i = 3; i >= 0; i--) {
			push((dlc >> i) & 1);
		}
		for (uint8_t b = 0; b < dlc; b++) {
			for (int i = 7; i >= 0; i--) {
				push((message.data[b] >> i) & 1);
			}
		}
		const uint16_t crc = can_crc15(raw, nraw);
		for (int i = 14; i >= 0; i--) {
			push((crc >> i) & 1);
		}

		uint8_t wire[256];
		uint8_t stuffed[256];
		int nstuffed = 0;
		stuff_bits(raw, nraw, stuffed, &nstuffed);
		int nwire = 0;
		const int idle_bits = 10;
		for (int i = 0; i < idle_bits && nwire < 256; i++) {
			wire[nwire++] = 1;
		}
		for (int i = 0; i < nstuffed && nwire < 256; i++) {
			wire[nwire++] = stuffed[i];
		}
		for (int i = 0; i < 13 && nwire < 256; i++) {
			wire[nwire++] = 1;
		}

		rmt_item32_t items[128];
		const int nitem = bits_to_rmt(wire, nwire, items, 128);
		if (nitem <= 0) {
			return false;
		}

		CanFrame encoded = { 0 };
		const bool encode_ok = rmt_items_to_frame(items, (size_t)nitem, &encoded);

		const uint32_t cpb = (uint32_t)getCpuFrequencyMhz() * s_rx_bit_ticks / 10;
		const uint32_t tx_mask = 1u << s_tx_pin;
		const uint32_t rx_mask = 1u << s_rx_pin;
		const int ack_index = idle_bits + nstuffed + 1;
		bool loopback = false;
		bool acked = false;
		int rmt_ack = -1;
		for (int attempt = 0; attempt < 8; attempt++) {
			(void)programmer_wait_idle(s_rx_pin);
			loopback = false;
			acked = false;
			s_watch_id = ext;
			s_watch_nstuffed = nstuffed;
			s_watch_ack = -1;

			auto wait_ack = [&]() {
				const uint32_t wait_end = millis() + 20;
				while ((int32_t)(wait_end - millis()) > 0) {
					if (s_watch_ack == 0 || rmt_q_has_reply(ext)) {
						return true;
					}
					delay(1);
				}
				rmt_ack = s_watch_ack;
				return false;
			};

			analyzer_hold_tx_recessive(s_tx_pin);
			s_watch_ack = -1;
			(void)programmer_gpio_tx(
				wire,
				nwire,
				tx_mask,
				rx_mask,
				cpb,
				ack_index,
				&loopback,
				&acked
			);
			if (acked || wait_ack()) {
				s_watch_id = 0;
				s_watch_nstuffed = 0;
				return true;
			}
			delayMicroseconds(200);
		}
		s_watch_id = 0;
		s_watch_nstuffed = 0;

		#ifdef DEBUG
			Serial.print("> CAN TX fail id=0x");
			Serial.print(message.id, HEX);
			Serial.print(" encode=");
			Serial.print(encode_ok ? 1 : 0);
			Serial.print(" ack=");
			Serial.print(acked ? 1 : 0);
			Serial.print(" rmtack=");
			Serial.print(rmt_ack);
			Serial.print(" loop=");
			Serial.print(loopback ? 1 : 0);
			Serial.print(" q=");
			Serial.println(s_rmt_q_n);
		#endif
		return false;
	}


	static uint32_t s_twai_bps = 500000;

	static bool twai_sync_bitrate(void) {
		const uint32_t want = (s_rx_bit_ticks >= 30) ? 250000u : 500000u;
		if (s_twai_tx_ok && want == s_twai_bps) {
			return true;
		}
		s_twai_bps = want;
		s_twai_tx_ok = twai_install((long)want, s_tx_pin, s_rx_pin, TWAI_MODE_NO_ACK);
		if (s_rx_pin >= 0) {
			(void)rmt_set_gpio(RMT_CAN_CH, RMT_MODE_RX, (gpio_num_t)s_rx_pin, false);
		}
		if (s_twai_tx_ok) {
			twai_bind_tx(s_tx_pin);
		}
		#ifdef DEBUG
			Serial.print("> TWAI TX ");
			Serial.print(want);
			Serial.print(" bps bit=");
			Serial.println(s_rx_bit_ticks);
		#endif
		return s_twai_tx_ok;
	}

	static bool programmer_twai_tx(const CAN_MESSAGE &message) {
		if (!twai_sync_bitrate()) {
			return false;
		}

		twai_recover_if_needed();
		twai_bind_tx(s_tx_pin);

		twai_status_info_t st;
		const uint32_t ready_end = millis() + 80;
		while ((int32_t)(ready_end - millis()) > 0) {
			if (twai_get_status_info(&st) == ESP_OK && st.state == TWAI_STATE_RUNNING) {
				break;
			}
			twai_recover_if_needed();
			delay(2);
		}
		if (twai_get_status_info(&st) != ESP_OK || st.state != TWAI_STATE_RUNNING) {
			#ifdef DEBUG
				Serial.print("> CAN TX fail twai state=");
				Serial.println(st.state);
			#endif
			return false;
		}

		CanFrame frame = { 0 };
		frame.identifier = ((uint32_t)message.id << 18) | (message.uuid & 0x3FFFF);
		frame.extd = 1;
		frame.rtr = 0;
		frame.ss = 1;
		frame.self = 0;
		frame.data_length_code = message.size;
		for (uint8_t i = 0; i < message.size; i++) {
			frame.data[i] = message.data[i];
		}

		s_watch_id = frame.identifier;
		s_watch_ack = -1;

		bool queued = false;
		for (int attempt = 0; attempt < 4 && !queued; attempt++) {
			(void)programmer_wait_idle(s_rx_pin);
			queued = ESP32Can.writeFrame(frame, 50);
			if (!queued) {
				twai_recover_if_needed();
				delay(2);
			}
		}
		if (!queued) {
			s_watch_id = 0;
			#ifdef DEBUG
				Serial.print("> CAN TX fail twai id=0x");
				Serial.print(message.id, HEX);
				Serial.print(" tec=");
				Serial.print(st.tx_error_counter);
				Serial.print(" rec=");
				Serial.println(st.rx_error_counter);
			#endif
			return false;
		}

		const uint32_t drain_end = millis() + 40;
		while ((int32_t)(drain_end - millis()) > 0) {
			if (twai_get_status_info(&st) == ESP_OK && st.msgs_to_tx == 0) {
				break;
			}
			delay(1);
		}

		const uint32_t wait_end = millis() + 20;
		while ((int32_t)(wait_end - millis()) > 0) {
			if (rmt_q_has_frame(frame.identifier, frame.data, frame.data_length_code)
				|| rmt_q_has_reply(frame.identifier)
				|| s_watch_ack == 0) {
				s_watch_id = 0;
				return true;
			}
			delay(1);
		}
		s_watch_id = 0;
		if (twai_get_status_info(&st) == ESP_OK && st.state == TWAI_STATE_RUNNING) {
			return true;
		}
		#ifdef DEBUG
			Serial.print("> CAN TX fail echo id=0x");
			Serial.print(message.id, HEX);
			Serial.print(" rmtack=");
			Serial.print(s_watch_ack);
			Serial.print(" q=");
			Serial.println(s_rmt_q_n);
		#endif
		return false;
	}
#endif


#ifndef GPIO_MATRIX_CONST_ONE_INPUT
	#define GPIO_MATRIX_CONST_ONE_INPUT 0x38
#endif

	static void twai_rx_isolate(void) {
		gpio_matrix_in(GPIO_MATRIX_CONST_ONE_INPUT, TWAI_RX_IDX, false);
	}


	static void twai_bind_tx(int8_t tx) {
		if (tx < 0) {
			return;
		}
		const gpio_num_t pin = (gpio_num_t)tx;
		gpio_reset_pin(pin);
		if (rtc_gpio_is_valid_gpio(pin)) {
			rtc_gpio_deinit(pin);
		}
		gpio_pad_select_gpio(pin);
		if (GPIO_PIN_MUX_REG[pin]) {
			PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin], PIN_FUNC_GPIO);
			PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin]);
		}
		gpio_set_direction(pin, GPIO_MODE_INPUT_OUTPUT);
		gpio_set_level(pin, 1);
		/* Matrix last: GPIO 17 defaults to U2TXD. */
		gpio_matrix_out((uint32_t)pin, TWAI_TX_IDX, false, false);
		/*
		 * Monitor own TXD, not CAN_RX. Isolated RX (constant 1) makes every
		 * dominant bit a bit error (tec→128, bus-off). NO_ACK still samples
		 * RX; looping the pad lets the engine see its own bits. MCP2515 ACK
		 * appears on CAN_RX and is decoded by RMT.
		 */
		gpio_matrix_in((uint32_t)pin, TWAI_RX_IDX, false);
	}


	static void twai_bind_rx(int8_t rx) {
		if (rx < 0) {
			return;
		}
		const gpio_num_t pin = (gpio_num_t)rx;
		if (rtc_gpio_is_valid_gpio(pin)) {
			rtc_gpio_deinit(pin);
		}
		gpio_pad_select_gpio(pin);
		if (GPIO_PIN_MUX_REG[pin]) {
			PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[pin], PIN_FUNC_GPIO);
			PIN_INPUT_ENABLE(GPIO_PIN_MUX_REG[pin]);
		}
		gpio_set_direction(pin, GPIO_MODE_INPUT);
		gpio_set_pull_mode(pin, GPIO_FLOATING);
		/* Matrix last: gpio_set_direction can drop a prior TWAI_RX route. */
		gpio_matrix_in((uint32_t)pin, TWAI_RX_IDX, false);
	}


	static bool twai_install(long speed, int8_t tx, int8_t rx, twai_mode_t mode) {

		s_listen_only = (mode == TWAI_MODE_LISTEN_ONLY);
		s_rx_pin = rx;

		(void)twai_stop();
		(void)twai_driver_uninstall();

		/*
		 * Programmer: NO_ACK + RX isolated. D0WD TWAI cannot lock the
		 * 500 kbit MCP2515 stream on RX (rec→128). TX still uses the
		 * hardware bit engine. RMT CH2 keeps GPIO CAN_RX.
		 */
		twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
			(gpio_num_t)tx,
			(gpio_num_t)rx,
			mode
		);
		g_config.tx_queue_len = (mode == TWAI_MODE_LISTEN_ONLY) ? 0 : 5;
		g_config.rx_queue_len = 16;
		g_config.alerts_enabled = TWAI_ALERT_NONE;

		twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
		if (speed == 250000 || speed == 250) {
			t_config = TWAI_TIMING_CONFIG_250KBITS();
		} else if (speed == 125000 || speed == 125) {
			t_config = TWAI_TIMING_CONFIG_125KBITS();
		}

		twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
		f_config.single_filter = true;

		if (twai_driver_install(&g_config, &t_config, &f_config) != ESP_OK) {
			return false;
		}
		if (mode == TWAI_MODE_NO_ACK) {
			twai_bind_tx(tx);
		} else {
			twai_bind_rx(rx);
		}
		if (twai_start() != ESP_OK) {
			twai_driver_uninstall();
			return false;
		}
		if (mode != TWAI_MODE_LISTEN_ONLY) {
			twai_bind_tx(tx);
		}

		#ifdef DEBUG
			if (mode == TWAI_MODE_LISTEN_ONLY) {
				Serial.println("> TWAI listen analyzer");
			} else if (mode == TWAI_MODE_NO_ACK) {
				Serial.println("> TWAI TX no-ack (RX isolated)");
			}
		#endif
		return true;
	}


	static void twai_recover_if_needed(void) {

		twai_status_info_t st;
		if (twai_get_status_info(&st) != ESP_OK) {
			return;
		}

		if (st.state == TWAI_STATE_BUS_OFF) {
			#ifdef DEBUG
				static uint32_t last_print = 0;
				uint32_t now = millis();
				if (now - last_print > 2000) {
					last_print = now;
					Serial.print("> TWAI bus-off tec=");
					Serial.print(st.tx_error_counter);
					Serial.print(" rec=");
					Serial.print(st.rx_error_counter);
					Serial.print(" txq=");
					Serial.println(st.msgs_to_tx);
				}
			#endif
			twai_clear_transmit_queue();
			twai_initiate_recovery();
		}
		else if (st.state == TWAI_STATE_STOPPED) {
			twai_clear_transmit_queue();
			if (twai_start() == ESP_OK) {
				#ifdef DEBUG
					Serial.println("> TWAI recovered");
				#endif
			}
		}
	}


	bool CAN_HANDLER::begin(long speed, uint16_t can_rx, uint16_t can_tx) {

		s_rx_pin = (int8_t)can_rx;
		s_tx_pin = (int8_t)can_tx;

#ifdef PROGRAMMER_MODULE_VERSION
		/*
		 * D0WD rev1 TWAI cannot lock the MCP2515 500 kbit stream on RX.
		 * RMT CH2 decodes CAN_RX. TWAI NO_ACK drives CAN_TX so GPIO 17
		 * is not left as UART2 and the bit time is the hardware engine.
		 */
		s_listen_only = false;
		s_rmt_tx_ok = false;
		s_twai_tx_ok = false;
		(void)twai_stop();
		(void)twai_driver_uninstall();
		const bool rmt_ok = rmt_can_start(s_rx_pin);
		s_twai_tx_ok = twai_install(speed, s_tx_pin, s_rx_pin, TWAI_MODE_NO_ACK);
		if (s_rx_pin >= 0) {
			(void)rmt_set_gpio(RMT_CAN_CH, RMT_MODE_RX, (gpio_num_t)s_rx_pin, false);
		}
		if (s_twai_tx_ok) {
			twai_bind_tx(s_tx_pin);
		} else {
			analyzer_hold_tx_recessive(s_tx_pin);
		}
		(void)&rmt_can_tx_start;
		(void)&rmt_bind_tx_pin;
		(void)&programmer_sample_ack;
		if (rmt_ok) {
			#ifdef DEBUG
				Serial.println("CAN bus started!");
				if (s_twai_tx_ok) {
					Serial.println("> RMT RX + TWAI TX no-ack (transceiver TXD to GPIO 17)");
				} else {
					Serial.println("> RMT RX + GPIO TX (transceiver TXD to GPIO 17)");
				}
			#endif
			return true;
		}
		#ifdef DEBUG
			Serial.println("CAN bus failed!");
		#endif
		return false;
#else
		if (twai_install(speed, s_tx_pin, s_rx_pin, TWAI_MODE_NORMAL)) {
			#ifdef DEBUG
				Serial.println("CAN bus started!");
			#endif
			return true;
		}

		#ifdef DEBUG
			Serial.println("CAN bus failed!");
		#endif
		return false;
#endif
	}


	bool CAN_HANDLER::available(void) {
		return _rxIndex < _rxLength;
	}


	uint16_t CAN_HANDLER::parsePacket(void) {

#ifndef PROGRAMMER_MODULE_VERSION
		twai_recover_if_needed();
#endif

		CanFrame frame = { 0 };
		bool got = false;

#ifdef PROGRAMMER_MODULE_VERSION
		got = rmt_poll_frame(&frame);
		#ifdef DEBUG
			if (got) {
				const uint32_t id11 = (frame.identifier >> 18) & 0x7FFu;
				if (id11 >= 0x700 || id11 == 0x510) {
					static uint32_t last_hb_log = 0;
					const uint32_t now = millis();
					if (id11 >= 0x700 || (now - last_hb_log) > 2000) {
						if (id11 == 0x510) {
							last_hb_log = now;
						}
						Serial.print("> CAN RX id=0x");
						Serial.print(id11, HEX);
						Serial.print(" u=");
						Serial.print(frame.identifier & 0xFFFFu, HEX);
						Serial.print(" n=");
						Serial.print(frame.data_length_code);
						Serial.print(" bit=");
						Serial.println(s_rx_bit_ticks);
					}
				}
			}
		#endif
#else
		/* 1 ms, not 0: some IDF builds never dequeue with a 0-tick wait. */
		got = ESP32Can.readFrame(frame, 1);
#endif

		if (!got) {
			_rxId = -1;
			_rxExtended = false;
			_rxLength = 0;
			_rxIndex = 0;
			return 0;
		}

		_rxId = frame.identifier;
		_rxExtended = frame.extd;
		_rxLength = frame.data_length_code;
		_rxIndex = 0;

		if (_rxLength > 8) {
			_rxLength = 8;
		}

		for (uint8_t i = 0; i < _rxLength; i++) {
			_rxData[i] = frame.data[i];
		}

		return _rxLength;
	}


	bool CAN_HANDLER::packetExtended(void) {
		return _rxExtended;
	}


	long CAN_HANDLER::packetId(void) {
		return _rxId;
	}


	bool CAN_HANDLER::send(CAN_MESSAGE message) {

		if (s_listen_only) {
			return false;
		}

		if (message.size > 8) {
			return false;
		}

#ifdef PROGRAMMER_MODULE_VERSION
		if (s_twai_tx_ok) {
			return programmer_twai_tx(message);
		}
		return programmer_bitbang_tx(message);
#else
		twai_recover_if_needed();

		twai_status_info_t st;
		if (twai_get_status_info(&st) != ESP_OK || st.state != TWAI_STATE_RUNNING) {
			return false;
		}

		CanFrame frame = { 0 };
		frame.identifier = ((uint32_t)message.id << 18) | (message.uuid & 0x3FFFF);
		frame.extd = 1;
		frame.data_length_code = message.size;

		for (uint8_t i = 0; i < message.size; i++) {
			frame.data[i] = message.data[i];
		}

		return ESP32Can.writeFrame(frame, 50);
#endif
	}


	uint8_t CAN_HANDLER::read(void) {

		if (_rxIndex >= _rxLength) {
			return 0;
		}

		return _rxData[_rxIndex++];
	}


	void loco_twai_rebind_rx(void) {
#ifdef PROGRAMMER_MODULE_VERSION
		if (s_rx_pin >= 0) {
			(void)rmt_set_gpio(RMT_CAN_CH, RMT_MODE_RX, (gpio_num_t)s_rx_pin, false);
		}
		if (s_twai_tx_ok) {
			twai_bind_tx(s_tx_pin);
		}
#else
		twai_bind_rx(s_rx_pin);
#endif
	}


#endif
