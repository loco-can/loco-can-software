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


	CAN_HANDLER::CAN_HANDLER() :
		_rxId(-1),
		_rxExtended(false),
		_rxLength(0),
		_rxIndex(0)
	{
		memset(_rxData, 0, sizeof(_rxData));
	}


	static bool s_listen_only = false;


	static bool twai_install(long speed, int8_t tx, int8_t rx, bool listen_only) {

		s_listen_only = listen_only;

		const TwaiSpeed twai_speed = ESP32Can.convertSpeed(speed);

		twai_general_config_t g_config = {
			.mode = listen_only ? TWAI_MODE_LISTEN_ONLY : TWAI_MODE_NORMAL,
			.tx_io = (gpio_num_t)tx,
			.rx_io = (gpio_num_t)rx,
			.clkout_io = TWAI_IO_UNUSED,
			.bus_off_io = TWAI_IO_UNUSED,
			.tx_queue_len = 5,
			.rx_queue_len = 16,
			.alerts_enabled = TWAI_ALERT_NONE,
			.clkout_divider = 0,
			.intr_flags = ESP_INTR_FLAG_LEVEL1
		};

		if (!ESP32Can.begin(twai_speed, tx, rx, 5, 16, nullptr, &g_config)) {
			return false;
		}

		#ifdef DEBUG
			if (listen_only) {
				Serial.println("> TWAI listen-only (no TX until the bus ACKs)");
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

		/*
		 * Programmer is an analyzer: a heartbeat with no ACK takes the
		 * controller bus-off and receive stops. Listen-only keeps RX up.
		 */
		#ifdef PROGRAMMER_MODULE_VERSION
			const bool listen_only = true;
		#else
			const bool listen_only = false;
		#endif

		if (twai_install(speed, (int8_t)can_tx, (int8_t)can_rx, listen_only)) {
			#ifdef DEBUG
				Serial.println("CAN bus started!");
			#endif
			return true;
		}

		#ifdef DEBUG
			Serial.println("CAN bus failed!");
		#endif
		return false;
	}


	bool CAN_HANDLER::available(void) {
		return _rxIndex < _rxLength;
	}


	uint16_t CAN_HANDLER::parsePacket(void) {

		twai_recover_if_needed();

		CanFrame frame = { 0 };

		/* 1 ms, not 0: some IDF builds never dequeue with a 0-tick wait. */
		if (!ESP32Can.readFrame(frame, 1)) {
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


#endif
