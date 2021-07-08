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
    float latestBrewTemperature = 0.f;
private:
    LccParsedPacket handleControlBoardPacket(ControlBoardParsedPacket packet);

    SystemStatus* status;

    bool ecoMode = false;

    float targetBrewTemperature;
    float targetServiceTemperature;

    float brewTemperatureOffset = -10.0f;


    float targetBrewPIDValue = 0.f;

    HysteresisController serviceBoilerController = HysteresisController(124.f, 126.f);

    TimedLatch waterTankEmptyLatch = TimedLatch(1000, false);
    TimedLatch serviceBoilerLowLatch = TimedLatch(500, false);
};


#endif //FIRMWARE_SYSTEMCONTROLLER_H
