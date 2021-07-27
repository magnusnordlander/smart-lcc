//
// Created by Magnus Nordlander on 2021-06-27.
//

#include <cstdio>
#include "hex_format.h"

void printhex(unsigned char * in, size_t insz)
{
    size_t outsz = 3*insz;
    char str[outsz];
    hex_format(in, insz, str, outsz);

    printf("%s", str);
}

void printlnhex(unsigned char * in, size_t insz)
{
    size_t outsz = 3*insz;
    char str[outsz];
    hex_format(in, insz, str, outsz);

    printf("%s\n", str);
}

void hex_format(unsigned char * in, size_t insz, char * out, size_t outsz)
{
    unsigned char * pin = in;
    const char * hex = "0123456789ABCDEF";
    char * pout = out;
    for(; pin < in+insz; pout +=3, pin++){
        pout[0] = hex[(*pin>>4) & 0xF];
        pout[1] = hex[ *pin     & 0xF];
        pout[2] = ':';
        if (pout + 3 - out > outsz){
            /* Better to truncate output string than overflow buffer */
            /* it would be still better to either return a status */
            /* or ensure the target buffer is large enough and it never happen */
            break;
        }
    }
    pout[-1] = 0;
}