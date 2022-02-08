//
// Created by Magnus Nordlander on 2021-08-20.
//

#ifndef FIRMWARE_SYSTEMSETTINGS_H
#define FIRMWARE_SYSTEMSETTINGS_H

#include <FS.h>
#include <LittleFS.h>
#include "utils/PicoQueue.h"
#include "types.h"
#include "FileIO.h"

struct SettingStruct {
    float brewTemperatureOffset = -10;
    bool sleepMode = false;
    bool ecoMode = false;
    float brewTemperatureTarget = 105;
    float serviceTemperatureTarget = 120;
    PidSettings brewPidParameters = PidSettings{.Kp = 0.8, .Ki = 0.12, .Kd = 12.0, .windupLow = -7.f, .windupHigh = 7.f};
    PidSettings servicePidParameters = PidSettings{.Kp = 0.6, .Ki = 0.1, .Kd = 1.0, .windupLow = -10.f, .windupHigh = 10.f};
};

class SystemSettings {
public:
    explicit SystemSettings(PicoQueue<SystemControllerCommand> *commandQueue, FileIO* fileIO);

    void initialize();

    inline float getBrewTemperatureOffset() const { return currentSettings.brewTemperatureOffset; };

    void setBrewTemperatureOffset(float offset);
    void setEcoMode(bool ecoMode);
    void setSleepMode(bool sleepMode);
    void setTargetBrewTemp(float targetBrewTemp);
    inline void setOffsetTargetBrewTemp(float offsetTargetBrewTemp) { setTargetBrewTemp(offsetTargetBrewTemp - currentSettings.brewTemperatureOffset); };
    void setTargetServiceTemp(float targetServiceTemp);
    void setBrewPidParameters(PidSettings params);
    void setServicePidParameters(PidSettings params);
private:
    PicoQueue<SystemControllerCommand> *_commandQueue;

    FileIO* _fileIO;

    SettingStruct currentSettings;
    void readSettings();
    void writeSettings();

    void sendCommand(SystemControllerCommandType commandType, bool value);
    void sendCommand(SystemControllerCommandType commandType, float value);
    void sendCommand(SystemControllerCommandType commandType, PidSettings value);

    void sendCommandObject(SystemControllerCommand command);
};


#endif //FIRMWARE_SYSTEMSETTINGS_H
