//
// Created by Magnus Nordlander on 2021-07-25.
//

#ifndef FIRMWARE_HYBRIDCONTROLLER_H
#define FIRMWARE_HYBRIDCONTROLLER_H


#include "HysteresisController.h"
#include "PIDController.h"

class HybridController {
public:
    explicit HybridController(float setPoint, float hybridDelta, const PidParameters &pidParameters, float hysteresisDelta, uint16_t cycleTime);

    void updateSetPoint(float setPoint);
    uint8_t getControlSignal(float value);

    void setPidParameters(PidParameters pidParameters);

    PIDController pidController;
private:
    HysteresisController hysteresisController;

    float delta;

    float lowerPidBound;
    float upperPidBound;
};


#endif //FIRMWARE_HYBRIDCONTROLLER_H
