//
// Created by Magnus Nordlander on 2021-07-06.
//

#include "HysteresisController.h"

HysteresisController::HysteresisController(float setPoint, float delta) : delta(delta),
                                                                          lowerBound(setPoint - delta),
                                                                          upperBound(setPoint + delta) {}

uint8_t HysteresisController::getControlSignal(float value) {
    if (state == HYSTERESIS_STATE_ASCENDING) {
        if (value < upperBound) {
            return 25;
        } else {
            state = HYSTERESIS_STATE_DESCENDING;
            return 0;
        }
    } else {
        if (value > lowerBound) {
            return 0;
        } else {
            state = HYSTERESIS_STATE_ASCENDING;
            return 25;
        }
    }
}

void HysteresisController::updateSetPoint(float setPoint) {
    lowerBound = setPoint - delta;
    upperBound = setPoint + delta;
}
