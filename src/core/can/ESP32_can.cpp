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
	static int8_t s_rx_pin = -1;
	static int8_t s_tx_pin = -1;

#ifdef PROGRAMMER_MODULE_VERSION
	/*
	 * ESP32-D0WD rev1 TWAI never accepts the live 500 kbit stream
	 * (rec sticks at 128, buserr ~15 k/s) even with GPIO matrix confirmed
	 * on CAN_RX. Decode the same pad with RMT edge timing instead.
	 */
	static const rmt_channel_t RMT_CAN_CH = RMT_CHANNEL_2;
	static RingbufHandle_t s_rmt_rb = NULL;

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
					return false;
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

		out->identifier = id;
		out->extd = ide ? 1 : 0;
		out->rtr = rtr ? 1 : 0;
		out->data_length_code = (uint8_t)dlc;
		memcpy(out->data, data, 8);
		return true;
	}

	static bool rmt_poll_frame(CanFrame *out) {
		if (!s_rmt_rb) {
			return false;
		}
		size_t len = 0;
		rmt_item32_t *items = (rmt_item32_t *)xRingbufferReceive(s_rmt_rb, &len, 0);
		if (!items) {
			return false;
		}
		const bool ok = rmt_items_to_frame(items, len / sizeof(rmt_item32_t), out);
		vRingbufferReturnItem(s_rmt_rb, items);
		return ok;
	}

	static bool rmt_can_start(int8_t rx) {
		(void)rmt_driver_uninstall(RMT_CAN_CH);
		s_rmt_rb = NULL;

		rmt_config_t cfg;
		memset(&cfg, 0, sizeof(cfg));
		cfg.rmt_mode = RMT_MODE_RX;
		cfg.channel = RMT_CAN_CH;
		cfg.gpio_num = (gpio_num_t)rx;
		cfg.clk_div = 8;
		cfg.mem_block_num = 2;
		cfg.rx_config.filter_en = true;
		cfg.rx_config.filter_ticks_thresh = 8;
		cfg.rx_config.idle_threshold = 300;

		if (rmt_config(&cfg) != ESP_OK) {
			return false;
		}
		esp_log_level_set("rmt", ESP_LOG_NONE);
		if (rmt_driver_install(RMT_CAN_CH, 8192, 0) != ESP_OK) {
			return false;
		}
		if (rmt_get_ringbuf_handle(RMT_CAN_CH, &s_rmt_rb) != ESP_OK) {
			s_rmt_rb = NULL;
			return false;
		}
		return rmt_rx_start(RMT_CAN_CH, true) == ESP_OK;
	}

	/* Force transceiver TXD recessive if CAN_TX is wired to it. */
	static void analyzer_hold_tx_recessive(int8_t tx) {
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
		gpio_matrix_out((uint32_t)pin, SIG_GPIO_OUT_IDX, false, false);
		gpio_set_level(pin, 1);
		gpio_set_direction(pin, GPIO_MODE_OUTPUT);
		gpio_set_level(pin, 1);
	}
#endif


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


	static bool twai_install(long speed, int8_t tx, int8_t rx, bool listen_only) {

		s_listen_only = listen_only;
		s_rx_pin = rx;

		(void)twai_stop();
		(void)twai_driver_uninstall();

		/*
		 * ESP32-D0WD rev1: listen-only freezes REC and drops frames at
		 * the ACK slot. NO_ACK/self-test expects a recessive ACK, so the
		 * other nodes' ACK becomes a receive error (rec=128, buserr++).
		 * NORMAL expects ACK; the other modules already provide it.
		 * Leave transceiver TXD unconnected so GPIO CAN_TX cannot drive
		 * the bus if the bit engine emits ACK or error flags.
		 */
		twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(
			(gpio_num_t)tx,
			(gpio_num_t)rx,
			TWAI_MODE_NORMAL
		);
		g_config.tx_queue_len = listen_only ? 0 : 5;
		g_config.rx_queue_len = 16;
		g_config.alerts_enabled = TWAI_ALERT_NONE;

		/* IDF 80 % sample point (20 tq). 56 % MCP2515 match still error-passive. */
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
		twai_bind_rx(rx);
		if (twai_start() != ESP_OK) {
			twai_driver_uninstall();
			return false;
		}

		#ifdef DEBUG
			if (listen_only) {
				Serial.println("> TWAI listen analyzer");
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
		 * Do not start TWAI. D0WD rev1 injects error flags on CAN_TX in
		 * every mode, and other Loco-CAN nodes then report bus errors.
		 * Hold TX recessive and decode RXD with RMT only.
		 */
		(void)speed;
		s_listen_only = true;
		analyzer_hold_tx_recessive(s_tx_pin);
		twai_bind_rx(s_rx_pin);
		const bool rmt_ok = rmt_can_start(s_rx_pin);
		analyzer_hold_tx_recessive(s_tx_pin);
		(void)rmt_set_gpio(RMT_CAN_CH, RMT_MODE_RX, (gpio_num_t)s_rx_pin, false);
		if (rmt_ok) {
			#ifdef DEBUG
				Serial.println("CAN bus started!");
			#endif
			return true;
		}
		#ifdef DEBUG
			Serial.println("CAN bus failed!");
		#endif
		return false;
#else
		if (twai_install(speed, s_tx_pin, s_rx_pin, false)) {
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

		/* 0 = non-blocking: a silent bus must not stall the HTTP server */
		return ESP32Can.writeFrame(frame, 0);
	}


	uint8_t CAN_HANDLER::read(void) {

		if (_rxIndex >= _rxLength) {
			return 0;
		}

		return _rxData[_rxIndex++];
	}


	void loco_twai_rebind_rx(void) {
#ifdef PROGRAMMER_MODULE_VERSION
		analyzer_hold_tx_recessive(s_tx_pin);
		twai_bind_rx(s_rx_pin);
		if (s_rx_pin >= 0) {
			(void)rmt_set_gpio(RMT_CAN_CH, RMT_MODE_RX, (gpio_num_t)s_rx_pin, false);
		}
#else
		twai_bind_rx(s_rx_pin);
#endif
	}


#endif
