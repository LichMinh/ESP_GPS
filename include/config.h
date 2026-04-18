#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// GPS Configuration
#define GPS_RX 16
#define GPS_TX 17
#define GPS_BAUD 9600

// SIM A7680C Configuration
#define SIM_RX 26
#define SIM_TX 27
#define SIM_BAUD 115200

// OLED pins
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_MOSI 23
#define OLED_CLK  18
#define OLED_DC   4
#define OLED_CS   5
#define OLED_RST  19
// MPU pins
#define MPU_SDA 21
#define MPU_SCL 22


// Home coordinates (Change home address)
const float HOME_LAT = 11.011160;
const float HOME_LON = 77.013080;

// Recipient phone number - Dùng extern để khai báo
static const char* MOBILE_NUMBER = "84387807105";

// Geofence radius (meters)
const float GEOFENCE_RADIUS = 50.0;

// Timeout settings
const unsigned long GPS_FIX_TIMEOUT = 30000;
const unsigned long SMS_CHECK_INTERVAL = 5000;

#endif