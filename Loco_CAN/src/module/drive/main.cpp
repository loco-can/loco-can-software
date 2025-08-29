/*
 * Loco-CAN DRIVE
 * 
 * @author: Thomas H Winkler
 * @copyright: 2025
 * @lizence: GG0
 *
 * Version 3.0
 *
 * The drive module is a 4q-motor controller with local and CAN control
 * ability. The local control works like a stand alone controller.
 * The modul has integrated sensors for battery voltage, motor voltage
 * and motor current.
 *
 * The module classes has to have the following methods:
 *		void begin(void)	// initializiation and startup
 *		void update(void)	// called in loop
 */

#include "config.h"
#include "module_drive.h"


/*
 * Loco-CAN function class includes
 */
#include "function_controller.h"
#include "function_vehicle.h"
#include "function_sensor.h"
#include "function_gauge.h"


/*
 * used additional class includes
 */
// LED dim library for ESP32
#ifdef BOARD_PLATFORM_ESP32
    #include "driver/ledc.h"
#endif


/* *************************************************
 * class initialisation
 */
void MODULE::begin(void) {

	// Serial.println("init module");
	pinMode(FORW, OUTPUT);
	pinMode(REV, OUTPUT);
	pinMode(BREAK, OUTPUT);

	// pinMode(PWM, OUTPUT);

	ledc_init();


	stop();
}


void MODULE::update(void) {

}


void MODULE::stop(void) {

	digitalWrite(FORW, LOW);
	digitalWrite(REV, LOW);
	digitalWrite(BREAK, LOW);
	digitalWrite(PWM, LOW);
}


void MODULE::forw(void) {

	// stop();
	digitalWrite(FORW, HIGH);
}


void MODULE::rev(void) {

	// stop();
	digitalWrite(REV, HIGH);
}


void MODULE::drive(void) {
	digitalWrite(BREAK, LOW);
}


void MODULE::brk(void) {
	digitalWrite(BREAK, HIGH);
}



void MODULE::set_pwm(uint16_t val) {
	// analogWrite(PWM, val);

	// Set duty to val
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, val));

    // Update duty to apply the new value
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
}


void MODULE::ledc_init(void) {

    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_MODE,
        .duty_resolution  = LEDC_DUTY_RES,
        .timer_num        = LEDC_TIMER,
        .freq_hz          = LEDC_FREQUENCY,  // Set output frequency at 4 kHz
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .gpio_num       = PWM,
        .speed_mode     = LEDC_MODE,
        .channel        = LEDC_CHANNEL,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER,
        .duty           = 0, // Set duty to 0%
        .hpoint         = 0
    };
    
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}