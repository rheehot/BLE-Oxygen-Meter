#ifndef __UTIL_HPP__
#define __UTIL_HPP__

#include <Arduino.h>
#include <esp_task_wdt.h>
#include <soc/rtc_cntl_reg.h>
#include "esp_bt_device.h"
#include <inttypes.h>

#include "./config.hpp"
#include "./task/button.hpp"


//LCD 인스턴스
U8G2_SSD1327_WS_128X128_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/OLED_CS_PIN, /* dc=*/OLED_DC_PIN, /* reset=*/OLED_RESET_PIN);

LCD lcd(&u8g2);
BLE ble;

inline void sleep(uint64_t ms) {
	ESP_LOGI("Sleep", "Go To sleep... %llu ms", ms);
	WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 1);
	digitalWrite(RED_LED_PIN, HIGH);
	digitalWrite(GREEN_LED_PIN, HIGH);
	digitalWrite(MOTOR_PIN, LOW);
	digitalWrite(BUZZER_PIN, LOW);
	digitalWrite(POWER_HOLD_PIN, LOW);
	detachInterrupt(digitalPinToInterrupt(FUNCTION_BUTTON_PIN));
	detachInterrupt(digitalPinToInterrupt(POWER_BUTTON_PIN));
	ESP_ERROR_CHECK(rtc_gpio_set_level(POWER_HOLD_PIN, LOW));
	ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(POWER_BUTTON_PIN, LOW));
	ESP_ERROR_CHECK(esp_sleep_enable_ext1_wakeup(BIT64(FUNCTION_BUTTON_PIN), ESP_EXT1_WAKEUP_ANY_HIGH));
	ESP_ERROR_CHECK(esp_sleep_enable_timer_wakeup(ms * US_TO_MS_FACTOR));
	esp_deep_sleep_start();
}

char *barray2hexstr(uint8_t *data, size_t datalen) {
	size_t final_len = datalen * 2;
	char *chrs = (char *)malloc((final_len + 1) * sizeof(*chrs));
	unsigned int j = 0;
	for (j = 0; j < datalen; j++) {
		chrs[2 * j] = (data[j] >> 4) + 48;
		chrs[2 * j + 1] = (data[j] & 15) + 48;
		if (chrs[2 * j] > 57) chrs[2 * j] += 7;
		if (chrs[2 * j + 1] > 57) chrs[2 * j + 1] += 7;
	}
	chrs[2 * j] = '\0';
	return chrs;
}

void whyReset(){
	switch(esp_reset_reason()){
		case ESP_RST_BROWNOUT : {
			status.powerOn = false;
			sleep(1ull);
			break;
		}
		case ESP_RST_PANIC : {
			status.powerOn = true;
			sleep(1ull);
			break;
		}
		default : break;
	}
}

void waitPowerOn() {
	switch (esp_sleep_get_wakeup_cause()) {
		case ESP_SLEEP_WAKEUP_UNDEFINED: {
			ESP_LOGI(TAG, "Wakeup by undefined source");
			status.powerOn = true;
			break;
		}
		case ESP_SLEEP_WAKEUP_EXT0: {
			ESP_LOGI(TAG, "Wakeup by ext0 (Power Button)");
			__power_handler();
			break;
		}
		default : break;
	}
}
void whyWakeup() {
	if (status.wakeupCount == BROADCAST_INTERVAL_TIME) {
		status.bleOn = 1;
		status.wakeupCount = 0;
	} else {
		status.bleOn = 0;
	}
	if (status.bleOn) {
		ble.broadcast();
		ble.update(&(status.sensor));
	}
	pinMode(POWER_HOLD_PIN, OUTPUT);
	digitalWrite(POWER_HOLD_PIN, HIGH);
	switch (esp_sleep_get_wakeup_cause()) {
		case ESP_SLEEP_WAKEUP_UNDEFINED: {
			ESP_LOGI(TAG, "Wakeup by undefined source");
			//활성화 안돼있던거면 킨 직후임
			status.powerOn = true;
			break;
		}
		case ESP_SLEEP_WAKEUP_EXT0: {
			ESP_LOGI(TAG, "Wakeup by ext0 (Power Button)");
			__power_handler();
			break;
		}
		case ESP_SLEEP_WAKEUP_EXT1: {
			ESP_LOGI(TAG, "Wakeup by ext1 (Function Button)");
			__function_handler();
			break;
		}
		case ESP_SLEEP_WAKEUP_TIMER: {
			ESP_LOGI(TAG, "Wakeup by timer");
			break;
		}
		case ESP_SLEEP_WAKEUP_ULP: {
			ESP_LOGI(TAG, "Wakeup by ulp program");
			break;
		}
		default: {
			ESP_LOGI(TAG, "Wakeup by unknown source");
			break;
		}
	}
	double bat = (double)analogRead(BATTERY_ADC_PIN);
	bat = min((bat / 4096 * 3.3), 4096.0);
	digitalWrite(POWER_HOLD_PIN, LOW);
	ESP_LOGI("Battery", "%g V", (bat * 2));
	if (status.sensor.requestSos | status.sensor.warringO2) {
		lcd.print();
	} else {
		if ((status.wakeupCount % OLED_UPDATE_INTERVAL_TIME) == OLED_UPDATE_TIME) {
			lcd.print();
		}
	}
}


/**
 * @param uint8_t frequency 초에 몇번하는건지
 */
void alarm(const uint8_t frequency){
	ble.broadcast();
	ble.update(&(status.sensor));
	lcd.print();
	const uint64_t pendingTime = millis() / 1000;
	digitalWrite(MOTOR_PIN, pendingTime & 1 ? HIGH : LOW);
	ledcSetup(BUZZER_CHANNEL, BUZZER_FREQ, BUZZER_RESOLUTION);
	ledcAttachPin(BUZZER_PIN, BUZZER_CHANNEL);
	for (int i = 0; i < (frequency * 2); i++) {
		digitalWrite(RED_LED_PIN, (i & 1 ) ? LOW : HIGH);
		ledcWrite(BUZZER_CHANNEL, (i & 1) ? BUZZER_ON : BUZZER_OFF);
		delay(300 / frequency);
	}
	digitalWrite(RED_LED_PIN, HIGH);
	ledcWrite(BUZZER_CHANNEL, BUZZER_OFF);
}

template <typename T>
uint8_t toUint8(T value){
	return static_cast<uint8_t>(value);
}

#endif