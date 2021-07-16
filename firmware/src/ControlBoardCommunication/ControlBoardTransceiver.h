//
// Created by Magnus Nordlander on 2021-07-02.
//

#ifndef FIRMWARE_CONTROLBOARDTRANSCEIVER_H
#define FIRMWARE_CONTROLBOARDTRANSCEIVER_H

#include <ControlBoardCommunication/Emulator/BiancaControlBoard.h>
#include "mbed.h"
#include "control_board_protocol.h"
#include "lcc_protocol.h"
#include "../SystemStatus.h"

class ControlBoardTransceiver {
public:
    ControlBoardTransceiver(PinName tx, PinName rx, SystemStatus* status);

    [[noreturn]] void run();
    void handleRxIrq();
private:
#ifdef EMULATED
    BiancaControlBoard emulatedControlBoard = BiancaControlBoard();
#endif
    mbed::UnbufferedSerial serial;
    SystemStatus* status;

    rtos::Kernel::Clock::time_point lastPacketSentAt;

    bool awaitingPacket = false;
    ControlBoardRawPacket currentPacket;
    uint8_t currentPacketIdx = 0;

    [[noreturn]] void bailForever();
    void sendPacket(LccRawPacket packet);
    void awaitReceipt();
};


#endif //FIRMWARE_CONTROLBOARDTRANSCEIVER_H
