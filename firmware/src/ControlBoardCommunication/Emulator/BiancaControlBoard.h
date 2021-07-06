//
// Created by Magnus Nordlander on 2021-06-19.
//

#ifndef LCC_RELAY_BIANCACONTROLBOARD_H
#define LCC_RELAY_BIANCACONTROLBOARD_H

#include <mbed.h>
#include "WaterBoiler.h"
#include <ControlBoardCommunication/lcc_protocol.h>
#include <ControlBoardCommunication/control_board_protocol.h>

class BiancaControlBoard {
public:
    BiancaControlBoard();

    void update(rtos::Kernel::Clock::time_point currentTime, LccRawPacket packet);
    ControlBoardRawPacket latestPacket();

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

    rtos::Kernel::Clock::time_point lastTime;

    ControlBoardRawPacket packet;
};


#endif //LCC_RELAY_BIANCACONTROLBOARD_H
