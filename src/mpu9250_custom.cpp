#include "mpu9250_custom.h"

MPU9250_Custom::MPU9250_Custom() {
    roll = pitch = yaw = 0;
    last_time = 0;
}

void MPU9250_Custom::writeRegister(uint8_t device_addr, uint8_t reg, uint8_t value) {
    Wire.beginTransmission(device_addr);
    Wire.write(reg);
    Wire.write(value);
    Wire.endTransmission();
}

uint8_t MPU9250_Custom::readRegister(uint8_t device_addr, uint8_t reg) {
    Wire.beginTransmission(device_addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(device_addr, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0;
}

void MPU9250_Custom::readBytes(uint8_t device_addr, uint8_t reg, uint8_t count, uint8_t* dest) {
    Wire.beginTransmission(device_addr);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(device_addr, count);
    for (uint8_t i = 0; i < count; i++) {
        if (Wire.available()) {
            dest[i] = Wire.read();
        }
    }
}

void MPU9250_Custom::calibrateSensors() {
    Serial.println("Calibrating MPU9250... Keep sensor still!");
    delay(2000);
    
    // Calibrate accelerometer (100 samples)
    long ax_sum = 0, ay_sum = 0, az_sum = 0;
    for (int i = 0; i < 100; i++) {
        update();
        ax_sum += accel_x;
        ay_sum += accel_y;
        az_sum += accel_z;
        delay(10);
    }
    
    // Accelerometer offsets (should be 0,0,16384 for 1g on Z)
    accel_offset_x = ax_sum / 100.0;
    accel_offset_y = ay_sum / 100.0;
    accel_offset_z = (az_sum / 100.0) - 16384.0;  // Assuming 1g = 16384 LSB
    
    // Calibrate gyro (100 samples)
    long gx_sum = 0, gy_sum = 0, gz_sum = 0;
    for (int i = 0; i < 100; i++) {
        update();
        gx_sum += gyro_x;
        gy_sum += gyro_y;
        gz_sum += gyro_z;
        delay(10);
    }
    
    gyro_offset_x = gx_sum / 100.0;
    gyro_offset_y = gy_sum / 100.0;
    gyro_offset_z = gz_sum / 100.0;
    
    Serial.println("Calibration complete!");
}

void MPU9250_Custom::begin(int sda, int scl) {
    sda_pin = sda;
    scl_pin = scl;
    
    Wire.begin(sda_pin, scl_pin);
    Wire.setClock(400000);
    
    delay(100);
    
    // Wake up MPU9250
    writeRegister(MPU9250_ADDRESS, MPU9250_PWR_MGMT_1, 0x00);
    delay(100);
    
    // Configure accelerometer (±2g)
    writeRegister(MPU9250_ADDRESS, MPU9250_ACCEL_CONFIG, 0x00);
    
    // Configure gyro (±250 dps)
    writeRegister(MPU9250_ADDRESS, MPU9250_GYRO_CONFIG, 0x00);
    
    // Configure magnetometer (AK8963)
    writeRegister(AK8963_ADDRESS, AK8963_CNTL, 0x16);  // 16-bit, 100Hz
    
    delay(100);
    
    // Calibrate sensors
    calibrateSensors();
    
    last_time = millis();
}

void MPU9250_Custom::update() {
    uint8_t buffer[14];
    
    // Read accelerometer and gyro data
    readBytes(MPU9250_ADDRESS, MPU9250_ACCEL_XOUT_H, 14, buffer);
    
    // Convert accelerometer data (16-bit signed)
    accel_x = (int16_t)((buffer[0] << 8) | buffer[1]);
    accel_y = (int16_t)((buffer[2] << 8) | buffer[3]);
    accel_z = (int16_t)((buffer[4] << 8) | buffer[5]);
    
    // Apply accelerometer calibration and convert to g (16384 LSB/g)
    ax = (accel_x - accel_offset_x) / 16384.0;
    ay = (accel_y - accel_offset_y) / 16384.0;
    az = (accel_z - accel_offset_z) / 16384.0;
    
    // Convert gyro data (16-bit signed) - 131 LSB/dps for ±250 dps
    gyro_x = (int16_t)((buffer[8] << 8) | buffer[9]);
    gyro_y = (int16_t)((buffer[10] << 8) | buffer[11]);
    gyro_z = (int16_t)((buffer[12] << 8) | buffer[13]);
    
    // Apply gyro calibration and convert to dps
    gx = (gyro_x - gyro_offset_x) / 131.0;
    gy = (gyro_y - gyro_offset_y) / 131.0;
    gz = (gyro_z - gyro_offset_z) / 131.0;
    
    // Read magnetometer data
    uint8_t mag_buffer[7];
    readBytes(AK8963_ADDRESS, 0x03, 7, mag_buffer);
    
    // Check if data is ready
    if (mag_buffer[6] & 0x01) {
        mag_x = (int16_t)((mag_buffer[1] << 8) | mag_buffer[0]);
        mag_y = (int16_t)((mag_buffer[3] << 8) | mag_buffer[2]);
        mag_z = (int16_t)((mag_buffer[5] << 8) | mag_buffer[4]);
        
        // Convert to uT (0.15 uT/LSB)
        mx = mag_x * 0.15;
        my = mag_y * 0.15;
        mz = mag_z * 0.15;
    }
    
    // Calculate roll and pitch from accelerometer
    float accel_roll = atan2(ay, az) * 180.0 / PI;
    float accel_pitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180.0 / PI;
    
    // Complementary filter with gyro
    unsigned long current_time = millis();
    if (last_time != 0) {
        gyro_dt = (current_time - last_time) / 1000.0;
        
        // Gyro integration
        roll += gx * gyro_dt;
        pitch += gy * gyro_dt;
        yaw += gz * gyro_dt;
        
        // Complementary filter (0.96 for gyro, 0.04 for accelerometer)
        roll = 0.96 * roll + 0.04 * accel_roll;
        pitch = 0.96 * pitch + 0.04 * accel_pitch;
    }
    
    last_time = current_time;
}