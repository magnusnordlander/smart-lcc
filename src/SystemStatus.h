//
// Created by Magnus Nordlander on 2021-07-02.
//

#ifndef FIRMWARE_SYSTEMSTATUS_H
#define FIRMWARE_SYSTEMSTATUS_H

#include <pico/time.h>
#include "Controller/Core0/lcc_protocol.h"
#include "Controller/Core0/control_board_protocol.h"
#include "Controller/Core0/PIDController.h"
#include "Controller/Core0/SystemSettings.h"


class SystemStatus {
public:
    explicit SystemStatus();

    // Status
    bool hasSentLccPacket = false;
    bool hasReceivedControlBoardPacket = false;

    float rp2040Temperature = 0.f;

    nonstd::optional<absolute_time_t> lastBrewStartedAt;
    nonstd::optional<absolute_time_t> lastBrewEndedAt;
    nonstd::optional<absolute_time_t> plannedAutoSleepAt;

    SystemMode mode;
    bool wifiConnected = false;
    bool mqttConnected = false;

    inline bool hasBailed() const { return latestStatusMessage.coalescedState == SYSTEM_CONTROLLER_COALESCED_STATE_BAILED; }
    inline SystemControllerBailReason bailReason() const { return latestStatusMessage.bailReason; }

    inline absolute_time_t getCurrentTime() const { return latestStatusMessage.timestamp; }

    inline float getOffsetTargetBrewTemperature() const { return latestStatusMessage.offsetBrewSetPoint; }
    inline float getOffsetBrewTemperature() const { return latestStatusMessage.offsetBrewTemperature; }
    inline float getBrewTemperature() const { return latestStatusMessage.brewTemperature; }
    inline float getServiceTemperature() const { return latestStatusMessage.serviceTemperature; }

    inline SystemControllerCoalescedState getState() const { return latestStatusMessage.coalescedState; }
    inline bool isInEcoMode() const { return latestStatusMessage.ecoMode; }
    inline bool isBrewSsrOn() const { return latestStatusMessage.brewSSRActive; }
    inline bool isServiceSsrOn() const { return latestStatusMessage.serviceSSRActive; }
    inline bool isWaterTankEmpty() const { return latestStatusMessage.waterTankLow; }
    inline bool isInSleepMode() const { return latestStatusMessage.sleepMode; }

    inline bool hasPreviousBrew() const { return !currentlyBrewing() && lastBrewStartedAt.has_value() && lastBrewEndedAt.has_value(); }
    inline uint32_t previousBrewDurationMs() const { return absolute_time_diff_us(lastBrewStartedAt.value(), lastBrewEndedAt.value()) / 1000; }

    inline bool currentlyBrewing() const { return latestStatusMessage.currentlyBrewing; }
    inline bool currentlyFillingServiceBoiler() const { return latestStatusMessage.currentlyFillingServiceBoiler; }

    inline float getTargetBrewTemp() const { return latestStatusMessage.brewSetPoint; }
    inline float getTargetServiceTemp() const { return latestStatusMessage.serviceSetPoint; }
    inline float getBrewTempOffset() const { return latestStatusMessage.brewTemperatureOffset; }

    inline PidSettings getBrewPidSettings() const { return latestStatusMessage.brewPidSettings; }
    inline PidSettings getServicePidSettings() const { return latestStatusMessage.servicePidSettings; }
    inline PidRuntimeParameters getBrewPidRuntimeParameters() const { return latestStatusMessage.brewPidParameters; }
    inline PidRuntimeParameters getServicePidRuntimeParameters() const { return latestStatusMessage.servicePidParameters; }

    inline absolute_time_t getLastSleepModeExitAt() const { return latestStatusMessage.lastSleepModeExitAt; };

    void updateStatusMessage(SystemControllerStatusMessage message);
private:
    SystemControllerStatusMessage latestStatusMessage;
};


#endif //FIRMWARE_SYSTEMSTATUS_H
