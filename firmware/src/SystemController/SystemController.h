//
// Created by Magnus Nordlander on 2021-07-06.
//

#ifndef FIRMWARE_SYSTEMCONTROLLER_H
#define FIRMWARE_SYSTEMCONTROLLER_H


#include <SystemStatus.h>
#include "TimedLatch.h"
#include "HysteresisController.h"
#include "HybridController.h"
#include <queue>
#include "../types.h"
#include <utils/PicoQueue.h>
#include <utils/MovingAverage.h>

typedef enum {
    UNDETERMINED,
    COLD_BOOT,
    WARM_BOOT,
    RUNNING,
    SLEEPING,
} SystemControllerInternalState;

typedef enum {
    BOTH_SSRS_OFF = 0,
    BREW_BOILER_SSR_ON,
    SERVICE_BOILER_SSR_ON,
} SsrState;

class SsrStateQueueItem {
public:
    SsrState state = BOTH_SSRS_OFF;
};

class SystemController {
public:
    explicit SystemController(
            uart_inst_t * _uart,
            PinName tx,
            PinName rx,
            PicoQueue<SystemControllerStatusMessage> *outgoingQueue,
            PicoQueue<SystemControllerCommand> *incomingQueue
            );

    [[noreturn]] void run();
private:
    bool has_bailed = false;
    SystemControllerBailReason bail_reason = BAIL_REASON_NONE;

    bool ecoMode = true;
    float targetBrewTemperature = 0.f;
    float targetServiceTemperature = 0.f;
    PidSettings brewPidParameters = PidSettings{.Kp = 0.f, .Ki = 0.f, .Kd = 0.f};
    PidSettings servicePidParameters = PidSettings{.Kp = 0.f, .Ki = 0.f, .Kd = 0.f};

    PidRuntimeParameters brewPidRuntimeParameters{};
    PidRuntimeParameters servicePidRuntimeParameters{};

    PicoQueue<SystemControllerStatusMessage> *outgoingQueue;
    PicoQueue<SystemControllerCommand> *incomingQueue;
    uart_inst_t* uart;

    MovingAverage<float> brewTempAverage = MovingAverage<float>(5);
    MovingAverage<float> serviceTempAverage = MovingAverage<float>(5);

    LccParsedPacket currentLccParsedPacket;
    ControlBoardParsedPacket currentControlBoardParsedPacket;
    LccRawPacket currentLccRawPacket;
    ControlBoardRawPacket currentControlBoardRawPacket;
    uint8_t currentPacketIdx = 0;

    [[noreturn]] void bailForever();

    LccParsedPacket handleControlBoardPacket(ControlBoardParsedPacket packet);

    HybridController brewBoilerController;
    HybridController serviceBoilerController;

    PicoQueue<SsrState> ssrStateQueue = PicoQueue<SsrState>(25);

    TimedLatch waterTankEmptyLatch = TimedLatch(1000, false);
    TimedLatch serviceBoilerLowLatch = TimedLatch(500, false);

    void handleCommands();
    void updateControllerSettings();
};


#endif //FIRMWARE_SYSTEMCONTROLLER_H
