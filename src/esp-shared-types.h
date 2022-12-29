//
// Created by Magnus Nordlander on 2022-11-13.
//

#include <cstdint>

#ifndef FIRMWARE_ARDUINO_COMMON_TYPES_H
#define FIRMWARE_ARDUINO_COMMON_TYPES_H

typedef enum {
    SYSTEM_MODE_UNDETERMINED,
    SYSTEM_MODE_NORMAL,
    SYSTEM_MODE_CONFIG,
    SYSTEM_MODE_OTA,
    SYSTEM_MODE_ESP_FLASH
} SystemMode;

typedef enum {
    SYSTEM_CONTROLLER_STATE_UNDETERMINED = 0,
    SYSTEM_CONTROLLER_STATE_HEATUP,
    SYSTEM_CONTROLLER_STATE_TEMPS_NORMALIZING,
    SYSTEM_CONTROLLER_STATE_WARM,
    SYSTEM_CONTROLLER_STATE_SLEEPING,
    SYSTEM_CONTROLLER_STATE_BAILED,
    SYSTEM_CONTROLLER_STATE_FIRST_RUN
} SystemControllerState;

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

typedef enum : uint16_t {
    LCC_ESP_MESSAGE_TYPE_STATUS = 0,
    LCC_ESP_MESSAGE_TYPE_PING = 1,
    LCC_ESP_MESSAGE_TYPE_PONG = 2,
} LCCESPMessageType;

struct LCCESPHeader {
    uint16_t version = 1;
    LCCESPMessageType messageType;
    uint16_t length;
    uint32_t crc;
};

struct LCCESPStatus {
    float rp2040Temperature;
    uint64_t currentTime;
    uint64_t plannedAutoSleepAt;
    SystemMode mode;
    bool hasBailed;
    bool ecoMode;
    bool brewSSRActive;
    bool serviceSSRActive;
    bool waterTankEmpty;
    SystemControllerState systemState;
    bool sleepModeRequested;
    uint32_t previousBrewDurationMs;
    bool currentlyBrewing;
    bool currentlyFillingServiceBoiler;
    float targetBrewTemp;
    float targetServiceTemp;
    float brewTempOffset;
    PidSettings brewPidSettings;
    PidSettings servicePidSettings;
    PidRuntimeParameters brewPidRuntimeParameters;
    PidRuntimeParameters servicePidRuntimeParameters;
    uint64_t lastSleepModeExitAt;
};

struct LCCESPLCCPing {
    uint16_t messageId;
};

struct LCCESPLCCPong {
    uint16_t messageId;
};

typedef enum : uint16_t {
    ESP_LCC_MESSAGE_TYPE_STATUS = 0,
    ESP_LCC_MESSAGE_TYPE_PING = 1,
    ESP_LCC_MESSAGE_TYPE_PONG = 2,
    ESP_LCC_MESSAGE_TYPE_HELLO = 3,
} ESPLCCMessageType;

struct ESPLCCHeader {
    uint16_t version = 1;
    ESPLCCMessageType messageType;
    uint16_t length;
    uint32_t crc;
};

struct ESPLCCStatus {
    bool isConnectedToWifi;
    bool isConnectedToMqtt;
    bool hasWifiSettings;
    bool hasMqttSettings;
};

#endif //FIRMWARE_ARDUINO_COMMON_TYPES_H
