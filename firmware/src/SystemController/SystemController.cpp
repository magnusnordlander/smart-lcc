//
// Created by Magnus Nordlander on 2021-07-06.
//

#include "SystemController.h"

using namespace std::chrono_literals;

SystemController::SystemController(PinName tx, PinName rx, SystemStatus *status) :
        serial(tx, rx, 9600),
        status(status),
        brewBoilerController(status->targetBrewTemperature, 20.0f, status->brewPidParameters, 2.0f, 200),
        serviceBoilerController(status->targetServiceTemperature, 20.0f, status->servicePidParameters, 2.0f, 800){
    serial.set_flow_control(mbed::SerialBase::Disabled);
    gpio_set_inover(rx, GPIO_OVERRIDE_INVERT);
    gpio_set_outover(tx, GPIO_OVERRIDE_INVERT);
    serial.set_blocking(false);
    serial.attach([this] { handleRxIrq(); });
}

void SystemController::run() {
    printf("Running\n");
    mbed::Timer t;
    while (true) {
        mbed::Watchdog::get_instance().kick();
        updateFromSystemStatus();

        LccRawPacket rawLccPacket = convert_lcc_parsed_to_raw(status->lccPacket);

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

        status->lccPacket = handleControlBoardPacket(status->controlBoardPacket);

        rtos::ThisThread::sleep_until(nextPacketAt);
    }
}

LccParsedPacket SystemController::handleControlBoardPacket(ControlBoardParsedPacket latestParsedPacket) {
    LccParsedPacket lcc;

    waterTankEmptyLatch.set(latestParsedPacket.water_tank_empty);
    serviceBoilerLowLatch.set(latestParsedPacket.service_boiler_low);

    if (brewBoilerController.getControlSignal(latestParsedPacket.brew_boiler_temperature)) {
        lcc.brew_boiler_ssr_on = true;
    }

    status->p = brewBoilerController.pidController.Pout;
    status->i = brewBoilerController.pidController.Iout;
    status->d = brewBoilerController.pidController.Dout;
    status->integral = brewBoilerController.pidController._integral;

    if (!lcc.brew_boiler_ssr_on && serviceBoilerController.getControlSignal(latestParsedPacket.service_boiler_temperature) && !status->ecoMode) {
        lcc.service_boiler_ssr_on = true;
    }

    if (!waterTankEmptyLatch.get()) {
        if (latestParsedPacket.brew_switch) {
            lcc.pump_on = true;
        } else if (serviceBoilerLowLatch.get()) {
            lcc.pump_on = true;
            lcc.service_boiler_solenoid_open = true;
        }
    }

    return lcc;
}

void SystemController::updateFromSystemStatus() {
    brewBoilerController.updateSetPoint(status->targetBrewTemperature);
    serviceBoilerController.updateSetPoint(status->targetServiceTemperature);
    brewBoilerController.setPidParameters(status->brewPidParameters);
}

[[noreturn]] void SystemController::bailForever() {
    status->has_bailed = true;

    while (true) {
        rtos::ThisThread::sleep_for(100ms);

        LccRawPacket safePacket = {0x80, 0, 0, 0, 0};
        //printf("Bailed, sending safe packet\n");
        serial.write((uint8_t *)&safePacket, sizeof(safePacket));

        mbed::Watchdog::get_instance().kick();
    }
}

void SystemController::handleRxIrq() {
    if (!awaitingPacket || currentPacketIdx >= sizeof(currentPacket)) {
        // Just clear the IRQ
        uint8_t buf;
        serial.read(&buf, 1);
    } else {
        auto* buf = reinterpret_cast<uint8_t*>(&currentPacket);
        serial.read(buf+currentPacketIdx++, 1);
    }
}

void SystemController::sendPacket(LccRawPacket rawLccPacket) {
#ifndef EMULATED
    serial.write((uint8_t *)&rawLccPacket, sizeof(rawLccPacket));
#else
    emulatedControlBoard.update(rtos::Kernel::Clock::now(), rawLccPacket);
#endif
}

void SystemController::awaitReceipt() {
#ifndef EMULATED
    rtos::ThisThread::sleep_for(5ms);
#else
    rtos::ThisThread::sleep_for(60ms);
    currentPacket = emulatedControlBoard.latestPacket();
    currentPacketIdx = 18;
#endif
}
