#include <Arduino.h>
#include <HardwareSerial.h>
#include "config.h"
#include "gps_manager.h"
#include "sim_manager.h"
#include "sms_handler.h"

// Serial objects
HardwareSerial gpsSerial(2);
HardwareSerial simSerial(1);

// Global objects
GPSManager gps;
SIMManager simManager; 
SMSHandler smsHandler;

bool processingCommand = false;
unsigned long lastSMSCheck = 0;

// Calculate distance between two coordinates
double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    double R = 6371000; // Earth radius in meters
    double dLat = (lat2 - lat1) * PI / 180.0;
    double dLon = (lon2 - lon1) * PI / 180.0;
    double a = sin(dLat / 2) * sin(dLat / 2) +
               cos(lat1 * PI / 180.0) * cos(lat2 * PI / 180.0) *
               sin(dLon / 2) * sin(dLon / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return R * c;
}

// Process location request
void processLocation() {
    Serial.println("\n[CMD] Processing LOCATION request...");
    
    gps.powerOn();
    
    float lat, lng;
    int sats;
    unsigned long startTime = millis();
    
    Serial.println("[GPS] Waiting for fix...");
    
    while (!gps.hasFix() && millis() - startTime < GPS_FIX_TIMEOUT) {
        gps.process();
        delay(100);
    }
    
    if (gps.getLocation(lat, lng, sats)) {
        String message = "CURRENT LOCATION\n";
        message += "Lat: " + String(lat, 6) + "\n";
        message += "Lon: " + String(lng, 6) + "\n";
        message += "Sats: " + String(sats) + "\n";
        message += "https://maps.google.com/?q=" + String(lat, 6) + "," + String(lng, 6);
        simManager.sendSMS(message);
    } else {
        simManager.sendSMS("Cannot get GPS fix.\nPlease move to open area.");
    }
    
    gps.powerOff();
    processingCommand = false;
}

// Process status request
void processStatus() {
    Serial.println("\n[CMD] Processing STATUS request...");
    
    gps.powerOn();
    
    float lat, lng;
    int sats;
    unsigned long startTime = millis();
    
    while (!gps.hasFix() && millis() - startTime < GPS_FIX_TIMEOUT) {
        gps.process();
        delay(100);
    }
    
    if (gps.getLocation(lat, lng, sats)) {
        double distance = calculateDistance(HOME_LAT, HOME_LON, lat, lng);
        
        String message = "GPS: " + String(sats) + " sats\n";
        message += "📏 Distance: " + String(distance, 0) + "m\n";
        message += (distance > GEOFENCE_RADIUS) ? "OUTSIDE safe zone" : "INSIDE safe zone";
        
        simManager.sendSMS(message);
    } else {
        simManager.sendSMS("GPS: NO FIX");
    }
    
    gps.powerOff();
    processingCommand = false;
}

// Command callback from SMS handler
void onCommandReceived(const String& cmd) {
    if (processingCommand) return;
    
    processingCommand = true;
    
    if (cmd == "LOCATION") {
        processLocation();
    } else if (cmd == "STATUS") {
        processStatus();
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println("     GPS TRACKER - POWER SAVING MODE     ");
    
    // Initialize SIM
    simSerial.begin(SIM_BAUD, SERIAL_8N1, SIM_RX, SIM_TX);
    if (!simManager.begin()) {
        Serial.println("\n[ERROR] SIM initialization failed!");
        Serial.println("Check:");
        Serial.println("  - Power supply (5V/2A)");
        Serial.println("  - SIM card");
        Serial.println("  - Antenna");
        Serial.println("  - Wiring (ESP32 TX27→SIM RX, RX26→SIM TX)");
        while(1) { delay(1000); }
    }
    
    // Initialize GPS (off by default)
    gps.powerOff();
    
    // Setup SMS handler
    smsHandler.registerCallback(onCommandReceived);
    
    Serial.println("     SYSTEM READY - POWER SAVING MODE    ");
    Serial.println("                                         ");
    Serial.println("GPS is OFF by default");
    Serial.println("GPS turns ON only when requested");
    Serial.println("\nCommands (send SMS to this device):");
    Serial.println("  'location' → Get current position");
    Serial.println("  'status'   → Check safe zone status");
    Serial.println("  'help'     → Show instructions");
    
    lastSMSCheck = millis();
}

void loop() {
    // Process GPS if active
    if (gps.isRunning()) {
        gps.process();
    }
    
    // Check for new SMS periodically
    if (simManager.isSimReady() && (millis() - lastSMSCheck >= SMS_CHECK_INTERVAL)) {
        String smsContent = simManager.checkNewSMS();
        if (smsContent.length() > 0) {
            smsHandler.processCommand(smsContent);
            simManager.deleteAllSMS();
        }
        lastSMSCheck = millis();
    }
    
    delay(100);
}
// end of main.cpp....