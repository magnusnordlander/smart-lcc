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
    PIDController(const PidParameters &pidParameters, float setPoint);

    PidParameters pidParameters;

    void updateSetPoint(float setPoint);
    bool getControlSignal(float value);

    const double _max = 1000;
    const double _min = 0;

    double _pre_error = 0;
    double _integral = 0;

    double Pout;
    double Iout;
    double Dout;

    long long pidSignal = 0;
private:
    float setPoint;

    rtos::Kernel::Clock::time_point lastPvAt;

    nonstd::optional<rtos::Kernel::Clock::time_point> onCycleEnds;
    nonstd::optional<rtos::Kernel::Clock::time_point> offCycleEnds;

    void updatePidSignal(float pv, double dT);
};


#endif //LCC_RELAY_PIDCONTROLLER_H
