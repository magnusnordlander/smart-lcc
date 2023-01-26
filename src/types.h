//
// Created by Magnus Nordlander on 2021-08-20.
//

#ifndef FIRMWARE_TYPES_H
#define FIRMWARE_TYPES_H

#include <pico/time.h>

typedef enum {
    SYSTEM_MODE_UNDETERMINED,
    SYSTEM_MODE_NORMAL,
    SYSTEM_MODE_CONFIG,
    SYSTEM_MODE_OTA,
    SYSTEM_MODE_ESP_FLASH
} SystemMode;

typedef enum {
    BAIL_REASON_NONE = 0,
    BAIL_REASON_CB_UNRESPONSIVE = 1,
    BAIL_REASON_CB_PACKET_INVALID = 2,
    BAIL_REASON_LCC_PACKET_INVALID = 3,
    BAIL_REASON_SSR_QUEUE_EMPTY = 4,
} SystemControllerBailReason;

struct PidSettings {
    float Kp{};
    float Ki{};
    float Kd{};
    float windupLow{};
    float windupHigh{};
};

struct PidRuntimeParameters {
    bool hysteresisMode = false;
    float p = 0;
    float i = 0;
    float d = 0;
    float integral = 0;
};

typedef enum {
    NOT_STARTED_YET,
    RUNNING,
    SOFT_BAIL,
    HARD_BAIL,
} SystemControllerInternalState;

typedef enum {
    RUN_STATE_UNDETEMINED,
    RUN_STATE_HEATUP_STAGE_1, // Bring the Brew boiler up to 130, don't run the service boiler
    RUN_STATE_HEATUP_STAGE_2, // Keep the Brew boiler at 130 for 4 minutes, run service boiler as normal
    RUN_STATE_NORMAL,
} SystemControllerRunState;

typedef enum {
    SYSTEM_CONTROLLER_COALESCED_STATE_UNDETERMINED = 0,
    SYSTEM_CONTROLLER_COALESCED_STATE_HEATUP,
    SYSTEM_CONTROLLER_COALESCED_STATE_TEMPS_NORMALIZING,
    SYSTEM_CONTROLLER_COALESCED_STATE_WARM,
    SYSTEM_CONTROLLER_COALESCED_STATE_SLEEPING,
    SYSTEM_CONTROLLER_COALESCED_STATE_BAILED,
    SYSTEM_CONTROLLER_COALESCED_STATE_FIRST_RUN
} SystemControllerCoalescedState;

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
    SystemControllerInternalState internalState{};
    SystemControllerRunState runState{};
    SystemControllerCoalescedState coalescedState{};
    SystemControllerBailReason bailReason{};
    bool currentlyBrewing{};
    bool currentlyFillingServiceBoiler{};
    bool waterTankLow{};
    uint16_t autoSleepMinutes{};
    float plannedSleepInSeconds{};
    absolute_time_t lastSleepModeExitAt = nil_time;
    uint16_t bailCounter{};
};

typedef enum {
    COMMAND_SET_BREW_SET_POINT,
    COMMAND_SET_OFFSET_BREW_SET_POINT,
    COMMAND_SET_BREW_OFFSET,
    COMMAND_SET_BREW_PID_PARAMETERS,
    COMMAND_SET_SERVICE_SET_POINT,
    COMMAND_SET_SERVICE_PID_PARAMETERS,
    COMMAND_SET_ECO_MODE,
    COMMAND_SET_SLEEP_MODE,
    COMMAND_SET_AUTO_SLEEP_MINUTES,
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
