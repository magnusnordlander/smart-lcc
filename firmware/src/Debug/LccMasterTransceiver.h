//
// Created by Magnus Nordlander on 2021-07-27.
//

#ifndef FIRMWARE_LCCMASTERTRANSCEIVER_H
#define FIRMWARE_LCCMASTERTRANSCEIVER_H

#include <mbed.h>
#include <SystemStatus.h>

class LccMasterTransceiver {
public:
    LccMasterTransceiver(PinName cbTx, PinName cbRx, PinName lccTx, PinName lccRx, SystemStatus* status);

    [[noreturn]] void run();
    void handleCbRxIrq();
    void handleLccRxIrq();
private:
    mbed::UnbufferedSerial cbSerial;
    mbed::UnbufferedSerial lccSerial;
    SystemStatus* status;

    rtos::Kernel::Clock::time_point lastCbPacketSentAt;
    rtos::Kernel::Clock::time_point lastLccPacketSentAt;

    bool awaitingCbPacket = false;
    ControlBoardRawPacket currentCbPacket;
    uint8_t currentCbPacketIdx = 0;

    bool awaitingLccPacket = false;
    LccRawPacket currentLccPacket;
    uint8_t currentLccPacketIdx = 0;

    bool lastBssrState = false;
    bool lastSssrState = false;
    rtos::Kernel::Clock::time_point bssrStateChangedAt;
    rtos::Kernel::Clock::time_point sssrStateChangedAt;

    rtos::Kernel::Clock::duration minBssrOnTime = std::chrono::milliseconds(10000);
    rtos::Kernel::Clock::duration minBssrOffTime = std::chrono::milliseconds(10000);
    rtos::Kernel::Clock::duration minSssrOnTime = std::chrono::milliseconds(10000);
    rtos::Kernel::Clock::duration minSssrOffTime = std::chrono::milliseconds(10000);

    [[noreturn]] void bailForever();
};


#endif //FIRMWARE_LCCMASTERTRANSCEIVER_H
