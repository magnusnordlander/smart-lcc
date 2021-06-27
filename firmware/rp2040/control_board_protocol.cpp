//
// Created by Magnus Nordlander on 2021-06-27.
//

#include "control_board_protocol.h"
#include "utils/polymath.h"
#include "utils/checksum.h"

float high_gain_adc_to_float(uint16_t adcValue) {
    double a = 2.80075E-07;
    double b = -0.000371374;
    double c = 0.272450858;
    double d = -4.737333399;

    return (float)polynomial4(a, b, c, d, adcValue);
}

float low_gain_adc_to_float(uint16_t adcValue) {
    double a = -1.99514E-07;
    double b = 7.66659E-05;
    double c = 0.546325171;
    double d = -17.22637553;

    return (float)polynomial4(a, b, c, d, adcValue);
}

uint16_t validate_raw_packet(ControlBoardRawPacket packet) {
    uint16_t error = NONE;

    if (packet.header != 0x81) {
        error |= INVALID_HEADER;
    }

    static_assert(sizeof(packet) > 2);
    if (calculate_checksum(((uint8_t *) &packet + 1), sizeof(packet) - 2, 0x01) != packet.checksum) {
        error |= INVALID_CHECKSUM;
    }

    return error;
}

ControlBoardParsedPacket convert_raw_packet(ControlBoardRawPacket raw_packet) {
    ControlBoardParsedPacket packet = ControlBoardParsedPacket();

    packet.brew_switch = raw_packet.flags & 0x02;
    packet.water_tank_empty = raw_packet.flags & 0x40;
    packet.service_boiler_low = triplet_to_int(raw_packet.service_boiler_level) > 256;
    packet.brew_boiler_temperature = high_gain_adc_to_float(
            triplet_to_int(raw_packet.brew_boiler_temperature_high_gain));
    packet.service_boiler_temperature = high_gain_adc_to_float(
            triplet_to_int(raw_packet.service_boiler_temperature_high_gain));
}
