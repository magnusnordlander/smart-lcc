//
// Created by Magnus Nordlander on 2021-06-27.
//

#ifndef LCC_RELAY_PIDCONTROLLER_H
#define LCC_RELAY_PIDCONTROLLER_H

#include <cstdint>

typedef enum {
    UNDETERMINED,
    COLD_BOOT,
    WARM_BOOT,
    RUNNING,
    SLEEPING,
} SystemState;

struct PidParameters {
    double Kp;
    double Ki;
    double Kd;
};

class PIDController {
public:
    bool ecoMode = false;

    float targetBrewTemperature;
    float targetServiceTemperature;

    float brewTemperatureOffset = -10.0f;

    PidParameters pidParameters;

    void updateState(uint32_t time_passed, float brew_temperature, float service_temperature);
protected:
    SystemState systemState = UNDETERMINED;
};


#endif //LCC_RELAY_PIDCONTROLLER_H
