//
// Created by Magnus Nordlander on 2021-07-25.
//

#include "HybridController.h"

HybridController::HybridController(float setPoint, float hybridDelta, const PidParameters &pidParameters,
                                   float hysteresisDelta):
                                   hysteresisController(setPoint, hysteresisDelta),
                                   pidController(pidParameters, setPoint),
                                   delta(hybridDelta),
                                   lowerPidBound(setPoint - hybridDelta),
                                   upperPidBound(setPoint + hybridDelta)
                                   { }

void HybridController::updateSetPoint(float setPoint) {
    lowerPidBound = setPoint - delta;
    upperPidBound = setPoint + delta;

    hysteresisController.updateSetPoint(setPoint);
    pidController.updateSetPoint(setPoint);
}

bool HybridController::getControlSignal(float value) {
    bool hysteresisValue = hysteresisController.getControlSignal(value);
    bool pidValue = pidController.getControlSignal(value);

    if (value > lowerPidBound && value < upperPidBound) {
        printf("Using PID. P: %02f I: %02f D:%02f PID: %u C: %s\n", pidController.Pout, pidController.Iout, pidController.Dout, (unsigned int)pidController.pidSignal, pidValue ? "Y": "N");
        return pidValue;
    }

    //printf("Using Hysteresis\n");
    return hysteresisValue;
}
