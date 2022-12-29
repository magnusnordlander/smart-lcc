//
// Created by Magnus Nordlander on 2021-08-20.
//

#include <cstring>
#include "SystemSettings.h"
#include <hardware/watchdog.h>
#include <hardware/flash.h>

// We're going to erase and reprogram a region 256k from the start of flash.
// Once done, we can access this at XIP_BASE + 256k.
#define FLASH_TARGET_OFFSET (256 * 1024)

const SettingStruct defaultSettings{
    .brewTemperatureOffset = -10,
    .sleepMode = false,
    .ecoMode = false,
    .brewTemperatureTarget = 105,
    .serviceTemperatureTarget = 120,
    .autoSleepMin = 0,
    .brewPidParameters = PidSettings{.Kp = 0.8, .Ki = 0.12, .Kd = 12.0, .windupLow = -7.f, .windupHigh = 7.f},
    .servicePidParameters = PidSettings{.Kp = 0.6, .Ki = 0.1, .Kd = 1.0, .windupLow = -10.f, .windupHigh = 10.f},
};

const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);

SystemSettings::SystemSettings(PicoQueue<SystemControllerCommand> *commandQueue, MulticoreSupport* multicoreSupport): _commandQueue(commandQueue), multicoreSupport(multicoreSupport) {

}

void SystemSettings::initialize() {
    readSettings();

    // If we've reset due to the watchdog or for some other reason, use the previous sleep mode setting, otherwise reset it to false
    if (currentSettings.sleepMode && !watchdog_enable_caused_reboot() && to_ms_since_boot(get_absolute_time()) < 20000) {
        currentSettings.sleepMode = false;
    }
}

void SystemSettings::readSettings() {
    currentSettings = SettingStruct{};

//    memcpy(&currentSettings, flash_target_contents, sizeof(SettingStruct));
    memcpy(&currentSettings, &defaultSettings, sizeof(SettingStruct));
}

void SystemSettings::writeSettingsIfChanged() {
    if (memcmp(&currentSettings, flash_target_contents, sizeof(SettingStruct)) != 0) {
        //writeToFlash();
    }
}

void SystemSettings::writeToFlash() {
    uint8_t paddedData[FLASH_PAGE_SIZE];
    memset(paddedData, 0, FLASH_PAGE_SIZE);
    memcpy(paddedData, &currentSettings, sizeof(SettingStruct));

    noInterrupts();
    multicoreSupport->idleOtherCore();

    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, paddedData, FLASH_PAGE_SIZE);

    interrupts();
    multicoreSupport->resumeOtherCore();
}
