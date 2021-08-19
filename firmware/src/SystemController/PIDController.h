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

class PIDController {
public:
    PIDController(const PidParameters &pidParameters, float setPoint, uint16_t cycleTime);

    PidParameters pidParameters;

    void updateSetPoint(float setPoint);
    uint8_t getControlSignal(float value);

    uint16_t cycleTime = 200;

    const double _max = 10;
    const double _min = 0;

    const double _integral_max = 10;
    const double _integral_min = -10;

    double _pre_error = 0;
    double _integral = 0;

    double Pout = 0;
    double Iout = 0;
    double Dout = 0;

    long long pidSignal = 0;
private:
    float setPoint;

    absolute_time_t lastPvAt;

    nonstd::optional<absolute_time_t> onCycleEnds;
    nonstd::optional<absolute_time_t> offCycleEnds;

    void updatePidSignal(float pv, double dT);
};


#endif //LCC_RELAY_PIDCONTROLLER_H
