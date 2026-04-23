#include <Arduino.h>
#include <HardwareSerial.h>
#include "config.h"
#include "gps_manager.h"
#include "sim_manager.h"
#include "sms_handler.h"
#include "mpu9250_custom.h"
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// Serial objects
HardwareSerial gpsSerial(2);
HardwareSerial simSerial(1);

// Global objects
GPSManager gps;
SIMManager simManager; 
SMSHandler smsHandler;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
                        OLED_MOSI, OLED_CLK, OLED_DC, OLED_RST, OLED_CS);
MPU9250_Custom mpu;

// State variables
bool processingCommand = false;
unsigned long lastSMSCheck = 0;
unsigned long lastDisplayUpdate = 0;
int score = 5;
unsigned long lastViolationTime = 0;
bool warningSent = false;          // Đã gửi cảnh báo score=0 chưa

// Non-blocking LED Yellow
unsigned long lastYellowTime = 0;
bool yellowLedOn = false;

// Non-blocking Reset blink effect
unsigned long lastBlinkTime = 0;
int resetBlinkCount = 0;
bool isResetting = false;

// Non-blocking Buzzer
unsigned long lastBuzzerTime = 0;
bool buzzerState = false;

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

void updateLEDs() {
    if (score > 0) {
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_RED, LOW);
    } else {
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_RED, HIGH);
    }
}

void updateDisplay() {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Tiêu đề
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.print("GPS - Tracker");
    display.drawLine(0, 9, 128, 9, SSD1306_WHITE);
    
    // Dòng 1 - AccelX
    display.setCursor(0, 12);  
    display.print("AccelX: ");
    display.println(mpu.getAccelX(), 2);
    
    // Dòng 2 - AccelY
    display.setCursor(0, 20);  
    display.print("AccelY: ");
    display.println(mpu.getAccelY(), 2);
    
    // Dòng 3 - Score
    display.setCursor(0, 28);  
    display.print("Score: ");
    display.println(score);
    
    // Dòng 4 - GPS Status
    display.setCursor(0, 36);  
    if (gps.isRunning()) {
        if (gps.hasFix()) {
            display.println("GPS: Active");
        } else {
            display.println("GPS: Searching");
        }
    } else {
        display.println("GPS: Sleep");
    }
    
    // Cảnh báo khi Score = 0
    if (score == 0) {
        display.setTextSize(2);
        display.setCursor(10, 48);  
        display.print("WARNING!");
        display.setTextSize(1);
    }
    
    display.display();
}

void buzzerAlert() {
    if (score == 0) {
        // Còi kêu nhấp nháy 200ms (non-blocking)
        if (millis() - lastBuzzerTime > 200) {
            buzzerState = !buzzerState;
            digitalWrite(BUZZER, buzzerState ? HIGH : LOW);
            lastBuzzerTime = millis();
        }
    } else {
        digitalWrite(BUZZER, LOW);
        buzzerState = false;
    }
}

void checkViolation() {
    float ax = mpu.getAccelX();
    float ay = mpu.getAccelY();
    float az = mpu.getAccelZ();
    
    // Tắt đèn vàng sau 300ms (non-blocking)
    if (yellowLedOn && (millis() - lastYellowTime >= 300)) {
        digitalWrite(LED_YELLOW, LOW);
        yellowLedOn = false;
    }
    
    // Phát hiện vi phạm (dùng độ lớn vector để chính xác hơn)
    float accelMagnitude = sqrt(ax*ax + ay*ay + az*az);
    bool isViolation = (accelMagnitude > 1.2);  // Ngưỡng phù hợp
    
    if (isViolation && (millis() - lastViolationTime > 1000)) {
        if (score > 0) {
            score--;
            lastViolationTime = millis();
            
            // Bật đèn vàng cảnh báo
            digitalWrite(LED_YELLOW, HIGH);
            yellowLedOn = true;
            lastYellowTime = millis();
            
            updateLEDs();
            updateDisplay();
            
            Serial.print("[VIOLATION] Score decreased to: ");
            Serial.println(score);
        }
        
        // CHỈ GỬI SMS DUY NHẤT KHI SCORE = 0
        if (score == 0 && !warningSent && simManager.isSimReady()) {
            String msg = "WARNING! Device violation detected! Score reached 0/5!";
            simManager.sendSMS(msg);
            warningSent = true;
            Serial.println("[SMS] Warning sent: Score = 0");
        }
    }
    if (score > 0 && warningSent) {
        warningSent = false;
        Serial.println("[WARNING] warningSent reset because score > 0");
    }
}

void resetScore() {
    if (score != 5) {
        score = 5;
        warningSent = false;  // Reset flag để có thể gửi cảnh báo lại
        updateLEDs();
        
        // Bắt đầu hiệu ứng nhấp nháy đèn xanh (non-blocking)
        isResetting = true;
        resetBlinkCount = 0;
        lastBlinkTime = millis();
        
        if (simManager.isSimReady()) {
            simManager.sendSMS("Score reset to 5/5");
        }
        
        Serial.println("[RESET] Score reset to 5");
        updateDisplay();
    }
}

// Process location request
void processLocation() {
    Serial.println("\n[CMD] Processing LOCATION request...");
    processingCommand = true;
    
    gps.powerOn();
    
    float lat, lng;
    int sats;
    unsigned long startTime = millis();
    
    Serial.println("[GPS] Waiting for fix...");
    
    while (!gps.hasFix() && millis() - startTime < GPS_FIX_TIMEOUT) {
        gps.process();
        delay(100);
        updateDisplay();
    }
    
    if (gps.getLocation(lat, lng, sats)) {
        String message = "CURRENT LOCATION\n";
        message += "Lat: " + String(lat, 6) + "\n";
        message += "Lon: " + String(lng, 6) + "\n";
        message += "Sats: " + String(sats) + "\n";
        message += "https://maps.google.com/?q=" + String(lat, 6) + "," + String(lng, 6);
        simManager.sendSMS(message);
        Serial.println("[GPS] Location sent");
    } else {
        simManager.sendSMS("Cannot get GPS fix.\nPlease move to open area.");
        Serial.println("[GPS] Fix timeout");
    }
    
    gps.powerOff();
    processingCommand = false;
}

// Process status request
void processStatus() {
    Serial.println("\n[CMD] Processing STATUS request...");
    processingCommand = true;
    
    gps.powerOn();
    
    float lat, lng;
    int sats;
    unsigned long startTime = millis();
    
    while (!gps.hasFix() && millis() - startTime < GPS_FIX_TIMEOUT) {
        gps.process();
        delay(100);
        updateDisplay();
    }
    
    if (gps.getLocation(lat, lng, sats)) {
        double distance = calculateDistance(HOME_LAT, HOME_LON, lat, lng);
        
        String message;
        if (distance > GEOFENCE_RADIUS) {
            message = String(distance, 0) + "m from home - OUTSIDE safe zone";
        } else {
            message = String(distance, 0) + "m from home - INSIDE safe zone";
        }
        
        if (message.length() > 100) {
            message = message.substring(0, 97) + "...";
        }
        
        simManager.sendSMS(message);
        Serial.println("[GPS] Status sent");
    } else {
        simManager.sendSMS("GPS: No fix");
        Serial.println("[GPS] No fix for status");
    }
    
    gps.powerOff();
    
    String scoreMsg = "Current Score: " + String(score) + "/5";
    simManager.sendSMS(scoreMsg);
    
    processingCommand = false;
}

// Command callback from SMS handler
void onCommandReceived(const String& cmd) {
    if (processingCommand) return;
    
    if (cmd == "LOCATION") {
        processLocation();
    } else if (cmd == "STATUS") {
        processStatus();
    } else if (cmd == "SCORE") {
        processingCommand = true; 
        String scoreMsg = "Current Score: " + String(score) + "/5";
        simManager.sendSMS(scoreMsg);
        processingCommand = false;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Khởi tạo LED và còi (các chân đã được define trong config.h)
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(BUZZER, OUTPUT);
    pinMode(RESET_BTN, INPUT_PULLUP);
    
    // Tắt tất cả
    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(BUZZER, LOW);
    
    Serial.println("     GPS TRACKER - POWER SAVING MODE     ");

    // Initialize OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC)) {
        Serial.println("[ERROR] OLED initialization failed!");
    } else {
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Initializing...");
        display.display();
    }
    
    // Initialize MPU9250
    Serial.println("[MPU] Initializing MPU9250...");
    mpu.begin(21, 22);
    delay(100);
    
    // Initialize SIM
    simSerial.begin(SIM_BAUD, SERIAL_8N1, SIM_RX, SIM_TX);
    if (!simManager.begin()) {
        Serial.println("\n[ERROR] SIM initialization failed!");
        Serial.println("Check:");
        Serial.println("  - Power supply (5V/2A)");
        Serial.println("  - SIM card");
        Serial.println("  - Antenna");
        Serial.println("  - Wiring (ESP32 TX27→SIM RX, RX26→SIM TX)");
        while(1) { 
            delay(1000);
            Serial.print(".");
        }
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
    Serial.println("  'score'    → Get current score");
    
    lastSMSCheck = millis();
    lastDisplayUpdate = millis();
    updateLEDs();
    updateDisplay();
    
    Serial.println("[SYSTEM] Ready!");
}

void loop() {
    // 1. Xử lý hiệu ứng reset blink (non-blocking)
    if (isResetting) {
        if (millis() - lastBlinkTime >= 100) {
            lastBlinkTime = millis();
            if (resetBlinkCount % 2 == 0) {
                digitalWrite(LED_GREEN, LOW);
            } else {
                digitalWrite(LED_GREEN, HIGH);
            }
            resetBlinkCount++;
            
            if (resetBlinkCount >= 6) {  // 3 lần nhấp nháy
                digitalWrite(LED_GREEN, score > 0 ? HIGH : LOW);
                isResetting = false;
            }
        }
    }
    
    // 2. Kiểm tra nút reset (có debounce)
    static unsigned long lastButtonTime = 0;
    if (digitalRead(RESET_BTN) == LOW && (millis() - lastButtonTime > 200)) {
        lastButtonTime = millis();
        resetScore();
    }
    
    // 3. Cập nhật dữ liệu MPU
    mpu.update();
    
    // 4. Kiểm tra vi phạm
    checkViolation();
    
    // 5. Còi báo khi hết điểm
    buzzerAlert();
    
    // 6. Xử lý GPS nếu đang chạy
    if (gps.isRunning()) {
        gps.process();
    }
    
    // 7. Cập nhật OLED (10Hz)
    if (millis() - lastDisplayUpdate >= 100) {
        updateDisplay();
        lastDisplayUpdate = millis();
    }
    
    // 8. Kiểm tra SMS mới
    if (simManager.isSimReady() && (millis() - lastSMSCheck >= SMS_CHECK_INTERVAL)) {
        String smsContent = simManager.checkNewSMS();
        if (smsContent.length() > 0) {
            Serial.println("[SMS] Received: " + smsContent);
            smsHandler.processCommand(smsContent);
            simManager.deleteAllSMS();
        }
        lastSMSCheck = millis();
    }
    
    // 9. Delay nhỏ để tránh tràn CPU
    delay(20);
}