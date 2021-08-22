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

    inline float getBrewTemperatureOffset() const { return brewTemperatureOffset; };

    void setBrewTemperatureOffset(float offset);
    void setEcoMode(bool ecoMode);
    void setTargetBrewTemp(float targetBrewTemp);
    inline void setOffsetTargetBrewTemp(float offsetTargetBrewTemp) { setTargetBrewTemp(offsetTargetBrewTemp - brewTemperatureOffset); };
    void setTargetServiceTemp(float targetServiceTemp);
    void setBrewPidParameters(PidSettings params);
    void setServicePidParameters(PidSettings params);
private:
    PicoQueue<SystemControllerCommand> *_commandQueue;

    float brewTemperatureOffset;

    void sendCommand(SystemControllerCommandType commandType, bool value);
    void sendCommand(SystemControllerCommandType commandType, float value);
    void sendCommand(SystemControllerCommandType commandType, PidSettings value);

    void sendLockout();
    void sendCommandObject(SystemControllerCommand command);

    int kvSetInternal(const char *full_name_key, const void *buffer, size_t size, uint32_t create_flags, bool skip_lockout);

    void writeKv(const char* key, bool value, bool skip_lockout = false);
    void writeKv(const char* key, float value, bool skip_lockout = false);
    void writeKv(const char* key, uint8_t value, bool skip_lockout = false);
    void writeKv(const char* key, PidSettings value, bool skip_lockout = false);

    static bool readKvBool(const char* key, bool defaultValue);
    static uint8_t readKvUint8(const char * key, uint8_t defaultValue);
    static float readKvFloat(const char* key, float defaultValue, float lowerBound, float upperBound);
    static PidSettings readKvPidParameters(const char* key, PidSettings defaultValue);
};


#endif //FIRMWARE_SYSTEMSETTINGS_H
