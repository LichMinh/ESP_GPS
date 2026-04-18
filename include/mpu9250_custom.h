#ifndef MPU9250_CUSTOM_H
#define MPU9250_CUSTOM_H

#include <Arduino.h>
#include <Wire.h>

#define MPU9250_ADDRESS 0x68
#define AK8963_ADDRESS 0x0C

// MPU9250 Registers
#define MPU9250_ACCEL_CONFIG 0x1C
#define MPU9250_GYRO_CONFIG 0x1B
#define MPU9250_PWR_MGMT_1 0x6B
#define MPU9250_ACCEL_XOUT_H 0x3B

// AK8963 Registers
#define AK8963_CNTL 0x0A

class MPU9250_Custom {
private:
    int sda_pin;
    int scl_pin;
    
    // Raw data
    int16_t accel_x, accel_y, accel_z;
    int16_t gyro_x, gyro_y, gyro_z;
    int16_t mag_x, mag_y, mag_z;
    
    // Calibrated data
    float ax, ay, az;
    float gx, gy, gz;
    float mx, my, mz;
    
    // Angles
    float roll, pitch, yaw;
    
    // Calibration offsets
    float accel_offset_x = 0, accel_offset_y = 0, accel_offset_z = 0;
    float gyro_offset_x = 0, gyro_offset_y = 0, gyro_offset_z = 0;
    float mag_offset_x = 0, mag_offset_y = 0, mag_offset_z = 0;
    float mag_scale_x = 1, mag_scale_y = 1, mag_scale_z = 1;
    
    // Complementary filter
    float gyro_dt = 0.01;  // 100Hz update rate
    unsigned long last_time;
    
    void writeRegister(uint8_t device_addr, uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t device_addr, uint8_t reg);
    void readBytes(uint8_t device_addr, uint8_t reg, uint8_t count, uint8_t* dest);
    void calibrateSensors();
    
public:
    MPU9250_Custom();
    void begin(int sda = 21, int scl = 22);
    void update();
    
    // Get raw values
    int16_t getRawAccelX() { return accel_x; }
    int16_t getRawAccelY() { return accel_y; }
    int16_t getRawAccelZ() { return accel_z; }
    
    // Get calibrated values (in g for accelerometer, dps for gyro, uT for magnetometer)
    float getAx() { return ax; }
    float getAy() { return ay; }
    float getAz() { return az; }
    float getGx() { return gx; }
    float getGy() { return gy; }
    float getGz() { return gz; }
    float getMx() { return mx; }
    float getMy() { return my; }
    float getMz() { return mz; }
    
    // Get angles (in degrees)
    float getRoll() { return roll; }
    float getPitch() { return pitch; }
    float getYaw() { return yaw; }
};

#endif