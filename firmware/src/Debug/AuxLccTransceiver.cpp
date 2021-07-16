//
// Created by Magnus Nordlander on 2021-07-16.
//

#include "AuxLccTransceiver.h"

using namespace std::chrono_literals;

AuxLccTransceiver::AuxLccTransceiver(PinName tx, PinName rx, SystemStatus *status):
    serial(tx, rx, 9600), status(status) {
    serial.set_flow_control(mbed::SerialBase::Disabled);
    gpio_set_inover(rx, GPIO_OVERRIDE_INVERT);
    gpio_set_outover(tx, GPIO_OVERRIDE_INVERT);
    serial.set_blocking(false);
    serial.attach([this] { handleRxIrq(); });
}

void AuxLccTransceiver::run() {
    while (true) {
        if (currentPacketIdx >= sizeof(LccRawPacket)) {
            ControlBoardRawPacket packet = convert_parsed_control_board_packet(status->controlBoardPacket);

            serial.write((uint8_t *)&packet, sizeof(packet));

            currentPacket = LccRawPacket();
            currentPacketIdx = 0;
        }

        rtos::ThisThread::sleep_for(10ms);
    }
}

void AuxLccTransceiver::handleRxIrq() {
    uint8_t buf;
    serial.read(&buf, 1);

    if (currentPacketIdx < sizeof(currentPacket)) {
        if (currentPacketIdx > 0 || buf == 0x81) {
            auto* currentPacketBuf = reinterpret_cast<uint8_t*>(&currentPacket);
            currentPacketBuf[currentPacketIdx++] = buf;
        }
    }
}
