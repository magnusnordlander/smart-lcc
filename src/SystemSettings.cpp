//
// Created by Magnus Nordlander on 2021-08-20.
//

#include <cstring>
#include "SystemSettings.h"
#include <hardware/watchdog.h>
#include <hardware/flash.h>
#include <utils/crc32.h>

// We're going to erase and reprogram a region 256k from the start of flash.
// Once done, we can access this at XIP_BASE + 256k.
#define FLASH_TARGET_OFFSET (256 * 1024)

#define SETTINGS_CURRENT_VERSION 0x01

struct SettingsHeader{
    uint8_t version;
    crc32_t crc;
    size_t len;
};

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

const SettingsHeader *storedHeader = reinterpret_cast<const SettingsHeader*>(flash_target_contents);
const SettingStruct *storedSettings = reinterpret_cast<const SettingStruct*>(flash_target_contents + sizeof(SettingsHeader));

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

    if (storedHeader->version == SETTINGS_CURRENT_VERSION && storedHeader->len == sizeof(SettingStruct)) {
        crc32_t readCrc;
        crc32(storedSettings, sizeof(SettingStruct), &readCrc);

        if (readCrc == storedHeader->crc) {
            memcpy(&currentSettings, storedSettings, sizeof(SettingStruct));
            return;
        }
    }

    memcpy(&currentSettings, &defaultSettings, sizeof(SettingStruct));
}

void SystemSettings::writeSettingsIfChanged() {
    if (memcmp(&currentSettings, storedSettings, sizeof(SettingStruct)) != 0) {
        writeToFlash();
    }
}

void SystemSettings::writeToFlash() {
    crc32_t crc;
    crc32(&currentSettings, sizeof(SettingStruct), &crc);

    static_assert(sizeof(SettingsHeader) + sizeof(SettingStruct) <= FLASH_PAGE_SIZE);

    uint8_t paddedData[FLASH_PAGE_SIZE];
    memset(paddedData, 0, FLASH_PAGE_SIZE);

    SettingsHeader header{
        .version = SETTINGS_CURRENT_VERSION,
        .crc = crc,
        .len = sizeof(SettingStruct),
    };

    memcpy(paddedData, &header, sizeof(SettingsHeader));
    memcpy(paddedData + sizeof(SettingsHeader), &currentSettings, sizeof(SettingStruct));

    noInterrupts();
    multicoreSupport->idleOtherCore();

    flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(FLASH_TARGET_OFFSET, paddedData, FLASH_PAGE_SIZE);

    interrupts();
    multicoreSupport->resumeOtherCore();
}
