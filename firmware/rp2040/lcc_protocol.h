//
// Created by Magnus Nordlander on 2021-06-27.
//

#ifndef LCC_RELAY_LCC_PROTOCOL_H
#define LCC_RELAY_LCC_PROTOCOL_H

#include <cstdint>

typedef enum : uint16_t {
    LCC_ERROR_NONE = 0,
    LCC_ERROR_INVALID_HEADER = 1 << 1,
    LCC_ERROR_INVALID_CHECKSUM = 1 << 2,
    LCC_ERROR_UNEXPECTED_FLAGS = 1 << 3,
    LCC_ERROR_BOTH_SSRS_ON = 1 << 4,
    LCC_ERROR_SOLENOID_OPEN_WITHOUT_PUMP = 1 << 5,
} LccPacketValidationError;

struct LccRawPacket {
    uint8_t header{};
    uint8_t byte1{};
    uint8_t byte2{};
    uint8_t byte3{};
    uint8_t checksum{};
};

struct LccParsedPacket {
    bool pump_on;
    bool service_boiler_ssr_on;
    bool service_boiler_solenoid_open;
    bool brew_boiler_ssr_on;
    bool minus_button_pressed;
    bool plus_button_pressed;
};

LccRawPacket convert_lcc_parsed_to_raw(LccParsedPacket parsed);
LccParsedPacket convert_lcc_raw_to_parsed(LccRawPacket raw);

#endif //LCC_RELAY_LCC_PROTOCOL_H
