//
// Created by Magnus Nordlander on 2021-07-25.
//

#include "HybridController.h"

HybridController::HybridController(float setPoint, float hybridDelta, const PidParameters &pidParameters,
                                   float hysteresisDelta, uint16_t cycleTime):
                                   pidController(pidParameters, setPoint, cycleTime),
                                   hysteresisController(setPoint, hysteresisDelta),
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
        return pidValue;
    }

    //printf("Using Hysteresis\n");
    return hysteresisValue;
}

void HybridController::setPidParameters(PidParameters pidParameters) {
    pidController.pidParameters = pidParameters;
}
