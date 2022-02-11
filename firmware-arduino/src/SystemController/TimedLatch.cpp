//
// Created by Magnus Nordlander on 2021-07-06.
//

#include "TimedLatch.h"

TimedLatch::TimedLatch(uint16_t thresholdMs, bool initialState) : threshold(thresholdMs),
                                                                 currentState(initialState) {}

bool TimedLatch::get() const {
    return currentState;
}

void TimedLatch::set(bool value) {
    if (value != currentState) {
        if (!changingSince.has_value()) {
            changingSince = get_absolute_time();
        } else {
            auto now = get_absolute_time();
            if ((absolute_time_diff_us(changingSince.value(), now) / 1000) > threshold) {
                changingSince.reset();
                currentState = value;
            }
        }
    } else {
        changingSince.reset();
    }
}

void TimedLatch::setImmediate(bool value) {
    currentState = value;
    changingSince.reset();
}

