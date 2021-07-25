//
// Created by Magnus Nordlander on 2021-07-02.
//

#include "ControlBoardTransceiver.h"
#include <hardware/gpio.h>
#include <pinDefinitions.h>
#include <utils/hex_format.h>

using namespace std::chrono_literals;

ControlBoardTransceiver::ControlBoardTransceiver(PinName tx, PinName rx, SystemStatus* status)
        : serial(tx, rx, 9600), status(status) {
    serial.set_flow_control(mbed::SerialBase::Disabled);
    gpio_set_inover(rx, GPIO_OVERRIDE_INVERT);
    gpio_set_outover(tx, GPIO_OVERRIDE_INVERT);
    serial.set_blocking(false);
    serial.attach([this] { handleRxIrq(); });
}

[[noreturn]] void ControlBoardTransceiver::run() {
    printf("Running\n");
    mbed::Timer t;
    while (true) {
        mbed::Watchdog::get_instance().kick();

        LccRawPacket rawLccPacket = convert_lcc_parsed_to_raw(status->lccPacket);

        printf("Sending LCC packet: ");
        printhex(reinterpret_cast<uint8_t*>(&rawLccPacket), sizeof(rawLccPacket));
        printf("\n");
        lastPacketSentAt = rtos::Kernel::Clock::now();
        status->lastLccPacketSentAt = lastPacketSentAt;
        status->hasSentLccPacket = true;

        sendPacket(rawLccPacket);

        t.start();
        awaitingPacket = true;

        do {
            awaitReceipt();
        } while (currentPacketIdx < sizeof(currentPacket) && t.elapsed_time() < 100ms);

        t.stop();
        awaitingPacket = false;

        if (t.elapsed_time() >= 100ms) {
            printf("Getting a packet from CB took too long (%u ms), bailing.\n", (uint16_t)std::chrono::duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count());
            bailForever();
        }

        rtos::Kernel::Clock::time_point receivedAt = rtos::Kernel::Clock::now();

        printf("CB packet received (took %u ms): ", (uint16_t)std::chrono::duration_cast<std::chrono::milliseconds>(t.elapsed_time()).count());
        printhex(reinterpret_cast<uint8_t*>(&currentPacket), sizeof(currentPacket));
        printf("\n");

        ControlBoardParsedPacket cbPacket = convert_raw_control_board_packet(currentPacket);
        status->controlBoardRawPacket = currentPacket;

        currentPacket = ControlBoardRawPacket();
        currentPacketIdx = 0;
        t.reset();

        status->controlBoardPacket = cbPacket;
        status->lastControlBoardPacketReceivedAt = receivedAt;
        status->hasReceivedControlBoardPacket = true;

        rtos::Kernel::Clock::time_point nextPacketAt = lastPacketSentAt + 100ms;

        if (rtos::Kernel::Clock::now() > (nextPacketAt + 500ms)) {
            printf("We're too far behind on packets, bailing.\n");
            bailForever();
        }

        rtos::ThisThread::sleep_until(nextPacketAt);
    }
}

[[noreturn]] void ControlBoardTransceiver::bailForever() {
    status->has_bailed = true;

    while (true) {
        rtos::ThisThread::sleep_for(100ms);

        LccRawPacket safePacket = {0x80, 0, 0, 0, 0};
        printf("Bailed, sending safe packet\n");
        serial.write((uint8_t *)&safePacket, sizeof(safePacket));

        mbed::Watchdog::get_instance().kick();
    }
}

void ControlBoardTransceiver::handleRxIrq() {
    if (!awaitingPacket || currentPacketIdx >= sizeof(currentPacket)) {
        // Just clear the IRQ
        uint8_t buf;
        serial.read(&buf, 1);
    } else {
        auto* buf = reinterpret_cast<uint8_t*>(&currentPacket);
        serial.read(buf+currentPacketIdx++, 1);
    }
}

void ControlBoardTransceiver::sendPacket(LccRawPacket rawLccPacket) {
#ifndef EMULATED
    serial.write((uint8_t *)&rawLccPacket, sizeof(rawLccPacket));
#else
    emulatedControlBoard.update(rtos::Kernel::Clock::now(), rawLccPacket);
#endif
}

void ControlBoardTransceiver::awaitReceipt() {
#ifndef EMULATED
    rtos::ThisThread::sleep_for(5ms);
#else
    rtos::ThisThread::sleep_for(60ms);
    currentPacket = emulatedControlBoard.latestPacket();
    currentPacketIdx = 18;
#endif
}
