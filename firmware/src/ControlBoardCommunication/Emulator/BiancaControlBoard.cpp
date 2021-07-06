//
// Created by Magnus Nordlander on 2021-06-19.
//

#include "BiancaControlBoard.h"
#include <cmath>
#include <cstring>
#include <ctime>
#include <utils/polymath.h>

using namespace std::chrono_literals;

BiancaControlBoard::BiancaControlBoard(): lastTime() {
}

void BiancaControlBoard::update(rtos::Kernel::Clock::time_point currentTime, LccRawPacket lccPacket){
    LccParsedPacket parsed = convert_lcc_raw_to_parsed(lccPacket);

    std::chrono::milliseconds diff = currentTime - lastTime;
    auto diffMs = (uint16_t)diff.count();

    // Coffee boiler is 1000 W (J/s)
    float coffeeEnergy = parsed.brew_boiler_ssr_on ? 1000.0f * ((float)diffMs/1000.0f) : 0.0f;
    bool pumpToCoffee = pumpState && !serviceBoilerSolenoidState;

    // Let's presume a flow of 10 g per second (or 10 mg / ms)
    uint16_t coffeeWaterExchanged = brewSwitch ? 10 * diffMs : 0;

    coffeeBoiler.update((int16_t)coffeeEnergy, diffMs, coffeeWaterExchanged, pumpToCoffee ? coffeeWaterExchanged : 0);

    // Coffee boiler is 1400 W (J/s)
    float serviceEnergy = parsed.service_boiler_ssr_on ? 1400.0f * ((float)diffMs/1000.0f) : 0.0f;
    bool pumpToService = pumpState && serviceBoilerSolenoidState;

    // Let's presume a refill rate of 100 g per second (or 50 mg / ms)
    uint16_t serviceWaterAdded = pumpToService ? 100 * diffMs : 0;

    // Let's presume a tap rate of 50 g per second
    uint16_t serviceWaterTapped = serviceTapOpen ? 50 * diffMs : 0;

    serviceBoiler.update((int16_t)serviceEnergy, diffMs, serviceWaterTapped, serviceWaterAdded);

    if (serviceBoilerLow && serviceBoiler.waterLevel() >= 2200000) {
        serviceBoilerLow = false;
    } else if (!serviceBoilerLow && serviceBoiler.waterLevel() < 2000000) {
        serviceBoilerLow = true;
    }

    ControlBoardParsedPacket controlBoardParsedPacket = ControlBoardParsedPacket();
    controlBoardParsedPacket.brew_boiler_temperature = coffeeBoiler.currentTemp();
    controlBoardParsedPacket.service_boiler_temperature = serviceBoiler.currentTemp();
    controlBoardParsedPacket.brew_switch = brewSwitch;
    controlBoardParsedPacket.water_tank_empty = !tankFull;
    controlBoardParsedPacket.service_boiler_low = serviceBoilerLow;

    packet = convert_parsed_control_board_packet(controlBoardParsedPacket);

    lastTime = currentTime;
    pumpState = parsed.pump_on;
    serviceBoilerSolenoidState = parsed.service_boiler_solenoid_open;
    coffeeBoilerSsrState = parsed.brew_boiler_ssr_on;
    serviceBoilerSsrState = parsed.service_boiler_ssr_on;
}

double BiancaControlBoard::coffeeTempC() {
    return coffeeBoiler.currentTemp();
}

double BiancaControlBoard::serviceTempC() {
    return serviceBoiler.currentTemp();
}

double BiancaControlBoard::serviceLevelMl() {
    return (float)serviceBoiler.waterLevel() / 1000.0f;
}

double BiancaControlBoard::coffeeLevelMl() {
    return (float)coffeeBoiler.waterLevel() / 1000.0f;
}

double BiancaControlBoard::coffeeAmbientC() {
    return coffeeBoiler.ambientTemp();
}

double BiancaControlBoard::serviceAmbientC() {
    return serviceBoiler.ambientTemp();
}

ControlBoardRawPacket BiancaControlBoard::latestPacket() {
    return packet;
}

