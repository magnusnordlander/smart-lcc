//
// Created by Magnus Nordlander on 2021-06-27.
//

#ifndef LCC_RELAY_PIDCONTROLLER_H
#define LCC_RELAY_PIDCONTROLLER_H

#include <cstdint>
#include <pico/time.h>
#include "../types.h"
#include "../optional.hpp"

struct PidParameters {
    double Kp;
    double Ki;
    double Kd;
};

class PIDController {
public:
    PIDController(const PidSettings &pidParameters, float setPoint);

    PidSettings pidParameters;

    void updateSetPoint(float setPoint);
    uint8_t getControlSignal(float value, float feedForward = 0.f);

    double integral = 0;

    double Pout = 0;
    double Iout = 0;
    double Dout = 0;
private:
    const double _max = 10;
    const double _min = 0;

    float pidSignal = 0;

    double _pre_error = 0;

    float setPoint;

    absolute_time_t lastPvAt;

    void updatePidSignal(float pv, double dT);
};


#endif //LCC_RELAY_PIDCONTROLLER_H
