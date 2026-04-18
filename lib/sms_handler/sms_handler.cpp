#include "sms_handler.h"
#include "sim_manager.h"

extern SIMManager simManager;

SMSHandler::SMSHandler() {
    commandCallback = nullptr;
}

void SMSHandler::registerCallback(CommandCallback callback) {
    commandCallback = callback;
}

void SMSHandler::processCommand(const String& command) {
    if (command.indexOf("location") != -1 || command.indexOf("loc") != -1) {
        if (commandCallback) commandCallback("LOCATION");
    }
    else if (command.indexOf("status") != -1) {
        if (commandCallback) commandCallback("STATUS");
    }
    else if (command.indexOf("help") != -1) {
        sendHelp();
    }
    else {
        sendUnknownCommand();
    }
}

void SMSHandler::sendHelp() {
    String message = "GPS TRACKER COMMANDS:\n";
    message += "location - Get current position\n";
    message += "status - Check safe zone status\n";
    message += "help - Show this guide";
    simManager.sendSMS(message);
}

void SMSHandler::sendUnknownCommand() {
    String message = "Unknown command.\nSend 'help' for available commands.";
    simManager.sendSMS(message);
}