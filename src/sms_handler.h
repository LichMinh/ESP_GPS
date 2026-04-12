#ifndef SMS_HANDLER_H
#define SMS_HANDLER_H

#include <Arduino.h>

class SMSHandler {
public:
    typedef void (*CommandCallback)(const String&);
    
    SMSHandler();
    
    void registerCallback(CommandCallback callback);
    void processCommand(const String& command);
    void sendHelp();
    void sendUnknownCommand();
    
private:
    CommandCallback commandCallback;
};

#endif