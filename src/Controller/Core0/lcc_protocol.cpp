//
// Created by Magnus Nordlander on 2021-06-27.
//

#include "lcc_protocol.h"
#include <utils/checksum.h>

LccRawPacket convert_lcc_parsed_to_raw(LccParsedPacket parsed) {
    LccRawPacket packet = LccRawPacket();

    packet.header = 0x80;
    packet.byte1 = (parsed.pump_on ? 0x10 : 0x00) | (parsed.service_boiler_ssr_on ? 0x01: 0x00);
    packet.byte2 = (parsed.service_boiler_solenoid_open ? 0x10 : 0x00) | (parsed.brew_boiler_ssr_on ? 0x08 : 0x00);
    packet.byte3 = (parsed.minus_button_pressed ? 0x08 : 0x00) | (parsed.plus_button_pressed ? 0x04 : 0x00);
    static_assert(sizeof(packet) > 2, "LCC Packet is too small, for some reason");
    packet.checksum = calculate_checksum(((uint8_t *) &packet + 1), sizeof(packet) - 2, 0x00);

    return packet;
}

LccParsedPacket convert_lcc_raw_to_parsed(LccRawPacket raw) {
    LccParsedPacket parsed = LccParsedPacket();

    parsed.pump_on = raw.byte1 & 0x10;
    parsed.service_boiler_ssr_on = raw.byte1 & 0x01;
    parsed.service_boiler_solenoid_open = raw.byte2 & 0x10;
    parsed.brew_boiler_ssr_on = raw.byte2 & 0x08;
    parsed.minus_button_pressed = raw.byte3 & 0x08;
    parsed.plus_button_pressed = raw.byte3 & 0x04;

    return parsed;
}

LccRawPacket create_safe_packet() {
    return (LccRawPacket){0x80, 0, 0, 0, 0};
}

uint16_t validate_lcc_raw_packet(LccRawPacket packet) {
    uint16_t error = LCC_VALIDATION_ERROR_NONE;

    if (packet.header != 0x80) {
        error |= LCC_VALIDATION_ERROR_INVALID_HEADER;
    }

    static_assert(sizeof(packet) == 5, "Weird LCC Packet size");
    uint8_t calculated_checksum = calculate_checksum(((uint8_t *) &packet + 1), sizeof(packet) - 2, 0x00);
    if (calculated_checksum != packet.checksum) {
        error |= LCC_VALIDATION_ERROR_INVALID_CHECKSUM;
    }

    if (packet.byte1 & 0xEE || packet.byte2 & 0xE7 || packet.byte3 & 0xF3) {
        error |= LCC_VALIDATION_ERROR_UNEXPECTED_FLAGS;
    }

    if (packet.byte1 & 0x01 && packet.byte2 & 0x08) {
        error |= LCC_VALIDATION_ERROR_BOTH_SSRS_ON;
    }

    if (packet.byte2 & 0x10 && (packet.byte1 & 0x10) == 0x00) {
        error |= LCC_VALIDATION_ERROR_SOLENOID_OPEN_WITHOUT_PUMP;
    }

    return error;
}
