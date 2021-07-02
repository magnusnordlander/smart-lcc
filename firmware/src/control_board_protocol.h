//
// Created by Magnus Nordlander on 2021-06-27.
//

#ifndef LCC_RELAY_CONTROL_BOARD_PROTOCOL_H
#define LCC_RELAY_CONTROL_BOARD_PROTOCOL_H

#include <cstdint>
#include "utils/triplet.h"

typedef enum : uint16_t {
    NONE = 0,
    INVALID_HEADER = 1 << 0,
    INVALID_CHECKSUM = 1 << 1,
    UNEXPECTED_FLAGS = 1 << 2,
    HIGH_AND_LOW_GAIN_BREW_BOILER_TEMP_TOO_DIFFERENT = 1 << 3,
    HIGH_AND_LOW_GAIN_SERVICE_BOILER_TEMP_TOO_DIFFERENT = 1 << 4,
} ControlBoardPacketValidationError;

struct ControlBoardRawPacket {
    uint8_t header{};
    uint8_t flags{};
    Triplet brew_boiler_temperature_low_gain{};
    Triplet service_boiler_temperature_low_gain{};
    Triplet brew_boiler_temperature_high_gain{};
    Triplet service_boiler_temperature_high_gain{};
    Triplet service_boiler_level{};
    uint8_t checksum{};
};

struct ControlBoardParsedPacket {
    bool brew_switch;
    bool water_tank_empty;
    bool service_boiler_low;
    float brew_boiler_temperature;
    float service_boiler_temperature;
};

uint16_t validate_raw_packet(ControlBoardRawPacket packet);
ControlBoardParsedPacket convert_raw_packet(ControlBoardRawPacket raw_packet);

#endif //LCC_RELAY_CONTROL_BOARD_PROTOCOL_H
