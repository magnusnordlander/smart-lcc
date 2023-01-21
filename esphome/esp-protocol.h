//
// Created by Magnus Nordlander on 2022-12-31.
//

#ifndef SMART_LCC_ESP_PROTOCOL_H
#define SMART_LCC_ESP_PROTOCOL_H

#include <cstdint>

#define ESP_RP2040_PROTOCOL_VERSION 0x0001

enum ESPMessageType: uint32_t {
    ESP_MESSAGE_PING = 0x00000001,
    ESP_MESSAGE_PONG,
    ESP_MESSAGE_ACK,
    ESP_MESSAGE_NACK,
    ESP_MESSAGE_SYSTEM_STATUS,
    ESP_MESSAGE_SYSTEM_COMMAND,
    ESP_MESSAGE_ESP_STATUS,
};

enum ESPDirection: uint32_t {
    ESP_DIRECTION_RP2040_TO_ESP32 = 0x00000001,
    ESP_DIRECTION_ESP32_TO_RP2040
};

enum ESPError: uint32_t {
    ESP_ERROR_NONE = 0x0,
    ESP_ERROR_INCOMPLETE_DATA = 0x01,
    ESP_ERROR_INVALID_CHECKSUM,
    ESP_ERROR_UNEXPECTED_MESSAGE_LENGTH,
    ESP_ERROR_PING_WRONG_VERSION = 0x05,
};

struct __packed ESPMessageHeader {
    ESPDirection direction;
    uint32_t id;
    uint32_t responseTo;
    ESPMessageType type;
    ESPError error;
    uint32_t length;
};

struct ESPPingMessage {
    uint16_t version = ESP_RP2040_PROTOCOL_VERSION;
};

struct ESPPongMessage {
    uint16_t version = ESP_RP2040_PROTOCOL_VERSION;
};

enum ESPSystemInternalState: uint8_t {
    ESP_SYSTEM_INTERNAL_STATE_NOT_STARTED_YET,
    ESP_SYSTEM_INTERNAL_STATE_RUNNING,
    ESP_SYSTEM_INTERNAL_STATE_SOFT_BAIL,
    ESP_SYSTEM_INTERNAL_STATE_HARD_BAIL,
};

enum ESPSystemRunState: uint8_t {
    ESP_SYSTEM_RUN_STATE_UNDETEMINED,
    ESP_SYSTEM_RUN_STATE_NORMAL,
    ESP_SYSTEM_RUN_STATE_HEATUP_STAGE_1,
    ESP_SYSTEM_RUN_STATE_HEATUP_STAGE_2,
    ESP_SYSTEM_RUN_STATE_FIRST_RUN,
};

enum ESPSystemCoalescedState: uint8_t {
    ESP_SYSTEM_COALESCED_STATE_UNDETERMINED = 0,
    ESP_SYSTEM_COALESCED_STATE_HEATUP,
    ESP_SYSTEM_COALESCED_STATE_TEMPS_NORMALIZING,
    ESP_SYSTEM_COALESCED_STATE_WARM,
    ESP_SYSTEM_COALESCED_STATE_SLEEPING,
    ESP_SYSTEM_COALESCED_STATE_BAILED,
    ESP_SYSTEM_COALESCED_STATE_FIRST_RUN
};

struct __packed ESPSystemStatusMessage {
    ESPSystemInternalState internalState;
    ESPSystemRunState runState;
    ESPSystemCoalescedState coalescedState;
    float brewBoilerTemperature;
    float brewBoilerSetPoint;
    float serviceBoilerTemperature;
    float serviceBoilerSetPoint;
    float brewTemperatureOffset;
    uint16_t autoSleepAfter;
    bool currentlyBrewing;
    bool currentlyFillingServiceBoiler;
    bool ecoMode;
    bool sleepMode;
    bool waterTankLow;
    uint16_t plannedAutoSleepInSeconds;
    float rp2040Temperature;
    /*
     * To add:
     * Pid settings and pid parameters
     */
};

enum ESPSystemCommandType: uint32_t {
    ESP_SYSTEM_COMMAND_SET_BREW_SET_POINT,
    ESP_SYSTEM_COMMAND_SET_BREW_PID_PARAMETERS,
    ESP_SYSTEM_COMMAND_SET_BREW_OFFSET,
    ESP_SYSTEM_COMMAND_SET_SERVICE_SET_POINT,
    ESP_SYSTEM_COMMAND_SET_SERVICE_PID_PARAMETERS,
    ESP_SYSTEM_COMMAND_SET_ECO_MODE,
    ESP_SYSTEM_COMMAND_SET_SLEEP_MODE,
    ESP_SYSTEM_COMMAND_SET_AUTO_SLEEP_MINUTES
};

struct __packed ESPSystemCommandPayload {
    ESPSystemCommandType type;
    bool bool1;
    float float1;
    float float2;
    float float3;
    float float4;
    float float5;
};

struct __packed ESPSystemCommandMessage {
    uint32_t checksum;
    ESPSystemCommandPayload payload;
};

struct __packed ESPESPStatusMessage {
    bool wifiConnected;
    bool improvAuthorizationRequested;
};
#endif //SMART_LCC_ESP_PROTOCOL_H
