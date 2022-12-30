#!/bin/bash

bin2header .esphome/build/rp2040-connect/.pioenvs/rp2040-connect/bootloader.bin
cp .esphome/build/rp2040-connect/.pioenvs/rp2040-connect/bootloader.bin.h ./

bin2header .esphome/build/rp2040-connect/.pioenvs/rp2040-connect/partitions.bin
cp .esphome/build/rp2040-connect/.pioenvs/rp2040-connect/partitions.bin.h ./

bin2header .esphome/build/rp2040-connect/.pioenvs/rp2040-connect/firmware.bin
cp .esphome/build/rp2040-connect/.pioenvs/rp2040-connect/firmware.bin.h ./

bin2header .esphome/platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin
cp .esphome/platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin.h ./
