//
// Created by Magnus Nordlander on 2021-06-19.
//

#ifndef LCC_RELAY_BIANCACONTROLBOARD_H
#define LCC_RELAY_BIANCACONTROLBOARD_H

#include "WaterBoiler.h"

class BiancaControlBoard {
public:
    BiancaControlBoard();

    uint64_t update(uint64_t currentTime, bool pumpOn, bool serviceBoilerSolenoidOpen, bool coffeeBoilerSsrOn, bool serviceBoilerSsrOn);
    void copyPacket(uint8_t* buf);

    bool tankFull = true;
    bool brewSwitch = false;
    bool serviceTapOpen = false;

    double coffeeTempC();
    double serviceTempC();

    double coffeeAmbientC();
    double serviceAmbientC();

    double coffeeLevelMl();
    double serviceLevelMl();
private:
    WaterBoiler coffeeBoiler = WaterBoiler(800000, 25, 0.0008, 25, 14);
    WaterBoiler serviceBoiler = WaterBoiler(1400000, 25, 0.0008, 25, 14);

    bool pumpState = false;
    bool serviceBoilerSolenoidState = false;
    bool coffeeBoilerSsrState = false;
    bool serviceBoilerSsrState = false;

    bool serviceBoilerLow = false;

    uint64_t lastTime{};

    // uint8_t packet[18] = {0x81, 0x00, 0x00, 0x5D, 0x7F, 0x00, 0x79, 0x7F, 0x02, 0x5D, 0x7F, 0x03, 0x2B, 0x00, 0x02, 0x05, 0x7F, 0x67};
     uint8_t packet[18] = {0x81,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    void updatePacketWithBooleans(void);
    void updatePacketWithBoilerLevel(void);
    void updatePacketWithTemperatures(void);
    void updatePacketWithChecksum(void);

    static void writeNumber(uint16_t number, uint8_t* buf);
};


#endif //LCC_RELAY_BIANCACONTROLBOARD_H
