//
// Created by Magnus Nordlander on 2021-07-02.
//

#ifndef FIRMWARE_SYSTEMSTATUS_H
#define FIRMWARE_SYSTEMSTATUS_H

#include <mbed.h>
#include <lcc_protocol.h>
#include <control_board_protocol.h>
#include <SystemController/PIDController.h>

class SystemStatus {
public:
    SystemStatus();

    bool hasSentLccPacket = false;
    rtos::Kernel::Clock::time_point lastLccPacketSentAt;
    LccParsedPacket lccPacket;

    bool hasReceivedControlBoardPacket = false;
    ControlBoardParsedPacket controlBoardPacket;
    ControlBoardRawPacket controlBoardRawPacket;

    bool has_bailed;

    bool currentlyBrewing = false;
    nonstd::optional<rtos::Kernel::Clock::time_point> lastBrewStartedAt;
    nonstd::optional<rtos::Kernel::Clock::time_point> lastBrewEndedAt;

    bool ecoMode = true;

    float targetBrewTemperature = 105.0f;
    float targetServiceTemperature = 140.0f;

    float brewTemperatureOffset = -10.0f;

    bool wifiConnected = false;
    bool mqttConnected = false;

    double integral = 0;

    double p = 0;
    double i = 0;
    double d = 0;

    PidParameters brewPidParameters = PidParameters{.Kp = 0.8, .Ki = 0.04, .Kd = 20.0};
    PidParameters servicePidParameters = PidParameters{.Kp = 0.6, .Ki = 0.1, .Kd = 1.0};

    uint16_t lastBssrCycleMs = 0;
    uint16_t lastSssrCycleMs = 0;
    uint16_t minBssrOnCycleMs = 0;
    uint16_t minBssrOffCycleMs = 0;
    uint16_t minSssrOnCycleMs = 0;
    uint16_t minSssrOffCycleMs = 0;

    inline float getOffsetTargetBrewTemperature() const { return targetBrewTemperature + brewTemperatureOffset; }
    inline float getOffsetBrewTemperature() const { return controlBoardPacket.brew_boiler_temperature + brewTemperatureOffset; }
};


#endif //FIRMWARE_SYSTEMSTATUS_H
