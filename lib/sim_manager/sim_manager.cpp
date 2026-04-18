#include "sim_manager.h"
#include <HardwareSerial.h>

extern HardwareSerial simSerial;

SIMManager::SIMManager() {
    isReady = false;
}

String SIMManager::sendCommand(String command, int timeout, bool debug) {
    clearBuffer();
    simSerial.println(command);
    delay(150);
    
    String response = "";
    unsigned long startTime = millis();
    
    while (millis() - startTime < timeout) {
        while (simSerial.available()) {
            char c = simSerial.read();
            response += c;
        }
    }
    
    if (debug && response.length() > 0) {
        Serial.print("[AT] ");
        Serial.print(command);
        Serial.print(" -> ");
        Serial.println(response);
    }
    
    return response;
}

void SIMManager::clearBuffer() {
    while (simSerial.available()) {
        simSerial.read();
    }
}

bool SIMManager::begin() {
    Serial.println("\n==========================================");
    Serial.println("  INITIALIZING SIM A7680C MODULE");
    Serial.println("==========================================");
    
    // Test AT
    Serial.print("[1] Testing AT... ");
    String response = sendCommand("AT", 2000);
    if (response.indexOf("OK") == -1) {
        Serial.println("FAILED");
        return false;
    }
    Serial.println("OK");
    
    // Check SIM
    Serial.print("[2] Checking SIM... ");
    response = sendCommand("AT+CPIN?", 2000);
    if (response.indexOf("READY") == -1) {
        Serial.println("FAILED");
        return false;
    }
    Serial.println("OK");
    
    // Signal quality
    Serial.print("[3] Signal... ");
    response = sendCommand("AT+CSQ", 2000);
    if (response.indexOf("CSQ:") != -1) {
        Serial.println("OK");
    } else {
        Serial.println("WEAK");
    }
    
    // Network registration
    Serial.print("[4] Network... ");
    response = sendCommand("AT+CREG?", 2000);
    if (response.indexOf("+CREG:0,1") != -1 || response.indexOf("+CREG:0,5") != -1) {
        Serial.println("Registered");
    } else {
        Serial.println("Not registered");
    }
    
    // Configure SMS
    Serial.print("[5] Configuring SMS... ");
    sendCommand("AT+CMGF=1", 1000);
    sendCommand("AT+CSCS=\"GSM\"", 1000);
    sendCommand("AT+CNMI=2,1,0,0,0", 1000);
    sendCommand("AT+CMGD=1,4", 1000);
    Serial.println("OK");
    
    Serial.println("==========================================");
    Serial.println("  SIM MODULE READY");
    Serial.println("==========================================\n");
    
    isReady = true;
    return true;
}

bool SIMManager::sendSMS(String message) {
    if (!isReady) {
        Serial.println("[SMS] SIM not ready");
        return false;
    }
    
    Serial.println("\n[SMS] Sending...");
    String cmd = "AT+CMGS=\"" + String(MOBILE_NUMBER) + "\"";
    String response = sendCommand(cmd, 1000, false);
    
    if (response.indexOf(">") == -1) {
        Serial.println("[SMS] No prompt");
        return false;
    }
    
    delay(300);
    simSerial.print(message);
    delay(300);
    simSerial.write(26);
    delay(5000);
    
    response = "";
    unsigned long startTime = millis();
    while (millis() - startTime < 3000) {
        while (simSerial.available()) {
            response += (char)simSerial.read();
        }
    }
    
    if (response.indexOf("+CMGS") != -1) {
        Serial.println("[SMS] Sent!");
        return true;
    }
    
    Serial.println("[SMS] Failed");
    return false;
}

String SIMManager::checkNewSMS() {
    String response = sendCommand("AT+CMGL=\"REC UNREAD\"", 3000, false);
    
    if (response.indexOf("+CMGL:") == -1) {
        return "";
    }
    
    Serial.println("\n[SMS] New message received!");
    
    // Extract message content
    int lastComma = response.lastIndexOf(",");
    int contentStart = response.indexOf("\n", lastComma);
    if (contentStart == -1) return "";
    
    String content = response.substring(contentStart + 1);
    content.trim();
    content.toLowerCase();
    
    Serial.print("[SMS] Content: ");
    Serial.println(content);
    
    return content;
}

void SIMManager::deleteAllSMS() {
    sendCommand("AT+CMGD=1,4", 1000, false);
}