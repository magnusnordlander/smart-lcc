//
// Created by Magnus Nordlander on 2021-07-06.
//

#include "HysteresisController.h"

HysteresisController::HysteresisController(float setPoint, float delta) : delta(delta),
                                                                          lowerBound(setPoint - delta),
                                                                          upperBound(setPoint + delta) {}

bool HysteresisController::getControlSignal(float value) {
    if (state == HYSTERESIS_STATE_ASCENDING) {
        if (value < upperBound) {
            return true;
        } else {
            state = HYSTERESIS_STATE_DESCENDING;
            return false;
        }
    } else {
        if (value > lowerBound) {
            return false;
        } else {
            state = HYSTERESIS_STATE_ASCENDING;
            return true;
        }
    }
}

void HysteresisController::updateSetPoint(float setPoint) {
    lowerBound = setPoint - delta;
    upperBound = setPoint + delta;
}
