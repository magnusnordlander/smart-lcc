//
// Created by Magnus Nordlander on 2021-08-20.
//

#ifndef FIRMWARE_SYSTEMSETTINGS_H
#define FIRMWARE_SYSTEMSETTINGS_H

#include "utils/PicoQueue.h"
#include "types.h"

class SystemSettings {
public:
    explicit SystemSettings(PicoQueue<SystemControllerCommand> *commandQueue);

    void initialize();

    void setEcoMode(bool ecoMode);
    void setTargetBrewTemp(float targetBrewTemp);
    void setTargetServiceTemp(float targetServiceTemp);
    void setBrewPidParameters(PidSettings params);
    void setServicePidParameters(PidSettings params);
private:
    PicoQueue<SystemControllerCommand> *_commandQueue;

    void sendCommand(SystemControllerCommandType commandType, bool value);
    void sendCommand(SystemControllerCommandType commandType, float value);
    void sendCommand(SystemControllerCommandType commandType, PidSettings value);

    static void writeKv(const char* key, bool value);
    static void writeKv(const char* key, float value);
    static void writeKv(const char* key, uint8_t value);
    static void writeKv(const char* key, PidSettings value);

    static bool readKvBool(const char* key, bool defaultValue);
    static uint8_t readKvUint8(const char * key, uint8_t defaultValue);
    static float readKvFloat(const char* key, float defaultValue, float lowerBound, float upperBound);
    static PidSettings readKvPidParameters(const char* key, PidSettings defaultValue);
};


#endif //FIRMWARE_SYSTEMSETTINGS_H
