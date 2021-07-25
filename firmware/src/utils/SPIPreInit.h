//
// Created by Magnus Nordlander on 2021-07-24.
//

#ifndef FIRMWARE_SPIPREINIT_H
#define FIRMWARE_SPIPREINIT_H

#include <mbed.h>

class SPIPreInit : public mbed::SPI
{
public:
    SPIPreInit(PinName mosi, PinName miso, PinName clk) : SPI(mosi,miso,clk)
    {
        format(8,3);
        frequency(2000000);
    };
};

#endif //FIRMWARE_SPIPREINIT_H
