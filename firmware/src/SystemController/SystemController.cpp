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

        ControlBoardParsedPacket cbPacket = convert_raw_control_board_packet(currentPacket);
        status->controlBoardRawPacket = currentPacket;

        currentPacket = ControlBoardRawPacket();
        currentPacketIdx = 0;
        t.reset();

        status->controlBoardPacket = cbPacket;
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

    auto now = rtos::Kernel::Clock::now();
    nonstd::optional<SsrStateQueueItem*> foundItem;

    while (!ssrStateQueue.empty()) {
        SsrStateQueueItem* item;
        bool success = ssrStateQueue.try_get(&item);
        if (success && item->expiresAt > now) {
            foundItem = item;
            break;
        }
        printf("Skipping queue item\n");
        delete item;
    }

    /*
     * New algorithm:
     *
     * Cap BB and SB at 25 respectively. Divide time into 25 x 100 ms slots.
     *
     * If BB + SB < 25: Both get what they want.
     * Else: They get a proportional share of what they want.
     *   I.e. if BB = 17 and SB = 13, BB gets round((17/(17+13))*25) = 14 and SB gets round((13/(17+13))*25) = 11.
     *
     * Each slot allocation is stamped with an expiration timestamp. That way any delay in popping the queue won't
     * result in old values being used.
     */
    if (ssrStateQueue.empty()) {
        uint8_t bbSignal = brewBoilerController.getControlSignal(latestParsedPacket.brew_boiler_temperature);
        uint8_t sbSignal = serviceBoilerController.getControlSignal(latestParsedPacket.service_boiler_temperature);

        printf("Raw signals. BB: %u SB: %u\n", bbSignal, sbSignal);

        if (status->ecoMode) {
            sbSignal = 0;
        }

        if (bbSignal + sbSignal > 25) {
            auto newBbSignal = (uint8_t)round(((float)bbSignal / (float)(bbSignal + sbSignal)) * 25.f);
            auto newSbSignal = (uint8_t)round(((float)sbSignal / (float)(bbSignal + sbSignal)) * 25.f);
            bbSignal = newBbSignal;
            sbSignal = newSbSignal;

            if (bbSignal + sbSignal == 26) {
                sbSignal--;
            }
        }

        uint8_t noSignal = 25 - bbSignal - sbSignal;

        printf("Adding new controls to the queue. BB: %u SB: %u NB: %u\n", bbSignal, sbSignal, noSignal);

        auto slotExpiration = now + 200ms;

        for (uint8_t i = 0; i < bbSignal; ++i) {
            auto* newItem = new SsrStateQueueItem;
            newItem->state = BREW_BOILER_SSR_ON;
            newItem->expiresAt = slotExpiration;
            slotExpiration += 100ms;
            ssrStateQueue.try_put(newItem);
        }

        for (uint8_t i = 0; i < sbSignal; ++i) {
            auto* newItem = new SsrStateQueueItem;
            newItem->state = SERVICE_BOILER_SSR_ON;
            newItem->expiresAt = slotExpiration;
            slotExpiration += 100ms;
            ssrStateQueue.try_put(newItem);
        }

        for (uint8_t i = 0; i < noSignal; ++i) {
            auto* newItem = new SsrStateQueueItem;
            newItem->state = BOTH_SSRS_OFF;
            newItem->expiresAt = slotExpiration;
            slotExpiration += 100ms;
            ssrStateQueue.try_put(newItem);
        }

        if (!foundItem.has_value()) {
            SsrStateQueueItem* item;
            bool success = ssrStateQueue.try_get(&item);
            if (success) {
                foundItem = item;
            }
        }
    }

    if (!foundItem.has_value()) {
        printf("No Queue item found\n");
        bailForever();
    }

    if (foundItem.value()->state == BREW_BOILER_SSR_ON) {
        lcc.brew_boiler_ssr_on = true;
    } else if (foundItem.value()->state == SERVICE_BOILER_SSR_ON) {
        lcc.service_boiler_ssr_on = true;
    }

    delete foundItem.value();
    foundItem.reset();

    status->p = brewBoilerController.pidController.Pout;
    status->i = brewBoilerController.pidController.Iout;
    status->d = brewBoilerController.pidController.Dout;
    status->integral = brewBoilerController.pidController._integral;

    if (!waterTankEmptyLatch.get()) {
        if (latestParsedPacket.brew_switch) {
            lcc.pump_on = true;

            if (!status->currentlyBrewing) {
                status->currentlyBrewing = true;
                status->lastBrewStartedAt = rtos::Kernel::Clock::now();
                status->lastBrewEndedAt.reset();
            }
        } else if (serviceBoilerLowLatch.get()) {
            lcc.pump_on = true;
            lcc.service_boiler_solenoid_open = true;
        }
    }

    if (!lcc.pump_on && status->currentlyBrewing) {
        status->currentlyBrewing = false;
        status->lastBrewEndedAt = rtos::Kernel::Clock::now();
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
