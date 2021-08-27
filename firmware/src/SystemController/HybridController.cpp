//
// Created by Magnus Nordlander on 2021-07-25.
//

#include "HybridController.h"

HybridController::HybridController(float setPoint, float hybridDelta, const PidSettings &pidParameters,
                                   float hysteresisDelta):
                                   pidController(pidParameters, setPoint),
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

uint8_t HybridController::getControlSignal(float value, float pidFeedForward, bool forceHysteresis) {
    uint8_t hysteresisValue = hysteresisController.getControlSignal(value);
    uint8_t pidValue = pidController.getControlSignal(value, pidFeedForward);

    if (!forceHysteresis && (value > lowerPidBound && value < upperPidBound)) {
        lastModeWasHysteresis = false;
        return pidValue;
    }

    lastModeWasHysteresis = true;
    return hysteresisValue;
}

void HybridController::setPidParameters(PidSettings pidParameters) {
    pidController.pidParameters = pidParameters;
}

PidRuntimeParameters HybridController::getRuntimeParameters() const {
    PidRuntimeParameters params{
        .hysteresisMode = lastModeWasHysteresis,
        .p = (float)pidController.Pout,
        .i = (float)pidController.Iout,
        .d = (float)pidController.Dout,
        .integral = (float)pidController.integral,
    };

    return params;
}
