//
// Created by Magnus Nordlander on 2021-07-06.
//

#ifndef FIRMWARE_TIMEDLATCH_H
#define FIRMWARE_TIMEDLATCH_H

#include <mbed.h>
#include <optional.hpp>

class TimedLatch {
public:
    TimedLatch(uint16_t thresholdMs, bool currentState);

    bool get() const;
    void set(bool value);
    void setImmediate(bool value);
private:
    rtos::Kernel::Clock::duration threshold;

    bool currentState;
    nonstd::optional<rtos::Kernel::Clock::time_point> changingSince = nonstd::nullopt;
};


#endif //FIRMWARE_TIMEDLATCH_H
