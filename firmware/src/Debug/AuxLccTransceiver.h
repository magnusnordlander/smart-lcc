//
// Created by Magnus Nordlander on 2021-07-16.
//

#ifndef FIRMWARE_AUXLCCTRANSCEIVER_H
#define FIRMWARE_AUXLCCTRANSCEIVER_H

#include <mbed.h>
#include <SystemStatus.h>

class AuxLccTransceiver {
public:
    AuxLccTransceiver(PinName tx, PinName rx, SystemStatus* status);

    [[noreturn]] void run();
    void handleRxIrq();
private:
    mbed::UnbufferedSerial serial;
    SystemStatus* status;

    rtos::Kernel::Clock::time_point lastByteAt;

    LccRawPacket currentPacket;
    uint8_t currentPacketIdx = 0;
};


#endif //FIRMWARE_AUXLCCTRANSCEIVER_H
