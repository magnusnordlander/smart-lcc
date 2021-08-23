//
// Created by Magnus Nordlander on 2021-06-27.
//

#ifndef LCC_RELAY_CONTROL_BOARD_PROTOCOL_H
#define LCC_RELAY_CONTROL_BOARD_PROTOCOL_H

#include <cstdint>
#include "utils/triplet.h"

typedef enum : uint16_t {
    CONTROL_BOARD_VALIDATION_ERROR_NONE = 0,
    CONTROL_BOARD_VALIDATION_ERROR_INVALID_HEADER = 1 << 0,
    CONTROL_BOARD_VALIDATION_ERROR_INVALID_CHECKSUM = 1 << 1,
    CONTROL_BOARD_VALIDATION_ERROR_UNEXPECTED_FLAGS = 1 << 2,
    CONTROL_BOARD_VALIDATION_ERROR_HIGH_AND_LOW_GAIN_BREW_BOILER_TEMP_TOO_DIFFERENT = 1 << 3,
    CONTROL_BOARD_VALIDATION_ERROR_HIGH_AND_LOW_GAIN_SERVICE_BOILER_TEMP_TOO_DIFFERENT = 1 << 4,
    CONTROL_BOARD_VALIDATION_ERROR_BREW_BOILER_TEMP_DANGEROUSLY_HIGH = 1 << 5,
    CONTROL_BOARD_VALIDATION_ERROR_SERVICE_BOILER_TEMP_DANGEROUSLY_HIGH = 1 << 6,
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
ControlBoardParsedPacket convert_raw_control_board_packet(ControlBoardRawPacket raw_packet);
ControlBoardRawPacket convert_parsed_control_board_packet(ControlBoardParsedPacket parsed_packet);

#endif //LCC_RELAY_CONTROL_BOARD_PROTOCOL_H
