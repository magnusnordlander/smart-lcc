//
// Created by Magnus Nordlander on 2021-07-25.
//

#ifndef FIRMWARE_HYBRIDCONTROLLER_H
#define FIRMWARE_HYBRIDCONTROLLER_H


#include "HysteresisController.h"
#include "PIDController.h"
#include <types.h>

class HybridController {
public:
    explicit HybridController(float setPoint, float hybridDelta, const PidSettings &pidParameters, float hysteresisDelta);

    void updateSetPoint(float setPoint);
    uint8_t getControlSignal(float value, float pidFeedForward = 0.f);
    PidRuntimeParameters getRuntimeParameters() const;

    void setPidParameters(PidSettings pidParameters);

    PIDController pidController;
private:
    HysteresisController hysteresisController;

    float delta;

    float lowerPidBound;
    float upperPidBound;

    bool lastModeWasHysteresis = true;
};


#endif //FIRMWARE_HYBRIDCONTROLLER_H
