//
// Created by Magnus Nordlander on 2021-08-20.
//

#include "SystemSettings.h"
#include <mbed.h>
#include <kvstore_global_api.h>
#include "pico/multicore.h"

#define SETTING_VERSION ((uint8_t)2)

SystemSettings::SystemSettings(PicoQueue<SystemControllerCommand> *commandQueue): _commandQueue(commandQueue) {

}

void SystemSettings::initialize() {
    uint8_t settingVersion = readKvUint8("/kv/setting_version", 0);

    if (settingVersion != SETTING_VERSION) {
        kv_reset("/kv/");
        writeKv("/kv/setting_version", SETTING_VERSION, true);
    }

    // Order matters here, the System Controller may well bake a control packet using an intermediate state
    sendCommand(COMMAND_SET_ECO_MODE, readKvBool("/kv/eco_mode", true));
    sendCommand(COMMAND_SET_BREW_PID_PARAMETERS, readKvPidParameters("/kv/brew_pid_params", PidSettings{.Kp = 0.8, .Ki = 0.04, .Kd = 16.0}));
    sendCommand(COMMAND_SET_SERVICE_PID_PARAMETERS, readKvPidParameters("/kv/service_pid_params", PidSettings{.Kp = 0.6, .Ki = 0.1, .Kd = 1.0}));
    sendCommand(COMMAND_SET_BREW_SET_POINT, readKvFloat("/kv/target_brew", 95.f, 0.f, 130.f));
    sendCommand(COMMAND_SET_SERVICE_SET_POINT, readKvFloat("/kv/target_service", 125.f, 0.f, 140.f));
}

void SystemSettings::setEcoMode(bool _ecoMode) {
    writeKv("/kv/eco_mode", _ecoMode);
    sendCommand(COMMAND_SET_ECO_MODE, _ecoMode);
}

void SystemSettings::setTargetBrewTemp(float targetBre) {
    sendCommand(COMMAND_SET_BREW_SET_POINT, targetBre);
    writeKv("/kv/target_brew", targetBre);
}

void SystemSettings::setTargetServiceTemp(float targetServiceTemp) {
    sendCommand(COMMAND_SET_SERVICE_SET_POINT, targetServiceTemp);
    writeKv("/kv/target_service", targetServiceTemp);
}

void SystemSettings::setBrewPidParameters(PidSettings params) {
    sendCommand(COMMAND_SET_BREW_PID_PARAMETERS, params);
    writeKv("/kv/brew_pid_params", params);
}

void SystemSettings::setServicePidParameters(PidSettings params) {
    sendCommand(COMMAND_SET_SERVICE_PID_PARAMETERS, params);
    writeKv("/kv/service_pid_params", params);
}


void SystemSettings::sendCommand(SystemControllerCommandType commandType, bool value) {
    SystemControllerCommand command{};
    command.type = commandType;
    command.bool1 = value;

    sendCommandObject(command);
}

void SystemSettings::sendCommand(SystemControllerCommandType commandType, float value) {
    SystemControllerCommand command{};
    command.type = commandType;
    command.float1 = value;

    sendCommandObject(command);
}

void SystemSettings::sendCommand(SystemControllerCommandType commandType, PidSettings value) {
    SystemControllerCommand command{};
    command.type = commandType;
    command.float1 = value.Kp;
    command.float2 = value.Ki;
    command.float3 = value.Kd;

    sendCommandObject(command);
}

void SystemSettings::writeKv(const char *key, bool value, bool skip_lockout) {
    int res = kvSetInternal(key, &value, sizeof(value), 0, skip_lockout);

    if (res != MBED_SUCCESS) {
        printf("Error writing bool %u to %s: %u\n", value, key, res);
    }
}

void SystemSettings::writeKv(const char *key, float value, bool skip_lockout) {
    int res = kvSetInternal(key, &value, sizeof(value), 0, skip_lockout);
    if (res != MBED_SUCCESS) {
        printf("Error writing float %f to %s: %u\n", value, key, res);
    }
}

void SystemSettings::writeKv(const char *key, uint8_t value, bool skip_lockout) {
    int res = kvSetInternal(key, &value, sizeof(value), 0, skip_lockout);
    if (res != MBED_SUCCESS) {
        printf("Error writing uint8 %u to %s: %u\n", value, key, res);
    }
}

void SystemSettings::writeKv(const char *key, PidSettings value, bool skip_lockout) {
    int res = kvSetInternal(key, &value, sizeof(value), 0, skip_lockout);
    if (res != MBED_SUCCESS) {
        printf("Error writing pid parameters to %s: %u\n", key, res);
    }
}

bool SystemSettings::readKvBool(const char *key, bool defaultValue) {
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

float SystemSettings::readKvFloat(const char *key, float defaultValue, float lowerBound, float upperBound) {
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

uint8_t SystemSettings::readKvUint8(const char *key, uint8_t defaultValue) {
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

PidSettings SystemSettings::readKvPidParameters(const char *key, PidSettings defaultValue) {
    size_t outSz;
    int res;

    PidSettings value{};
    res = kv_get(key, &value, sizeof(value), &outSz);

    if (res != MBED_SUCCESS) {
        printf("Kv pid parameters %s was not set, defaulting\n", key);
        value = defaultValue;
    }

    return value;
}

void SystemSettings::sendCommandObject(SystemControllerCommand command) {
    printf("Sending command of type %u. F1: %.1f F2 %.1f F3 %.1f B1: %u\n", (uint8_t)command.type, command.float1, command.float2, command.float3, command.bool1);
    _commandQueue->addBlocking(&command);
}

void SystemSettings::sendLockout() {
    SystemControllerCommand command{};
    command.type = COMMAND_ALLOW_LOCKOUT;
    sendCommandObject(command);
}

int SystemSettings::kvSetInternal(const char *full_name_key, const void *buffer, size_t size, uint32_t create_flags, bool skip_lockout) {
    if (!skip_lockout) {
        sendLockout();
        multicore_lockout_start_blocking();
    }

    int res = kv_set(full_name_key, buffer, size, create_flags);

    if (!skip_lockout) {
        multicore_lockout_end_blocking();
    }

    return res;
}
