#include "mpu9250_custom.h"

MPU9250_Custom::MPU9250_Custom() {
    ax_raw = ay_raw = az_raw = 0;
    ax = ay = az = 0;
    data_ready = false;
}

void MPU9250_Custom::writeRegister(uint8_t reg, uint8_t value) {
    Wire.beginTransmission(MPU9250_ADDRESS);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
    delay(10);
}

bool MPU9250_Custom::readAccelData() {
    // Gửi yêu cầu đọc từ thanh ghi 0x3B
    Wire.beginTransmission(MPU9250_ADDRESS);
    Wire.write(MPU9250_ACCEL_XOUT_H);
    if (Wire.endTransmission(false) != 0) {
        return false;
    }
    
    // Đọc 6 bytes (Ax, Ay, Az mỗi giá trị 2 bytes)
    uint8_t buffer[6];
    // Cách 1: Ép kiểu tường minh (RECOMMENDED)
    uint8_t received = Wire.requestFrom((uint16_t)MPU9250_ADDRESS, (uint8_t)6);
    
    if (received != 6) {
        return false;
    }
    
    for (uint8_t i = 0; i < 6; i++) {
        buffer[i] = Wire.read();
    }
    
    // Chuyển đổi raw data
    ax_raw = (int16_t)((buffer[0] << 8) | buffer[1]);
    ay_raw = (int16_t)((buffer[2] << 8) | buffer[3]);
    az_raw = (int16_t)((buffer[4] << 8) | buffer[5]);
    
    // Chuyển đổi sang đơn vị G (với scale ±2g, 16384 LSB/G)
    ax = ax_raw / 16384.0;
    ay = ay_raw / 16384.0;
    az = az_raw / 16384.0;
    
    return true;
}

void MPU9250_Custom::begin(int sda, int scl) {
    sda_pin = sda;
    scl_pin = scl;
    
    // Khởi tạo I2C
    Wire.begin(sda_pin, scl_pin);
    Wire.setClock(100000); // 100kHz
    
    delay(100);
    
    // Kiểm tra kết nối MPU9250
    Wire.beginTransmission(MPU9250_ADDRESS);
    if (Wire.endTransmission() != 0) {
        Serial.println("MPU9250 not found!");
        data_ready = false;
        return;
    }
    
    Serial.println("MPU9250 connected!");
    
    // Wake up MPU9250 (khỏi sleep mode)
    writeRegister(MPU9250_PWR_MGMT_1, 0x00);
    delay(100);
    
    // Cấu hình accelerometer: ±2g
    writeRegister(MPU9250_ACCEL_CONFIG, 0x00);
    delay(10);
    
    data_ready = true;
    Serial.println("MPU9250 ready!");
}

void MPU9250_Custom::update() {
    if (!data_ready) return;
    
    // Đọc dữ liệu gia tốc
    if (!readAccelData()) {
        Serial.println("Failed to read MPU data!");
        return;
    }
}

float MPU9250_Custom::getAccelX() {
    return ax;
}

float MPU9250_Custom::getAccelY() {
    return ay;
}

float MPU9250_Custom::getAccelZ() {
    return az;
}

bool MPU9250_Custom::isDataReady() {
    return data_ready;
}