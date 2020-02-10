#include "./config.hpp"
#include "./type.hpp"
#include "./lcd.hpp"
#include "./communication.hpp"
#include "./task/button.hpp"
#include "./task/sensor.hpp"

#include <Arduino.h>
#include <HardwareSerial.cpp>
#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

//계속 유지되야 하는 데이터들
//LCD 인스턴스 
RTC_DATA_ATTR U8G2_SSD1327_WS_128X128_F_4W_HW_SPI u8g2(U8G2_R0, /* cs=*/14, /* dc=*/5, /* reset=*/19);
//센서 데이터
RTC_DATA_ATTR sensor_t sensor;
//디바이스 정보
RTC_DATA_ATTR device_status_t status;

void setup() {
	status.wakeCount++;
	LCD lcd(&u8g2);
	lcd.begin();
	randomSeed(analogRead(0));
	sensor.barometric = random(2000);
	sensor.temp = (float)random(100000) / 100.0;
	sensor.o2 = (float)random(100000) / 100.0;
	sensor.ppO2 = random(1000);
	lcd.print(&sensor);
	pinMode(BUZZER_PIN, OUTPUT);
	pinMode(MOTOR_PIN, OUTPUT);
	ESP_LOGD(TAG, "enter deep sleep %d", status.wakeCount);
	esp_sleep_enable_timer_wakeup(2 * mS_TO_S_FACTOR);
	esp_deep_sleep_start();
}

void loop() {
}