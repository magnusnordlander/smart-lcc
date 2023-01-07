//
// Created by Magnus Nordlander on 2023-01-06.
//

#ifndef SMART_LCC_CRC32_H
#define SMART_LCC_CRC32_H

/* Simple public domain implementation of the standard CRC32 checksum.
 * Outputs the checksum for each file given as a command line argument.
 * Invalid file names and files that cause errors are silently skipped.
 * The program reads from stdin if it is called with no arguments. */

#include <cstdio>
#include <cstdint>
#include <cstdlib>

typedef uint32_t crc32_t;

void crc32(const void *data, size_t n_bytes, crc32_t* crc);

#endif //SMART_LCC_CRC32_H
