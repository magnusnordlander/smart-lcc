//
// Created by Magnus Nordlander on 2021-06-27.
//

#include "checksum.h"

uint8_t calculate_checksum(const uint8_t *buf, size_t len, uint8_t initial) {
    if (len > 32) {
        return 0;
    }

    uint16_t checksum = initial;
    for (unsigned int i = 1; i < len; i++) {
        checksum += buf[i];
    }

    return checksum & 0x7F;
}