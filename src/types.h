//
// Created by Magnus Nordlander on 2021-08-20.
//

#ifndef FIRMWARE_TYPES_H
#define FIRMWARE_TYPES_H

#include <pico/time.h>
#include "esp-shared-types.h"

struct SettingStruct {
    float brewTemperatureOffset = -10;
    bool sleepMode = false;
    bool ecoMode = false;
    float brewTemperatureTarget = 105;
    float serviceTemperatureTarget = 120;
    uint16_t autoSleepMin = 0;
    PidSettings brewPidParameters = PidSettings{.Kp = 0.8, .Ki = 0.12, .Kd = 12.0, .windupLow = -7.f, .windupHigh = 7.f};
    PidSettings servicePidParameters = PidSettings{.Kp = 0.6, .Ki = 0.1, .Kd = 1.0, .windupLow = -10.f, .windupHigh = 10.f};
};


struct SystemControllerStatusMessage{
    absolute_time_t timestamp{};
    float brewTemperature{};
    float offsetBrewTemperature{};
    float brewTemperatureOffset{};
    float brewSetPoint{};
    float offsetBrewSetPoint{};
    PidSettings brewPidSettings{};
    PidRuntimeParameters brewPidParameters{};
    float serviceTemperature{};
    float serviceSetPoint{};
    PidSettings servicePidSettings{};
    PidRuntimeParameters servicePidParameters{};
    bool brewSSRActive{};
    bool serviceSSRActive{};
    bool ecoMode{};
    bool sleepMode{};
    SystemControllerState state{};
    SystemControllerBailReason bailReason{};
    bool currentlyBrewing{};
    bool currentlyFillingServiceBoiler{};
    bool waterTankLow{};
    absolute_time_t lastSleepModeExitAt = nil_time;
};

typedef enum {
    COMMAND_SET_BREW_SET_POINT,
    COMMAND_SET_BREW_PID_PARAMETERS,
    COMMAND_SET_SERVICE_SET_POINT,
    COMMAND_SET_SERVICE_PID_PARAMETERS,
    COMMAND_SET_ECO_MODE,
    COMMAND_SET_SLEEP_MODE,
    COMMAND_UNBAIL,
    COMMAND_TRIGGER_FIRST_RUN,
    COMMAND_BEGIN
} SystemControllerCommandType;

struct SystemControllerCommand {
    SystemControllerCommandType type{};
    float float1{};
    float float2{};
    float float3{};
    float float4{};
    float float5{};
    bool bool1{};
};



#endif //FIRMWARE_TYPES_H
