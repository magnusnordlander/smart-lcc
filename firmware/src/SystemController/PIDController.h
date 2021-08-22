//
// Created by Magnus Nordlander on 2021-06-27.
//

#ifndef LCC_RELAY_PIDCONTROLLER_H
#define LCC_RELAY_PIDCONTROLLER_H

#include <cstdint>
#include <mbed.h>
#include <optional.hpp>

struct PidParameters {
    double Kp;
    double Ki;
    double Kd;
};
#include <types.h>

class PIDController {
public:
    PIDController(const PidSettings &pidParameters, float setPoint);

    PidSettings pidParameters;

    void updateSetPoint(float setPoint);
    uint8_t getControlSignal(float value);

    double integral = 0;

    double Pout = 0;
    double Iout = 0;
    double Dout = 0;

    long long pidSignal = 0;
private:
    const double _max = 10;
    const double _min = 0;

    double _pre_error = 0;

    float setPoint;

    absolute_time_t lastPvAt;

    void updatePidSignal(float pv, double dT);
};


#endif //LCC_RELAY_PIDCONTROLLER_H
