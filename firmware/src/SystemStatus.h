//
// Created by Magnus Nordlander on 2021-07-02.
//

#ifndef FIRMWARE_SYSTEMSTATUS_H
#define FIRMWARE_SYSTEMSTATUS_H

#include <mbed.h>
#include <SystemController/lcc_protocol.h>
#include <SystemController/control_board_protocol.h>
#include <SystemController/PIDController.h>

class SystemStatus {
public:
    SystemStatus();

    // Status
    bool hasSentLccPacket = false;
    bool hasReceivedControlBoardPacket = false;

    nonstd::optional<rtos::Kernel::Clock::time_point> lastBrewStartedAt;
    nonstd::optional<rtos::Kernel::Clock::time_point> lastBrewEndedAt;

    bool wifiConnected = false;
    bool mqttConnected = false;

    inline bool hasBailed() const { return latestStatusMessage.state == SYSTEM_CONTROLLER_STATE_BAILED; }
    inline SystemControllerBailReason bailReason() const { return latestStatusMessage.bailReason; }

    inline float getOffsetTargetBrewTemperature() const { return getTargetBrewTemp() + brewTemperatureOffset; }
    inline float getOffsetBrewTemperature() const { return getBrewTemperature() + brewTemperatureOffset; }
    inline float getBrewTemperature() const { return latestStatusMessage.brewTemperature; }
    inline float getServiceTemperature() const { return latestStatusMessage.serviceTemperature; }

    inline bool isInEcoMode() const { return latestStatusMessage.ecoMode; }
    inline bool isBrewSsrOn() const { return latestStatusMessage.brewSSRActive; }
    inline bool isServiceSsrOn() const { return latestStatusMessage.serviceSSRActive; }
    inline bool isWaterTankEmpty() const { return latestStatusMessage.waterTankLow; }

    inline bool currentlyBrewing() const { return latestStatusMessage.currentlyBrewing; }
    inline bool currentlyFillingServiceBoiler() const { return latestStatusMessage.currentlyFillingServiceBoiler; }

    inline float getTargetBrewTemp() const { return latestStatusMessage.brewSetPoint; }
    inline float getTargetServiceTemp() const { return latestStatusMessage.serviceSetPoint; }
    inline float getBrewTempOffset() const { return brewTemperatureOffset; }

    inline PidSettings getBrewPidSettings() const { return latestStatusMessage.brewPidSettings; }
    inline PidSettings getServicePidSettings() const { return latestStatusMessage.servicePidSettings; }
    inline PidRuntimeParameters getBrewPidRuntimeParameters() const { return latestStatusMessage.brewPidParameters; }
    inline PidRuntimeParameters getServicePidRuntimeParameters() const { return latestStatusMessage.servicePidParameters; }

    void updateStatusMessage(SystemControllerStatusMessage message);
private:
    // Settings
    float brewTemperatureOffset = -10.0f;

    SystemControllerStatusMessage latestStatusMessage;
};


#endif //FIRMWARE_SYSTEMSTATUS_H
