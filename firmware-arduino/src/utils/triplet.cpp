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

Triplet int_to_triplet(uint16_t number) {
    Triplet triplet = Triplet();

    if (number & 0x0080) {
        triplet.byte2 = 0x7F;
    } else {
        triplet.byte2 = 0x00;
    }

    triplet.byte0 = number >> 8;
    triplet.byte1 = number & 0x007F;

    return triplet;
}
