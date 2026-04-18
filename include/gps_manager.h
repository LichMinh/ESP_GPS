#ifndef GPS_MANAGER_H
#define GPS_MANAGER_H

#include <TinyGPSPlus.h>
#include "config.h"

class GPSManager {
private:
    TinyGPSPlus gps;
    bool isActive;
    unsigned long startTime;
    
public:
    GPSManager();
    
    void begin();
    void end();
    void process();
    bool hasFix();
    bool getLocation(float &lat, float &lng, int &sats);
    void powerOn();
    void powerOff();
    bool isRunning() { return isActive; }
    void setTimeout(unsigned long timeout) { startTime = millis(); }
    bool isTimeout(unsigned long timeout);
};

#endif