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

using namespace std::chrono_literals;

PIDController::PIDController(const PidParameters &pidParameters, float setPoint, uint16_t cycleTime) : pidParameters(pidParameters), cycleTime(cycleTime), 
                                                                                   setPoint(setPoint) {}

uint8_t PIDController::getControlSignal(float pv) {
    auto now = rtos::Kernel::Clock::now();

    double diffS = (double)(std::chrono::duration_cast<std::chrono::milliseconds>(now - lastPvAt).count()) / 1000.0;
    updatePidSignal(pv, diffS);
    lastPvAt = now;

    return round((float)pidSignal*2.5f);
}

void PIDController::updatePidSignal(float pv, double dT) {
    // Calculate error
    double error = setPoint - pv;

    // Proportional term
    Pout = pidParameters.Kp * error;

    // Integral term
    _integral += error * dT;

    // Prevent integral wind-up
    if( _integral > _integral_max )
        _integral = _integral_max;
    else if( _integral < _integral_min )
        _integral = _integral_min;

    Iout = pidParameters.Ki * _integral;

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

    pidSignal = (long long)round(output);
}

void PIDController::updateSetPoint(float newSetPoint) {
    setPoint = newSetPoint;
}
