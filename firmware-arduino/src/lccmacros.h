//
// Created by Magnus Nordlander on 2021-08-23.
//

#ifndef FIRMWARE_MACROS_H
#define FIRMWARE_MACROS_H

// I *think* there's something wrong with printf:ing floats. It's been causing hard faults and wrong numbers
// This ugly ass way of formatting floats to a string solves it.

#define FUCKED_UP_FLOAT_1_FMT "%d.%u"
#define FUCKED_UP_FLOAT_2_FMT "%d.%02u"

#define FUCKED_UP_FLOAT_1_ARG(f) (int16_t)f, (uint8_t)abs((int)((f*10)-(float)(((int16_t)f)*10)))
#define FUCKED_UP_FLOAT_2_ARG(f) (int16_t)f, (uint8_t)abs((int)((f*100)-(float)(((int16_t)f)*100)))

#endif //FIRMWARE_MACROS_H
