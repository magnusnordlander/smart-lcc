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
    uint16_t threshold;

    bool currentState;
    nonstd::optional<absolute_time_t> changingSince = nonstd::nullopt;
};


#endif //FIRMWARE_TIMEDLATCH_H
