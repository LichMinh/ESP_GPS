#include "gps_manager.h"
#include <HardwareSerial.h>

extern HardwareSerial gpsSerial;


GPSManager::GPSManager() {
    isActive = false;
    startTime = 0;
}

void GPSManager::begin() {
    if (isActive) return;
    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, GPS_RX, GPS_TX);
    delay(500);
    isActive = true;
    startTime = millis();
    Serial.println("[GPS] Powered ON");
}

void GPSManager::end() {
    if (!isActive) return;
    gpsSerial.end();
    isActive = false;
    Serial.println("[GPS] Powered OFF");
}

void GPSManager::process() {
    if (!isActive) return;
    while (gpsSerial.available() > 0) {
        gps.encode(gpsSerial.read());
    }
}

bool GPSManager::hasFix() {
    return gps.location.isValid() && gps.location.age() < 2000;
}

bool GPSManager::getLocation(float &lat, float &lng, int &sats) {
    if (!hasFix()) return false;
    
    lat = gps.location.lat();
    lng = gps.location.lng();
    sats = gps.satellites.value();
    return true;
}

void GPSManager::powerOn() {
    begin();
}

void GPSManager::powerOff() {
    end();
}

bool GPSManager::isTimeout(unsigned long timeout) {
    return (millis() - startTime) > timeout;
}