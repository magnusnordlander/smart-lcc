//
// Created by Magnus Nordlander on 2021-07-02.
//

#include "SystemStatus.h"
#include <kvstore_global_api.h>

#define SETTING_VERSION ((uint8_t)1)

SystemStatus::SystemStatus():
    lccPacket(LccParsedPacket()),
    controlBoardPacket(ControlBoardParsedPacket()),
    has_bailed(false) {
}

void SystemStatus::setEcoMode(bool _ecoMode) {
    ecoMode = _ecoMode;
    writeKv("/kv/eco_mode", _ecoMode);
}

void SystemStatus::setTargetBrewTemp(float targetBre) {
    targetBrewTemperature = targetBre;
    writeKv("/kv/target_brew", targetBre);
}

void SystemStatus::setTargetServiceTemp(float targetServiceTemp) {
    targetServiceTemperature = targetServiceTemp;
    writeKv("/kv/target_service", targetServiceTemp);
}

void SystemStatus::setBrewTemperatureOffset(float brewTempOffset) {
    brewTemperatureOffset = brewTempOffset;
    writeKv("/kv/brew_temp_offset", brewTempOffset);
}

void SystemStatus::setBrewPidParameters(PidParameters params) {
    brewPidParameters = params;
    writeKv("/kv/brew_pid_params", params);
}

void SystemStatus::setServicePidParameters(PidParameters params) {
    servicePidParameters = params;
    writeKv("/kv/service_pid_params", params);
}

void SystemStatus::readSettingsFromKV() {
    uint8_t settingVersion = readKvUint8("/kv/setting_version", 0);

    if (settingVersion != SETTING_VERSION) {
        kv_reset("/kv/");
        writeKv("/kv/setting_version", SETTING_VERSION);
    }

    ecoMode = readKvBool("/kv/eco_mode", true);
    targetBrewTemperature = readKvFloat("/kv/target_brew", 105.f, 10.f, 130.f);
    targetServiceTemperature = readKvFloat("/kv/target_service", 125.f, 10.f, 140.f);
    brewTemperatureOffset = readKvFloat("/kv/brew_temp_offset", -10.f, -30.f, 30.f);
    brewPidParameters = readKvPidParameters("/kv/brew_pid_params", PidParameters{.Kp = 0.8, .Ki = 0.04, .Kd = 16.0});
    servicePidParameters = readKvPidParameters("/kv/service_pid_params", PidParameters{.Kp = 0.6, .Ki = 0.1, .Kd = 1.0});
}

void SystemStatus::writeKv(const char *key, bool value) {
    int res = kv_set(key, &value, sizeof(value), 0);
    if (res != MBED_SUCCESS) {
        printf("Error writing bool %u to %s: %d\n", value, key, res);
    }
}

void SystemStatus::writeKv(const char *key, float value) {
    int res = kv_set(key, &value, sizeof(value), 0);
    if (res != MBED_SUCCESS) {
        printf("Error writing float %f to %s: %d\n", value, key, res);
    }
}

void SystemStatus::writeKv(const char *key, uint8_t value) {
    int res = kv_set(key, &value, sizeof(value), 0);
    if (res != MBED_SUCCESS) {
        printf("Error writing uint8 %u to %s: %d\n", value, key, res);
    }
}

void SystemStatus::writeKv(const char *key, PidParameters value) {
    int res = kv_set(key, &value, sizeof(value), 0);
    if (res != MBED_SUCCESS) {
        printf("Error writing pid parameters to %s: %d\n", key, res);
    }
}


bool SystemStatus::readKvBool(const char *key, bool defaultValue) {
    size_t outSz;
    int res;

    bool value;
    res = kv_get(key, &value, sizeof(value), &outSz);

    if (res != MBED_SUCCESS) {
        printf("Kv bool %s was not set, defaulting to %u\n", key, defaultValue);
        value = defaultValue;
    }

    return value;
}

float SystemStatus::readKvFloat(const char *key, float defaultValue, float lowerBound, float upperBound) {
    size_t outSz;
    int res;

    float value;
    res = kv_get(key, &value, sizeof(value), &outSz);

    if (res != MBED_SUCCESS) {
        printf("Kv float %s was not set, defaulting to %f\n", key, defaultValue);
        value = defaultValue;
    }

    if (value > upperBound || value < lowerBound) {
        printf("Kv float %s was outside bounds, defaulting to %f\n", key, defaultValue);
        value = defaultValue;
    }

    return value;
}

uint8_t SystemStatus::readKvUint8(const char *key, uint8_t defaultValue) {
    size_t outSz;
    int res;

    bool value;
    res = kv_get(key, &value, sizeof(value), &outSz);

    if (res != MBED_SUCCESS) {
        printf("Kv uint8 %s was not set, defaulting to %u\n", key, defaultValue);
        value = defaultValue;
    }

    return value;
}

PidParameters SystemStatus::readKvPidParameters(const char *key, PidParameters defaultValue) {
    size_t outSz;
    int res;

    PidParameters value{};
    res = kv_get(key, &value, sizeof(value), &outSz);

    if (res != MBED_SUCCESS) {
        printf("Kv pid parameters %s was not set, defaulting\n", key);
        value = defaultValue;
    }

    return value;
}
