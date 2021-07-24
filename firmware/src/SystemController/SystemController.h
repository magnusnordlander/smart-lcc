//
// Created by Magnus Nordlander on 2021-07-06.
//

#ifndef FIRMWARE_SYSTEMCONTROLLER_H
#define FIRMWARE_SYSTEMCONTROLLER_H


#include <SystemStatus.h>
#include "TimedLatch.h"
#include "HysteresisController.h"

typedef enum {
    UNDETERMINED,
    COLD_BOOT,
    WARM_BOOT,
    RUNNING,
    SLEEPING,
} SystemState;

class SystemController {
public:
    explicit SystemController(SystemStatus *status);

    [[noreturn]] void run();
private:
    LccParsedPacket handleControlBoardPacket(ControlBoardParsedPacket packet);
    void updateFromSystemStatus();

    SystemStatus* status;

    HysteresisController brewBoilerController;
    HysteresisController serviceBoilerController;

    TimedLatch waterTankEmptyLatch = TimedLatch(1000, false);
    TimedLatch serviceBoilerLowLatch = TimedLatch(500, false);
};


#endif //FIRMWARE_SYSTEMCONTROLLER_H
