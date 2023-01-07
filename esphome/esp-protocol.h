//
// Created by Magnus Nordlander on 2022-12-31.
//

#ifndef SMART_LCC_ESP_PROTOCOL_H
#define SMART_LCC_ESP_PROTOCOL_H

#include <cstdint>

enum ESPMessageType: uint32_t {
    ESP_MESSAGE_PING = 0x00000001,
    ESP_MESSAGE_PONG,
    ESP_MESSAGE_ACK,
    ESP_MESSAGE_NACK,
    ESP_MESSAGE_SYSTEM_STATUS,
    ESP_MESSAGE_SYSTEM_COMMAND,
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

struct ESPMessageHeader {
    ESPDirection direction;
    uint32_t id;
    uint32_t responseTo;
    ESPMessageType type;
    ESPError error;
    uint32_t length;
};

struct ESPPingMessage {
    uint16_t version;
};

struct ESPPongMessage {
    uint16_t version;
};

struct ESPSystemStatusMessage {
    uint32_t brewBoilerTemperatureMilliCelsius;
    uint32_t brewBoilerSetPointMilliCelsius;
    uint32_t serviceBoilerTemperatureMilliCelsius;
    uint32_t serviceBoilerSetPointMilliCelsius;
    int32_t brewTemperatureOffsetMilliCelsius;
    uint8_t autoSleepAfter;
    bool currentlyBrewing;
    bool currentlyFillingServiceBoiler;
    bool ecoMode;
    bool sleepMode;
    bool waterTankLow;
    uint16_t plannedAutoSleepInSeconds;
    uint32_t rp2040TemperatureMilliCelsius;
    /*
     * To add:
     * Internal state
     * Run state
     *
     * Pid settings and pid parameters
     */
};

typedef enum {
    ESP_SYSTEM_COMMAND_SET_BREW_SET_POINT,
    ESP_SYSTEM_COMMAND_SET_BREW_PID_PARAMETERS,
    ESP_SYSTEM_COMMAND_SET_SERVICE_SET_POINT,
    ESP_SYSTEM_COMMAND_SET_SERVICE_PID_PARAMETERS,
    ESP_SYSTEM_COMMAND_SET_ECO_MODE,
    ESP_SYSTEM_COMMAND_SET_SLEEP_MODE,
} ESPSystemCommandType;

struct ESPSystemCommandPayload {
    ESPSystemCommandType type;
    bool bool1;
    char pad[3];
    float float1;
    float float2;
    float float3;
    float float4;
    float float5;
};

struct ESPSystemCommandMessage {
    uint32_t checksum;
    ESPSystemCommandPayload payload;
};

#endif //SMART_LCC_ESP_PROTOCOL_H
