#ifndef __BUTTON_TASK_HPP__
#define __BUTTON_TASK_HPP__
#include "../config.hpp"
#include "../type.hpp"
#include "../ble.hpp"
#include <driver/rtc_io.h>
#include <driver/uart.h>
#include <esp_sleep.h>
#include <soc/uart_channel.h>

static uint64_t lastHandle;
void IRAM_ATTR __function_handler() {
	if((millis() - lastHandle) > 1){
		lastHandle = millis();
		status.sosEnable = !status.sosEnable;
		ESP_LOGI(TAG, "Alarm %s", status.sosEnable ? "ON" : "OFF");
		while(digitalRead(FUNCTION_BUTTON_PIN) != HIGH);
	}
	lastHandle = millis();
}

void IRAM_ATTR __power_handler() {
	//어차피 꺼질때만 실행 될 것
	uint64_t start = millis(); 
	while(digitalRead(POWER_BUTTON_PIN) != LOW);
	uint64_t end = millis();
	ESP_LOGI(TAG, "power pin pressed start : %llums", start);
	ESP_LOGI(TAG, "power pin pressed end : %llums", end);
	ESP_LOGI(TAG, "power pin pressed : %llums", end - start);
	if(end - start >= POWER_FLAG_THRESHOLD){
		status.powerOn = !status.powerOn;
		ESP_LOGI(TAG, "Power %s", status.powerOn ? "On" : "Off");
	}else{
		ESP_LOGI(TAG, "Power no change");
	}
}

#endif