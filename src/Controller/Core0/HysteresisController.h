//
// Created by Magnus Nordlander on 2021-07-06.
//

#ifndef FIRMWARE_HYSTERESISCONTROLLER_H
#define FIRMWARE_HYSTERESISCONTROLLER_H

#include <cstdint>

typedef enum {
    HYSTERESIS_STATE_ASCENDING,
    HYSTERESIS_STATE_DESCENDING,
} HysteresisState;

class HysteresisController {
public:
    HysteresisController(float setPoint, float delta);

    void updateSetPoint(float setPoint);

    uint8_t getControlSignal(float value);

private:
    float delta;

    float lowerBound;
    float upperBound;

    HysteresisState state = HYSTERESIS_STATE_ASCENDING;
};


#endif //FIRMWARE_HYSTERESISCONTROLLER_H
