//
// Created by Magnus Nordlander on 2021-06-14.
//

#include "WaterBoiler.h"
#include <cmath>
#include <cstdio>

WaterBoiler::WaterBoiler(uint32_t massMg, float startingTempC, double coolingCoefficientSecinv, float ambientTempC,
                         float coldWaterTempC)
        : mass_mg(massMg), temp_c(startingTempC), cooling_coefficient_secinv(coolingCoefficientSecinv),
          ambient_temp_c(ambientTempC), cold_water_temp_c(coldWaterTempC) {
}

void WaterBoiler::update(uint16_t energy_j, uint16_t time_passed_ms, uint32_t warm_water_tapped, uint32_t cold_water_added) {
    // Heating
    temp_c += (float)energy_j / (4.186f * (float)mass_mg/1000.0f);

    // Cooling
    float newtemp = (float)(ambient_temp_c + (temp_c - ambient_temp_c) * exp(-cooling_coefficient_secinv * (float)time_passed_ms/1000));
    float diff = temp_c - newtemp;
    temp_c = newtemp;

    if (ambient_temp_c < 60) {
        ambient_temp_c += diff / 4;
    }

    // Removing water
    mass_mg -= warm_water_tapped;

    // Adding water
    mass_mg += cold_water_added;
    float prop = (float)cold_water_added / (float)mass_mg/1000.0f;
    temp_c = (prop * cold_water_temp_c) + (1.f - prop) * temp_c;
}

float WaterBoiler::currentTemp() const {
    return temp_c;
}

uint32_t WaterBoiler::waterLevel() const {
    return mass_mg;
}

float WaterBoiler::ambientTemp() const {
    return ambient_temp_c;
}
