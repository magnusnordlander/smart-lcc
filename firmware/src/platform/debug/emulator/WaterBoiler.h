//
// Created by Magnus Nordlander on 2021-06-14.
//

#ifndef COFFEESIMKIT_WATERBOILER_H
#define COFFEESIMKIT_WATERBOILER_H

#include <cstdint>

class WaterBoiler {
public:
    WaterBoiler(uint32_t massMg, float startingTempC, double coolingCoefficientSecinv, float ambientTempC,
                float coldWaterTempC);

    void update(uint16_t energy_j, uint16_t time_passed_ms, uint32_t warm_water_tapped, uint32_t cold_water_added);

    float currentTemp() const;
    uint32_t waterLevel() const;
    float ambientTemp() const;

private:
    uint32_t mass_mg;
    float temp_c;
    double cooling_coefficient_secinv;
    float ambient_temp_c;
    float cold_water_temp_c;
};


#endif //COFFEESIMKIT_WATERBOILER_H
