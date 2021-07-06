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
            changingSince = rtos::Kernel::Clock::now();
        } else {
            if (rtos::Kernel::Clock::now() - changingSince.value() > threshold) {
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

