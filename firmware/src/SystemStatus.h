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

    // Status
    bool hasSentLccPacket = false;
    LccParsedPacket lccPacket;

    bool hasReceivedControlBoardPacket = false;
    ControlBoardParsedPacket controlBoardPacket;

    bool has_bailed = false;

    bool currentlyBrewing = false;
    nonstd::optional<rtos::Kernel::Clock::time_point> lastBrewStartedAt;
    nonstd::optional<rtos::Kernel::Clock::time_point> lastBrewEndedAt;

    bool wifiConnected = false;
    bool mqttConnected = false;

    double integral = 0;

    double p = 0;
    double i = 0;
    double d = 0;

    inline float getOffsetTargetBrewTemperature() const { return targetBrewTemperature + brewTemperatureOffset; }
    inline float getOffsetBrewTemperature() const { return controlBoardPacket.brew_boiler_temperature + brewTemperatureOffset; }
    inline void setOffsetTargetBrewTemperature(float target) { setTargetBrewTemp(target - brewTemperatureOffset); }

    void setEcoMode(bool ecoMode);
    void setTargetBrewTemp(float targetBrewTemp);
    void setTargetServiceTemp(float targetServiceTemp);
    void setBrewTemperatureOffset(float brewTempOffset);
    void setBrewPidParameters(PidParameters params);
    void setServicePidParameters(PidParameters params);

    inline bool isInEcoMode() const { return ecoMode; }
    inline float getTargetBrewTemp() const { return targetBrewTemperature; }
    inline float getTargetServiceTemp() const { return targetServiceTemperature; }
    inline float getBrewTempOffset() const { return brewTemperatureOffset; }
    inline PidParameters getBrewPidParameters() const { return brewPidParameters; }
    inline PidParameters getServicePidParameters() const { return servicePidParameters; }


    void readSettingsFromKV();
private:
    // Settings
    bool ecoMode = true;
    float targetBrewTemperature = 105.0f;
    float targetServiceTemperature = 0.0f;
    float brewTemperatureOffset = -10.0f;
    PidParameters brewPidParameters = PidParameters{.Kp = 0.8, .Ki = 0.04, .Kd = 20.0};
    PidParameters servicePidParameters = PidParameters{.Kp = 0.6, .Ki = 0.1, .Kd = 1.0};

    static void writeKv(const char* key, bool value);
    static void writeKv(const char* key, float value);
    static void writeKv(const char* key, uint8_t value);
    static void writeKv(const char* key, PidParameters value);
    static bool readKvBool(const char* key, bool defaultValue);
    static uint8_t readKvUint8(const char * key, uint8_t defaultValue);
    static float readKvFloat(const char* key, float defaultValue, float lowerBound, float upperBound);
    static PidParameters readKvPidParameters(const char* key, PidParameters defaultValue);
};


#endif //FIRMWARE_SYSTEMSTATUS_H
