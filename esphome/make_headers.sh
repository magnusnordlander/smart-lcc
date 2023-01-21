#!/bin/bash

bin2header .esphome/platformio/packages/framework-arduinoespressif32/tools/sdk/bin/bootloader_dio_40m.bin
cp .esphome/platformio/packages/framework-arduinoespressif32/tools/sdk/bin/bootloader_dio_40m.bin.h ../esphome-output/

bin2header .esphome/build/smart-lcc/.pioenvs/smart-lcc/partitions.bin
cp .esphome/build/smart-lcc/.pioenvs/smart-lcc/partitions.bin.h ../esphome-output/

bin2header .esphome/build/smart-lcc/.pioenvs/smart-lcc/firmware.bin
cp .esphome/build/smart-lcc/.pioenvs/smart-lcc/firmware.bin.h ../esphome-output/

CRC=$(crc32 .esphome/build/smart-lcc/.pioenvs/smart-lcc/firmware.bin)
MD5=$(echo ${CRC:6:2}${CRC:4:2}${CRC:2:2}${CRC:0:2} | xxd -r -p | md5 | sed -r 's/(..)/0x\1, /g')

CONTENT="#ifndef FIRMWARE_CRC_H
#define FIRMWARE_CRC_H

#include <cstdint>

static const uint32_t firmware_crc = 0x$CRC;
static const esp_bootloader_md5_t firmware_crc_md5{
    .x = {$MD5}
};

#endif /* FIRMWARE_CRC_H */"

echo "$CONTENT" > ../esphome-output/firmware_crc.h

bin2header .esphome/platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin
cp .esphome/platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin.h ../esphome-output/

