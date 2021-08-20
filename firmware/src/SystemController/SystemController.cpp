//
// Created by Magnus Nordlander on 2021-07-06.
//

#include <utils/hex_format.h>
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
        outgoingQueue(outgoingQueue),
        incomingQueue(incomingQueue),
        uart(_uart),
        brewBoilerController(targetBrewTemperature, 20.0f, brewPidParameters, 2.0f, 200),
        serviceBoilerController(targetServiceTemperature, 20.0f, servicePidParameters, 2.0f, 800){
    // hardware/uart
    gpio_set_function(rx, GPIO_FUNC_UART);
    gpio_set_inover(rx, GPIO_OVERRIDE_INVERT);
    gpio_set_function(tx, GPIO_FUNC_UART);
    gpio_set_outover(tx, GPIO_OVERRIDE_INVERT);

    uart_init(uart, 9600);
}

void SystemController::run() {
    //printf("Running\n");
    //watchdog_enable(1000, true);

    currentLccParsedPacket = LccParsedPacket();

    handleCommands();

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

//        printf("Got package\n");
//        printlnhex((uint8_t*)&currentControlBoardRawPacket, sizeof(currentControlBoardRawPacket));
        currentControlBoardParsedPacket = convert_raw_control_board_packet(currentControlBoardRawPacket);

        currentControlBoardRawPacket = ControlBoardRawPacket();
        currentPacketIdx = 0;

        handleCommands();
        currentLccParsedPacket = handleControlBoardPacket(currentControlBoardParsedPacket);

        SystemControllerStatusMessage message = {
                .timestamp = get_absolute_time(),
                .brewTemperature = static_cast<float>(brewTempAverage.average()),
                .brewSetPoint = targetBrewTemperature,
                .brewPidSettings = brewPidParameters,
                .brewPidParameters = brewPidRuntimeParameters,
                .serviceTemperature = static_cast<float>(serviceTempAverage.average()),
                .serviceSetPoint = targetServiceTemperature,
                .servicePidSettings = servicePidParameters,
                .servicePidParameters = servicePidRuntimeParameters,
                .brewSSRActive = currentLccParsedPacket.brew_boiler_ssr_on,
                .serviceSSRActive = currentLccParsedPacket.service_boiler_ssr_on,
                .ecoMode = ecoMode,
                .state = SYSTEM_CONTROLLER_STATE_WARM,
                .bailReason = bail_reason,
                .currentlyBrewing = currentControlBoardParsedPacket.brew_switch && currentLccParsedPacket.pump_on,
                .currentlyFillingServiceBoiler = currentLccParsedPacket.pump_on && currentLccParsedPacket.service_boiler_solenoid_open,
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

    brewTempAverage.addValue(latestParsedPacket.brew_boiler_temperature);
    serviceTempAverage.addValue(latestParsedPacket.service_boiler_temperature);

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
    if (ssrStateQueue.isEmpty()) {
        uint8_t bbSignal = brewBoilerController.getControlSignal(brewTempAverage.average());
        uint8_t sbSignal = serviceBoilerController.getControlSignal(serviceTempAverage.average());

        //printf("Raw signals. BB: %u SB: %u\n", bbSignal, sbSignal);

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

        //printf("Adding new controls to the queue. BB: %u SB: %u NB: %u\n", bbSignal, sbSignal, noSignal);

        for (uint8_t i = 0; i < bbSignal; ++i) {
            SsrState state = BREW_BOILER_SSR_ON;
            ssrStateQueue.tryAdd(&state);
        }

        for (uint8_t i = 0; i < sbSignal; ++i) {
            SsrState state = SERVICE_BOILER_SSR_ON;
            ssrStateQueue.tryAdd(&state);
        }

        for (uint8_t i = 0; i < noSignal; ++i) {
            SsrState state = BOTH_SSRS_OFF;
            ssrStateQueue.tryAdd(&state);
        }
    }

    SsrState state;
    if (!ssrStateQueue.tryRemove(&state)) {
        //printf("No Queue item found\n");
        bail_reason = BAIL_REASON_SSR_QUEUE_EMPTY;
        bailForever();
    }

    if (state == BREW_BOILER_SSR_ON) {
        lcc.brew_boiler_ssr_on = true;
    } else if (state == SERVICE_BOILER_SSR_ON) {
        lcc.service_boiler_ssr_on = true;
    }

    brewPidRuntimeParameters.p = brewBoilerController.pidController.Pout;
    brewPidRuntimeParameters.i = brewBoilerController.pidController.Iout;
    brewPidRuntimeParameters.d = brewBoilerController.pidController.Dout;
    brewPidRuntimeParameters.integral = brewBoilerController.pidController._integral;

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

[[noreturn]] void SystemController::bailForever() {
    has_bailed = true;

    while (true) {
        sleep_ms(100);

        LccRawPacket safePacket = {0x80, 0, 0, 0, 0};
        //printf("Bailed, sending safe packet\n");
        uart_write_blocking(uart, (uint8_t *)&safePacket, sizeof(safePacket));

        hal_watchdog_kick();
    }
}

void SystemController::handleCommands() {
    SystemControllerCommand command;
    while (!incomingQueue->isEmpty()) {
        incomingQueue->removeBlocking(&command);

        //printf("Handling command of type %u. F1: %.1f F2 %.1f F3 %.1f B1: %u\n", (uint8_t)command.type, command.float1, command.float2, command.float3, command.bool1);

        switch (command.type) {
            case COMMAND_SET_BREW_SET_POINT:
                targetBrewTemperature = command.float1;
                break;
            case COMMAND_SET_BREW_PID_PARAMETERS:
                brewPidParameters.Kp = command.float1;
                brewPidParameters.Ki = command.float2;
                brewPidParameters.Kd = command.float3;
                break;
            case COMMAND_SET_SERVICE_SET_POINT:
                targetServiceTemperature = command.float1;
                break;
            case COMMAND_SET_SERVICE_PID_PARAMETERS:
                servicePidParameters.Kp = command.float1;
                servicePidParameters.Ki = command.float2;
                servicePidParameters.Kd = command.float3;
                break;
            case COMMAND_SET_ECO_MODE:
                ecoMode = command.bool1;
                break;
            case COMMAND_SET_SLEEP_MODE:
                break;
            case COMMAND_UNBAIL:
                break;
            case COMMAND_TRIGGER_FIRST_RUN:
                break;
        }
    }

    updateControllerSettings();
}

void SystemController::updateControllerSettings() {
    brewBoilerController.setPidParameters(brewPidParameters);
    brewBoilerController.updateSetPoint(targetBrewTemperature);
    serviceBoilerController.setPidParameters(servicePidParameters);
    serviceBoilerController.updateSetPoint(targetServiceTemperature);
}