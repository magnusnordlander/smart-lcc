//
// Created by Magnus Nordlander on 2021-07-06.
//

#ifndef FIRMWARE_SYSTEMCONTROLLER_H
#define FIRMWARE_SYSTEMCONTROLLER_H


#include <SystemStatus.h>
#include "TimedLatch.h"
#include "HysteresisController.h"

class SystemController {
public:
    explicit SystemController(SystemStatus *status);

    [[noreturn]] void run();

private:
    LccParsedPacket handleControlBoardPacket(ControlBoardParsedPacket packet);

    SystemStatus* status;

    HysteresisController serviceBoilerController = HysteresisController(124.f, 126.f);

    TimedLatch waterTankEmptyLatch = TimedLatch(1000, false);
    TimedLatch serviceBoilerLowLatch = TimedLatch(500, false);
};


#endif //FIRMWARE_SYSTEMCONTROLLER_H
