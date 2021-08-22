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
    HEATUP_STAGE_1, // Bring the Brew boiler up to 130, don't run the service boiler
    HEATUP_STAGE_2, // Keep the Brew boiler at 130 for 4 minutes, run service boiler as normal
    RUNNING,
    SLEEPING,
    SOFT_BAILED,
    HARD_BAILED,
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
    SystemControllerBailReason bail_reason = BAIL_REASON_NONE;
    SystemControllerInternalState internalState = UNDETERMINED;

    nonstd::optional<absolute_time_t> unbailTimer{};
    nonstd::optional<absolute_time_t> heatupStage2Timer{};

    bool ecoMode = true;
    float targetBrewTemperature = 0.f;
    float targetServiceTemperature = 0.f;
    PidSettings brewPidParameters = PidSettings{.Kp = 0.f, .Ki = 0.f, .Kd = 0.f, .windupLow = -1.f, .windupHigh = 1.f};
    PidSettings servicePidParameters = PidSettings{.Kp = 0.f, .Ki = 0.f, .Kd = 0.f, .windupLow = -1.f, .windupHigh = 1.f};

    PidRuntimeParameters brewPidRuntimeParameters{};
    PidRuntimeParameters servicePidRuntimeParameters{};

    PicoQueue<SystemControllerStatusMessage> *outgoingQueue;
    PicoQueue<SystemControllerCommand> *incomingQueue;
    uart_inst_t* uart;

    MovingAverage<float> brewTempAverage = MovingAverage<float>(5);
    MovingAverage<float> serviceTempAverage = MovingAverage<float>(5);

    bool core0Alive = true;

    LccParsedPacket currentLccParsedPacket;
    ControlBoardParsedPacket currentControlBoardParsedPacket;
    ControlBoardRawPacket currentControlBoardRawPacket;

    SystemControllerState externalState();

    void softBail(SystemControllerBailReason reason);
    void hardBail(SystemControllerBailReason reason);
    inline bool isBailed() { return internalState == SOFT_BAILED || internalState == HARD_BAILED; }
    inline bool isSoftBailed() { return internalState == SOFT_BAILED; };
    void unbail();

    inline bool onlySendSafePackages() { return isBailed() || internalState == UNDETERMINED; }

    inline bool sleepModeChangePossible() { return !isBailed() && internalState != UNDETERMINED; };
    void setSleepMode(bool sleepMode);

    bool areTemperaturesAtSetPoint() const;

    void initiateHeatup();
    void transitionToHeatupStage2();
    void finishHeatup();

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
