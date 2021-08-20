//
// Created by Magnus Nordlander on 2021-07-06.
//

#include "SystemController.h"
#include "hardware/watchdog.h"
#include "pico/timeout_helper.h"

using namespace std::chrono_literals;

static inline bool uart_read_blocking_timeout(uart_inst_t *uart, uint8_t *dst, size_t len, check_timeout_fn timeout_check, struct timeout_state *ts) {
    for (size_t i = 0; i < len; ++i) {
        while (!uart_is_readable(uart)) {
            if (timeout_check(ts)) {
                return false;
            }

            tight_loop_contents();
        }
        *dst++ = uart_get_hw(uart)->dr;
    }

    return true;
}

SystemController::SystemController(
        uart_inst_t * _uart,
        PinName tx,
        PinName rx,
        PicoQueue<SystemControllerStatusMessage> *outgoingQueue,
        PicoQueue<SystemControllerCommand> *incomingQueue)
        :
        brewBoilerController(targetBrewTemperature, 20.0f, brewPidParameters, 2.0f, 200),
        serviceBoilerController(targetServiceTemperature, 20.0f, servicePidParameters, 2.0f, 800),
        outgoingQueue(outgoingQueue),
        incomingQueue(incomingQueue),
        uart(_uart) {
    // hardware/uart
    gpio_set_function(rx, GPIO_FUNC_UART);
    gpio_set_inover(rx, GPIO_OVERRIDE_INVERT);
    gpio_set_function(tx, GPIO_FUNC_UART);
    gpio_set_outover(tx, GPIO_OVERRIDE_INVERT);

    uart_init(uart, 9600);
}

void SystemController::run() {
    printf("Running\n");
    //watchdog_enable(1000, true);

    currentLccParsedPacket = LccParsedPacket();

    //mbed::Timer t;
    while (true) {
        // hardware/watchdog
        // RTOS on Core0 doesn't like us entering critical sections on Core1,
        // so we can't use mbed::Watchdog.
        hal_watchdog_kick();

        LccRawPacket rawLccPacket = convert_lcc_parsed_to_raw(currentLccParsedPacket);
        uart_write_blocking(uart, (uint8_t *)&rawLccPacket, sizeof(rawLccPacket));

        auto timeout = make_timeout_time_ms(100);

        timeout_state_t ts;
        bool success = uart_read_blocking_timeout(uart, reinterpret_cast<uint8_t *>(&currentControlBoardRawPacket), sizeof(currentControlBoardRawPacket), init_single_timeout_until(&ts, timeout), &ts);

        if (!success) {
            bail_reason = BAIL_REASON_CB_UNRESPONSIVE;
            bailForever();
        }

        printf("Got package\n");
        currentControlBoardParsedPacket = convert_raw_control_board_packet(currentControlBoardRawPacket);

        currentControlBoardRawPacket = ControlBoardRawPacket();
        currentPacketIdx = 0;

        currentLccParsedPacket = handleControlBoardPacket(currentControlBoardParsedPacket);

        SystemControllerStatusMessage message = {
                .timestamp = get_absolute_time(),
                .brewTemperature = currentControlBoardParsedPacket.brew_boiler_temperature,
                .brewSetPoint = targetBrewTemperature,
                .brewPidSettings = brewPidParameters,
                .brewPidParameters = brewPidRuntimeParameters,
                .serviceTemperature = currentControlBoardParsedPacket.service_boiler_temperature,
                .serviceSetPoint = targetServiceTemperature,
                .servicePidSettings = servicePidParameters,
                .servicePidParameters = servicePidRuntimeParameters,
                .ecoMode = ecoMode,
                .state = SYSTEM_CONTROLLER_STATE_WARM,
                .bailReason = bail_reason,
                .currentlyBrewing = currentControlBoardParsedPacket.brew_switch,
                .currentlyFillingServiceBoiler = false,
                .waterTankLow = currentControlBoardParsedPacket.water_tank_empty,
        };

        if (!outgoingQueue->isFull()) {
            outgoingQueue->tryAdd(&message);
        }

        sleep_until(timeout);
    }
}

LccParsedPacket SystemController::handleControlBoardPacket(ControlBoardParsedPacket latestParsedPacket) {
    LccParsedPacket lcc;

    waterTankEmptyLatch.set(latestParsedPacket.water_tank_empty);
    serviceBoilerLowLatch.set(latestParsedPacket.service_boiler_low);

    auto now = get_absolute_time();
    nonstd::optional<SsrStateQueueItem> foundItem;

    while (!ssrStateQueue.empty()) {
        SsrStateQueueItem item = ssrStateQueue.front();
        ssrStateQueue.pop();
        if (absolute_time_diff_us(now, item.expiresAt) > 0) {
            foundItem = item;
            break;
        }
        printf("Skipping queue item, expires at %lu, current %lu, diff %lu\n", (unsigned long)to_ms_since_boot(item.expiresAt), (unsigned long)to_ms_since_boot(now), (unsigned long)absolute_time_diff_us(now, item.expiresAt));
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

        if (ecoMode) {
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

        auto slotExpiration = delayed_by_ms(now, 200);

        for (uint8_t i = 0; i < bbSignal; ++i) {
            auto newItem = SsrStateQueueItem();
            newItem.state = BREW_BOILER_SSR_ON;
            newItem.expiresAt = slotExpiration;
            slotExpiration = delayed_by_ms(slotExpiration, 100);
            ssrStateQueue.push(newItem);
        }

        for (uint8_t i = 0; i < sbSignal; ++i) {
            auto newItem = SsrStateQueueItem();
            newItem.state = SERVICE_BOILER_SSR_ON;
            newItem.expiresAt = slotExpiration;
            slotExpiration = delayed_by_ms(slotExpiration, 100);
            ssrStateQueue.push(newItem);
        }

        for (uint8_t i = 0; i < noSignal; ++i) {
            auto newItem = SsrStateQueueItem();
            newItem.state = BOTH_SSRS_OFF;
            newItem.expiresAt = slotExpiration;
            slotExpiration = delayed_by_ms(slotExpiration, 100);
            ssrStateQueue.push(newItem);
        }

        if (!foundItem.has_value()) {
            SsrStateQueueItem item = ssrStateQueue.front();
            foundItem = item;
        }
    }

    if (!foundItem.has_value()) {
        printf("No Queue item found\n");
        bail_reason = BAIL_REASON_SSR_QUEUE_EMPTY;
        bailForever();
    }

    if (foundItem.value().state == BREW_BOILER_SSR_ON) {
        lcc.brew_boiler_ssr_on = true;
    } else if (foundItem.value().state == SERVICE_BOILER_SSR_ON) {
        lcc.service_boiler_ssr_on = true;
    }

    foundItem.reset();

    brewPidRuntimeParameters.p = brewBoilerController.pidController.Pout;
    brewPidRuntimeParameters.i = brewBoilerController.pidController.Iout;
    brewPidRuntimeParameters.d = brewBoilerController.pidController.Dout;
    brewPidRuntimeParameters.integral = brewBoilerController.pidController._integral;

    if (!waterTankEmptyLatch.get()) {
        if (latestParsedPacket.brew_switch) {
            lcc.pump_on = true;

/*            if (!status->currentlyBrewing) {
                status->currentlyBrewing = true;
                status->lastBrewStartedAt = rtos::Kernel::Clock::now();
                status->lastBrewEndedAt.reset();
            }*/
        } else if (serviceBoilerLowLatch.get()) {
            lcc.pump_on = true;
            lcc.service_boiler_solenoid_open = true;
        }
    }

/*    if (!lcc.pump_on && status->currentlyBrewing) {
        status->currentlyBrewing = false;
        status->lastBrewEndedAt = rtos::Kernel::Clock::now();
    }*/

    return lcc;
}

[[noreturn]] void SystemController::bailForever() {
    has_bailed = true;

    while (true) {
        sleep_ms(100);

        LccRawPacket safePacket = {0x80, 0, 0, 0, 0};
        printf("Bailed, sending safe packet\n");
        uart_write_blocking(uart, (uint8_t *)&safePacket, sizeof(safePacket));

        hal_watchdog_kick();
    }
}