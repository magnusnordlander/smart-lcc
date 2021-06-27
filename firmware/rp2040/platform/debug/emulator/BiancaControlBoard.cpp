//
// Created by Magnus Nordlander on 2021-06-19.
//

#include "BiancaControlBoard.h"
#include <cmath>
#include <cstring>
#include <ctime>
#include "../../../utils/polymath.h"

BiancaControlBoard::BiancaControlBoard() {
    lastTime = 0;
}

uint64_t BiancaControlBoard::update(uint64_t currentTime, bool pumpOn, bool serviceBoilerSolenoidOpen,
                                   bool coffeeBoilerSsrOn, bool serviceBoilerSsrOn) {
    int64_t diffMs = currentTime - lastTime;

    // Coffee boiler is 1000 W (J/s)
    float coffeeEnergy = coffeeBoilerSsrOn ? 1000.0f * ((float)diffMs/1000.0f) : 0.0f;
    bool pumpToCoffee = pumpState && !serviceBoilerSolenoidState;

    // Let's presume a flow of 10 g per second (or 10 mg / ms)
    uint16_t coffeeWaterExchanged = brewSwitch ? 10 * diffMs : 0;

    coffeeBoiler.update((int16_t)coffeeEnergy, diffMs, coffeeWaterExchanged, pumpToCoffee ? coffeeWaterExchanged : 0);

    // Coffee boiler is 1400 W (J/s)
    float serviceEnergy = serviceBoilerSsrOn ? 1400.0f * ((float)diffMs/1000.0f) : 0.0f;
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

    packet[0] = 0x81;
    updatePacketWithBooleans();
    updatePacketWithTemperatures();
    updatePacketWithBoilerLevel();
    updatePacketWithChecksum();

    lastTime = currentTime;
    pumpState = pumpOn;
    serviceBoilerSolenoidState = serviceBoilerSolenoidOpen;
    coffeeBoilerSsrState = coffeeBoilerSsrOn;
    serviceBoilerSsrState = serviceBoilerSsrOn;

    return diffMs;
}

void BiancaControlBoard::updatePacketWithBooleans(void) {
    uint8_t byte1 = 0x0;
    if (!tankFull) {
        byte1 |= 0x40;
    }
    if (brewSwitch) {
        byte1 |= 0x02;
    }
    packet[1] = byte1;
}

void BiancaControlBoard::updatePacketWithTemperatures(void) {
    double smallA = 1.94759E-06;
    double smallB = -0.000294428;
    double smallC = 1.812604664;
    double smallD = 31.49048711;

    double largeA = -0.000468472;
    double largeB = 0.097074921;
    double largeC = 1.6935213;
    double largeD = 27.8765092;

    uint16_t smallCoffee = round(polynomial4(smallA, smallB, smallC, smallD, coffeeBoiler.currentTemp()));
    uint16_t smallService = round(polynomial4(smallA, smallB, smallC, smallD, serviceBoiler.currentTemp()));
    uint16_t largeCoffee = round(polynomial4(largeA, largeB, largeC, largeD, coffeeBoiler.currentTemp()));
    uint16_t largeService = round(polynomial4(largeA, largeB, largeC, largeD, serviceBoiler.currentTemp()));

    writeNumber(smallCoffee, &packet[2]);
    writeNumber(smallService, &packet[5]);
    writeNumber(largeCoffee, &packet[8]);
    writeNumber(largeService, &packet[11]);
}

void BiancaControlBoard::updatePacketWithBoilerLevel(void) {
    uint16_t levelNumber = !serviceBoilerLow ? 128 : 650;
    writeNumber(levelNumber, &packet[14]);
}

void BiancaControlBoard::writeNumber(uint16_t number, uint8_t *buf) {
    if (number & 0x0080) {
        buf[2] = 0x7F;
    } else {
        buf[2] = 0x00;
    }

    buf[0] = number >> 8;
    buf[1] = number & 0x007F;
}

void BiancaControlBoard::updatePacketWithChecksum(void) {
    // The second nibble of byte 0 is also included, but that's always 1.
    uint16_t checksum = 0x1;
    for (unsigned int i = 1; i < 17; i++) {
        checksum += packet[i];
    }

    packet[17] = checksum & 0x7F;
}

void BiancaControlBoard::copyPacket(uint8_t *buf) {
    memcpy(buf, packet, 18);
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

