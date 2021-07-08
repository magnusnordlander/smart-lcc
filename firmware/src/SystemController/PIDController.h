//
// Created by Magnus Nordlander on 2021-06-27.
//

#ifndef LCC_RELAY_PIDCONTROLLER_H
#define LCC_RELAY_PIDCONTROLLER_H

#include <cstdint>



struct PidParameters {
    double Kp;
    double Ki;
    double Kd;
};

class PIDController {
public:


    PidParameters pidParameters;

    void updateState(uint32_t time_passed, float brew_temperature, float service_temperature);
};


#endif //LCC_RELAY_PIDCONTROLLER_H
