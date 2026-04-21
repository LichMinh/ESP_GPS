#ifndef MPU9250_CUSTOM_H
#define MPU9250_CUSTOM_H

#include <Arduino.h>
#include <Wire.h>

#define MPU9250_ADDRESS 0x68
#define MPU9250_PWR_MGMT_1 0x6B
#define MPU9250_ACCEL_XOUT_H 0x3B
#define MPU9250_ACCEL_CONFIG 0x1C

class MPU9250_Custom {
public:
    MPU9250_Custom();
    
    // Khởi tạo MPU với các chân I2C
    void begin(int sda, int scl);
    
    // Đọc dữ liệu gia tốc (raw values)
    void update();
    
    // Lấy giá trị gia tốc theo trục X (đơn vị G)
    float getAccelX();
    
    // Lấy giá trị gia tốc theo trục Y (đơn vị G)
    float getAccelY();
    
    // Lấy giá trị gia tốc theo trục Z (đơn vị G)
    float getAccelZ();
    
    // Kiểm tra đọc dữ liệu thành công
    bool isDataReady();

private:
    int sda_pin;
    int scl_pin;
    
    // Raw data
    int16_t ax_raw, ay_raw, az_raw;
    
    // Giá trị gia tốc sau khi chuyển đổi (G)
    float ax, ay, az;
    
    // Cờ báo dữ liệu hợp lệ
    bool data_ready;
    
    // Hàm ghi thanh ghi
    void writeRegister(uint8_t reg, uint8_t value);
    
    // Hàm đọc dữ liệu gia tốc
    bool readAccelData();
};

#endif