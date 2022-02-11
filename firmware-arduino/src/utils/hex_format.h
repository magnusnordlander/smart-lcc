//
// Created by Magnus Nordlander on 2021-06-27.
//

#ifndef LCC_RELAY_HEX_FORMAT_H
#define LCC_RELAY_HEX_FORMAT_H

#include <cstdlib>

void hex_format(unsigned char * in, size_t insz, char * out, size_t outsz);
void printhex(unsigned char * in, size_t insz);
void printlnhex(unsigned char * in, size_t insz);


#endif //LCC_RELAY_HEX_FORMAT_H
