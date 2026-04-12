#ifndef SIM_MANAGER_H
#define SIM_MANAGER_H

#include "config.h"

class SIMManager {
private:
    bool isReady;
    
public:
    SIMManager();
    
    bool begin();
    bool isSimReady() { return isReady; }
    String sendCommand(String command, int timeout = 3000, bool debug = false);
    void clearBuffer();
    bool sendSMS(String message);
    String checkNewSMS();
    void deleteAllSMS();
};

#endif