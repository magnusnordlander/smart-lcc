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

typedef enum {
    BAIL_REASON_NONE = 0,
    BAIL_REASON_CB_UNRESPONSIVE,
    BAIL_REASON_TOO_FAR_BEHIND_ON_RESPONSE,
    BAIL_REASON_TOO_FAR_BEHIND_BETWEEN_PACKETS,
    BAIL_REASON_SSR_QUEUE_EMPTY,
} BailReason;

class SsrStateQueueItem {
public:
    SsrState state = BOTH_SSRS_OFF;
    absolute_time_t expiresAt;
};

class SystemController {
public:
    explicit SystemController(uart_inst_t * _uart, PinName tx, PinName rx, SystemStatus *status);

    [[noreturn]] void run();
private:
    SystemStatus* status;
    uart_inst_t* uart;

    ControlBoardRawPacket currentPacket;
    uint8_t currentPacketIdx = 0;

    [[noreturn]] void bailForever();

    LccParsedPacket handleControlBoardPacket(ControlBoardParsedPacket packet);
    void updateFromSystemStatus();

    HybridController brewBoilerController;
    HybridController serviceBoilerController;

    std::queue<SsrStateQueueItem> ssrStateQueue;

    TimedLatch waterTankEmptyLatch = TimedLatch(1000, false);
    TimedLatch serviceBoilerLowLatch = TimedLatch(500, false);
};


#endif //FIRMWARE_SYSTEMCONTROLLER_H
