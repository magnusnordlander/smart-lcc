//
// Created by Magnus Nordlander on 2021-06-27.
//

#include "triplet.h"

uint16_t triplet_to_int(Triplet triplet) {
    if (triplet.byte2 == 0x7F) {
        return (triplet.byte1 | 0x80) + (triplet.byte0 << 8);
    }

    return triplet.byte1 + (triplet.byte0 << 8);
}