//
// Created by Magnus Nordlander on 2021-07-06.
//

#include "HysteresisController.h"

HysteresisController::HysteresisController(float lowerBound, float upperBound) : lowerBound(lowerBound),
                                                                                 upperBound(upperBound) {}

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
