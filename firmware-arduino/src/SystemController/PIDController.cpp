/**
 * Copyright 2019 Bradley J. Snyder <snyder.bradleyj@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "PIDController.h"
#include <cmath>

PIDController::PIDController(const PidSettings &pidParameters, float setPoint) : pidParameters(pidParameters),
                                                                                   setPoint(setPoint) {}

uint8_t PIDController::getControlSignal(float pv, float feedForward) {
    auto now = get_absolute_time();

    double diffS = (double)absolute_time_diff_us(lastPvAt, now)/1000000;

    updatePidSignal(pv, diffS);
    lastPvAt = now;

    if (feedForward > 10.f) {
        feedForward = 10.f;
    }

    if (feedForward < 0.f) {
        feedForward = 0.f;
    }

    float unscaledSignal = pidSignal + feedForward;

    if (unscaledSignal > 10.f) {
        unscaledSignal = 10.f;
    }

    return round(unscaledSignal*2.5f);
}

void PIDController::updatePidSignal(float pv, double dT) {
    // Calculate error
    double error = setPoint - pv;

    // Proportional term
    Pout = pidParameters.Kp * error;

    // Integral term
    integral += error * dT;

    // Prevent integral wind-up
    if(integral > pidParameters.windupHigh )
        integral = pidParameters.windupHigh;
    else if(integral < pidParameters.windupLow )
        integral = pidParameters.windupLow;

    Iout = pidParameters.Ki * integral;

    // Derivative term
    double derivative = (error - _pre_error) / dT;
    Dout = pidParameters.Kd * derivative;

    // Calculate total output
    double output = Pout + Iout + Dout;

    // Restrict to max/min
    if( output > _max )
        output = _max;
    else if( output < _min )
        output = _min;

    // Save error to previous error
    _pre_error = error;

    pidSignal = (float)round(output);
}

void PIDController::updateSetPoint(float newSetPoint) {
    setPoint = newSetPoint;
}
