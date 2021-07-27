//
// Created by Magnus Nordlander on 2021-07-27.
//

#include "LccMasterTransceiver.h"
#include <utils/hex_format.h>


using namespace std::chrono_literals;

LccMasterTransceiver::LccMasterTransceiver(PinName cbTx, PinName cbRx, PinName lccTx, PinName lccRx, SystemStatus* status)
        : cbSerial(cbTx, cbRx, 9600), lccSerial(lccTx, lccRx, 9600), status(status) {
    cbSerial.set_flow_control(mbed::SerialBase::Disabled);
    gpio_set_inover(cbRx, GPIO_OVERRIDE_INVERT);
    gpio_set_outover(cbTx, GPIO_OVERRIDE_INVERT);
    cbSerial.set_blocking(false);
    cbSerial.attach([this] { handleCbRxIrq(); });

    lccSerial.set_flow_control(mbed::SerialBase::Disabled);
    gpio_set_inover(lccRx, GPIO_OVERRIDE_INVERT);
    gpio_set_outover(lccTx, GPIO_OVERRIDE_INVERT);
    lccSerial.set_blocking(false);
    lccSerial.attach([this] { handleLccRxIrq(); });
}

[[noreturn]] void LccMasterTransceiver::run() {
    printf("Running\n");
    mbed::Timer t;
    while (true) {
        mbed::Watchdog::get_instance().kick();

        t.start();
        awaitingLccPacket = true;

        do {
            rtos::ThisThread::sleep_for(5ms);
        } while (currentLccPacketIdx < sizeof(currentLccPacket) && t.elapsed_time() < 5s);
        awaitingLccPacket = false;
        t.stop();

        if (t.elapsed_time() >= 5s) {
            printf("Getting a packet from LCC took too long (%u ms), got %u bytes, bailing.\n", (uint16_t)std::chrono::duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count(), currentCbPacketIdx);
            bailForever();
        }

/*        printf("Got packet from LCC\n");
        printlnhex((uint8_t *)&currentLccPacket, sizeof(currentLccPacket));*/

        t.reset();

        status->lastLccPacketSentAt = rtos::Kernel::Clock::now();
        status->hasSentLccPacket = true;
        status->lccPacket = convert_lcc_raw_to_parsed(currentLccPacket);

        cbSerial.write((uint8_t *)&currentLccPacket, sizeof(currentLccPacket));
        t.start();
        awaitingCbPacket = true;

        // Reset LCC Packet
        currentLccPacket = LccRawPacket();
        currentLccPacketIdx = 0;

        do {
            rtos::ThisThread::sleep_for(5ms);
        } while (currentCbPacketIdx < sizeof(currentCbPacket) && t.elapsed_time() < 100ms);

        t.stop();
        awaitingCbPacket = false;

        if (t.elapsed_time() >= 100ms) {
            printf("Getting a packet from CB took too long (%u ms), bailing.\n", (uint16_t)std::chrono::duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count());
            bailForever();
        }
        rtos::Kernel::Clock::time_point cbReceivedAt = rtos::Kernel::Clock::now();

        lccSerial.write((uint8_t*)&currentCbPacket, sizeof(currentCbPacket));

        status->controlBoardRawPacket = currentCbPacket;
        status->controlBoardPacket = convert_raw_control_board_packet(currentCbPacket);
        status->lastControlBoardPacketReceivedAt = cbReceivedAt;
        status->hasReceivedControlBoardPacket = true;

        // Reset CB Packet
        currentCbPacket = ControlBoardRawPacket();
        currentCbPacketIdx = 0;
    }
}

[[noreturn]] void LccMasterTransceiver::bailForever() {
    status->has_bailed = true;

    while (true) {
        rtos::ThisThread::sleep_for(100ms);

        LccRawPacket safePacket = {0x80, 0, 0, 0, 0};
        //printf("Bailed, sending safe packet\n");
        cbSerial.write((uint8_t *)&safePacket, sizeof(safePacket));

        mbed::Watchdog::get_instance().kick();
    }
}

void LccMasterTransceiver::handleCbRxIrq() {
    uint8_t buf;
    cbSerial.read(&buf, 1);

    if (awaitingCbPacket && currentCbPacketIdx < sizeof(currentCbPacket)) {
//        if (currentCbPacketIdx > 0 || buf == 0x81) {
            auto* currentPacketBuf = reinterpret_cast<uint8_t*>(&currentCbPacket);
            currentPacketBuf[currentCbPacketIdx++] = buf;
//        }
    }
}

void LccMasterTransceiver::handleLccRxIrq() {
    uint8_t buf;
    lccSerial.read(&buf, 1);

    if (awaitingLccPacket && currentLccPacketIdx < sizeof(currentLccPacket)) {
        if (currentLccPacketIdx > 0 || buf == 0x80) {
            auto* currentPacketBuf = reinterpret_cast<uint8_t*>(&currentLccPacket);
            currentPacketBuf[currentLccPacketIdx++] = buf;
        }
    }
}