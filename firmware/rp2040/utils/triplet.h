//
// Created by Magnus Nordlander on 2021-06-27.
//

#ifndef LCC_RELAY_TRIPLET_H
#define LCC_RELAY_TRIPLET_H

#include <cstdint>

struct Triplet {
    uint8_t byte0;
    uint8_t byte1;
    uint8_t byte2;
};

uint16_t triplet_to_int(Triplet triplet);

#endif //LCC_RELAY_TRIPLET_H
