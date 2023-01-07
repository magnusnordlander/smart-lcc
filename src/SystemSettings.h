//
// Created by Magnus Nordlander on 2021-08-20.
//

#ifndef FIRMWARE_SYSTEMSETTINGS_H
#define FIRMWARE_SYSTEMSETTINGS_H

#include "utils/PicoQueue.h"
#include "types.h"
#include <MulticoreSupport.h>

class SystemSettings {
public:
    explicit SystemSettings(PicoQueue<SystemControllerCommand> *commandQueue, MulticoreSupport *multicoreSupport);

    void initialize();

    [[nodiscard]] inline float getBrewTemperatureOffset() const { return currentSettings.brewTemperatureOffset; };
    inline bool getEcoMode() const { return currentSettings.ecoMode; };
    inline bool getSleepMode() const { return currentSettings.sleepMode; };
    inline float getTargetBrewTemp() const { return currentSettings.brewTemperatureTarget; };
    [[nodiscard]] inline uint16_t getAutoSleepMin() const { return currentSettings.autoSleepMin; };
    inline float getOffsetTargetBrewTemp() const { return currentSettings.brewTemperatureTarget + currentSettings.brewTemperatureOffset; };
    inline float getTargetServiceTemp() const { return currentSettings.serviceTemperatureTarget; };
    inline PidSettings getBrewPidParameters() const { return currentSettings.brewPidParameters; };
    inline PidSettings getServicePidParameters() const { return currentSettings.servicePidParameters; };

    inline void setBrewTemperatureOffset(float offset) { currentSettings.brewTemperatureOffset = offset; };
    inline void setEcoMode(bool ecoMode) { currentSettings.ecoMode = ecoMode; };
    inline void setSleepMode(bool sleepMode) { currentSettings.sleepMode = sleepMode; };
    inline void setTargetBrewTemp(float targetBrewTemp) { currentSettings.brewTemperatureTarget = targetBrewTemp; };
    inline void setAutoSleepMin(uint16_t minutes) { currentSettings.autoSleepMin = minutes; };
    inline void setOffsetTargetBrewTemp(float offsetTargetBrewTemp) { setTargetBrewTemp(offsetTargetBrewTemp - currentSettings.brewTemperatureOffset); };
    inline void setTargetServiceTemp(float targetServiceTemp) { currentSettings.serviceTemperatureTarget = targetServiceTemp; };
    inline void setBrewPidParameters(PidSettings params) { currentSettings.brewPidParameters = params; };
    inline void setServicePidParameters(PidSettings params) { currentSettings.servicePidParameters = params; };

    void writeSettingsIfChanged();

private:
    PicoQueue<SystemControllerCommand> *_commandQueue;
    MulticoreSupport* multicoreSupport;

    SettingStruct currentSettings;
    void readSettings();
    void writeToFlash();
};


#endif //FIRMWARE_SYSTEMSETTINGS_H
