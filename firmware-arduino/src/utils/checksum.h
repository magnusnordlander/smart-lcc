//
// Created by Magnus Nordlander on 2021-06-27.
//

#ifndef LCC_RELAY_CHECKSUM_H
#define LCC_RELAY_CHECKSUM_H

#include <cstdint>
#include <cstdlib>

uint8_t calculate_checksum(const uint8_t *buf, size_t len, uint8_t initial);

#endif //LCC_RELAY_CHECKSUM_H
