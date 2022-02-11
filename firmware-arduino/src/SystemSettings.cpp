//
// Created by Magnus Nordlander on 2021-08-20.
//

#include "SystemSettings.h"
#include <CRC32.h>
#include <hardware/watchdog.h>

#define SETTING_FILENAME ("/fs/settings.dat")
#define SETTING_VERSION ((uint8_t)4)

SystemSettings::SystemSettings(PicoQueue<SystemControllerCommand> *commandQueue, FileIO* fileIO): _commandQueue(commandQueue), _fileIO(fileIO) {

}

void SystemSettings::initialize() {
    readSettings();

    // If we've reset due to the watchdog, use the previous sleep mode setting, otherwise reset it to false
    if (currentSettings.sleepMode && !watchdog_caused_reboot()) {
        currentSettings.sleepMode = false;
        writeSettings();
    }

    // This is run before core1 is launched, and the System controller processes all commands before starting to
    // drive the bus, so order doesn't matter.
    sendCommand(COMMAND_SET_SLEEP_MODE, currentSettings.sleepMode);
    sendCommand(COMMAND_SET_ECO_MODE, currentSettings.ecoMode);
    sendCommand(COMMAND_SET_BREW_PID_PARAMETERS, currentSettings.brewPidParameters);
    sendCommand(COMMAND_SET_SERVICE_PID_PARAMETERS, currentSettings.servicePidParameters);
    sendCommand(COMMAND_SET_BREW_SET_POINT, currentSettings.brewTemperatureTarget);
    sendCommand(COMMAND_SET_SERVICE_SET_POINT, currentSettings.serviceTemperatureTarget);
}

void SystemSettings::setBrewTemperatureOffset(float offset) {
    currentSettings.brewTemperatureOffset = offset;
    writeSettings();
}

void SystemSettings::setEcoMode(bool _ecoMode) {
    currentSettings.ecoMode = _ecoMode;
    writeSettings();
    sendCommand(COMMAND_SET_ECO_MODE, _ecoMode);
}

void SystemSettings::setSleepMode(bool _sleepMode) {
    currentSettings.sleepMode = _sleepMode;
    writeSettings();
    sendCommand(COMMAND_SET_SLEEP_MODE, _sleepMode);
}

void SystemSettings::setTargetBrewTemp(float targetBrew) {
    currentSettings.brewTemperatureTarget = targetBrew;
    writeSettings();
    sendCommand(COMMAND_SET_BREW_SET_POINT, targetBrew);
}

void SystemSettings::setTargetServiceTemp(float targetServiceTemp) {
    currentSettings.serviceTemperatureTarget = targetServiceTemp;
    writeSettings();
    sendCommand(COMMAND_SET_SERVICE_SET_POINT, targetServiceTemp);
}

void SystemSettings::setBrewPidParameters(PidSettings params) {
    currentSettings.brewPidParameters = params;
    writeSettings();
    sendCommand(COMMAND_SET_BREW_PID_PARAMETERS, params);
}

void SystemSettings::setServicePidParameters(PidSettings params) {
    currentSettings.servicePidParameters = params;
    writeSettings();
    sendCommand(COMMAND_SET_SERVICE_PID_PARAMETERS, params);
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
    command.float4 = value.windupLow;
    command.float5 = value.windupHigh;

    sendCommandObject(command);
}

void SystemSettings::sendCommandObject(SystemControllerCommand command) {
    //printf("Sending command of type %u. F1: %.1f F2 %.1f F3 %.1f B1: %u\n", (uint8_t)command.type, command.float1, command.float2, command.float3, command.bool1);
    _commandQueue->addBlocking(&command);
}

void SystemSettings::readSettings() {
    nonstd::optional<SettingStruct> settings = _fileIO->readSystemSettings(SETTING_FILENAME, SETTING_VERSION);

    if (!settings.has_value()) {
        currentSettings = SettingStruct{};
    } else {
        currentSettings = settings.value();
    }
}

void SystemSettings::writeSettings() {
    if (!_fileIO->saveSystemSettings(currentSettings, SETTING_FILENAME, SETTING_VERSION)) {
        DEBUGV("Unable to save system settings\n");
    }
}