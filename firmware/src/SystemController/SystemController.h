//
// Created by Magnus Nordlander on 2021-07-06.
//

#ifndef FIRMWARE_SYSTEMCONTROLLER_H
#define FIRMWARE_SYSTEMCONTROLLER_H


#include <SystemStatus.h>
#include "TimedLatch.h"
#include "HysteresisController.h"
#include "HybridController.h"

typedef enum {
    UNDETERMINED,
    COLD_BOOT,
    WARM_BOOT,
    RUNNING,
    SLEEPING,
} SystemState;

typedef enum {
    BOTH_SSRS_OFF = 0,
    BREW_BOILER_SSR_ON,
    SERVICE_BOILER_SSR_ON,
} SsrState;

class SsrStateQueueItem {
public:
    SsrState state = BOTH_SSRS_OFF;
    rtos::Kernel::Clock::time_point expiresAt;
};

class SystemController {
public:
    explicit SystemController(PinName tx, PinName rx, SystemStatus *status);

    [[noreturn]] void run();
    void handleRxIrq();
private:
    mbed::UnbufferedSerial serial;
    SystemStatus* status;

    rtos::Kernel::Clock::time_point lastPacketSentAt;

    bool awaitingPacket = false;
    ControlBoardRawPacket currentPacket;
    uint8_t currentPacketIdx = 0;

    [[noreturn]] void bailForever();
    void sendPacket(LccRawPacket packet);
    void awaitReceipt();

    LccParsedPacket handleControlBoardPacket(ControlBoardParsedPacket packet);
    void updateFromSystemStatus();

    HybridController brewBoilerController;
    HybridController serviceBoilerController;

    rtos::Queue<SsrStateQueueItem, 50> ssrStateQueue;

    TimedLatch waterTankEmptyLatch = TimedLatch(1000, false);
    TimedLatch serviceBoilerLowLatch = TimedLatch(500, false);
};


#endif //FIRMWARE_SYSTEMCONTROLLER_H
