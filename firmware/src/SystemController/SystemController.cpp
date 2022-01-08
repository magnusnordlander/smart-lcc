//
// Created by Magnus Nordlander on 2021-07-06.
//

#include <utils/hex_format.h>
#include "SystemController.h"
#include "hardware/watchdog.h"
#include "pico/timeout_helper.h"
#include "pico/multicore.h"

using namespace std::chrono_literals;

static inline bool uart_read_blocking_timeout(uart_inst_t *uart, uint8_t *dst, size_t len, absolute_time_t timeout_time) {
    timeout_state_t ts;
    check_timeout_fn timeout_check = init_single_timeout_until(&ts, timeout_time);

    for (size_t i = 0; i < len; ++i) {
        while (!uart_is_readable(uart)) {
            if (timeout_check(&ts)) {
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
        brewBoilerController(targetBrewTemperature, 20.0f, brewPidParameters, 2.0f),
        serviceBoilerController(targetServiceTemperature, 0.5f){
    // hardware/uart
    gpio_set_function(rx, GPIO_FUNC_UART);
    gpio_set_inover(rx, GPIO_OVERRIDE_INVERT);
    gpio_set_function(tx, GPIO_FUNC_UART);
    gpio_set_outover(tx, GPIO_OVERRIDE_INVERT);

    uart_init(uart, 9600);
}

void SystemController::run() {
    LccRawPacket safeLccRawPacket = create_safe_packet();
    currentLccParsedPacket = LccParsedPacket();

    handleCommands();

    while (true) {
        // hardware/watchdog
        // RTOS on Core0 doesn't like us entering critical sections on Core1,
        // so we can't use mbed::Watchdog.
        if (core0Alive) {
            hal_watchdog_kick();
        }

        LccRawPacket rawLccPacket = convert_lcc_parsed_to_raw(currentLccParsedPacket);
        if (validate_lcc_raw_packet(rawLccPacket)) {
            hardBail(BAIL_REASON_LCC_PACKET_INVALID);
        }

        if (onlySendSafePackages()) {
            uart_write_blocking(uart, (uint8_t *)&safeLccRawPacket, sizeof(safeLccRawPacket));
        } else {
            uart_write_blocking(uart, (uint8_t *)&rawLccPacket, sizeof(rawLccPacket));
        }

        // This timeout is used both as a timeout for reading from the UART and to know when to send the next packet.
        auto timeout = make_timeout_time_ms(100);

        bool success = uart_read_blocking_timeout(uart, reinterpret_cast<uint8_t *>(&currentControlBoardRawPacket), sizeof(currentControlBoardRawPacket), timeout);

        if (!success) {
            softBail(BAIL_REASON_CB_UNRESPONSIVE);
        }

        if (validate_raw_packet(currentControlBoardRawPacket)) {
            softBail(BAIL_REASON_CB_PACKET_INVALID);
        }

        if (isBailed()) {
            if (isSoftBailed()) {
                if (!success) {
                    unbailTimer.reset();
                } else if (!unbailTimer.has_value()) {
                    unbailTimer = get_absolute_time();
                } else if (absolute_time_diff_us(unbailTimer.value(), get_absolute_time()) > 2000000) {
                    unbail();
                }
            }
        } else {
            currentControlBoardParsedPacket = convert_raw_control_board_packet(currentControlBoardRawPacket);

            if (sleepModeRequested && internalState != SLEEPING) {
                setSleepMode(true);
            } else if (!sleepModeRequested && internalState == SLEEPING) {
                setSleepMode(false);
            }

            if (internalState == UNDETERMINED) {
                if (currentControlBoardParsedPacket.brew_boiler_temperature < 65) {
                    initiateHeatup();
                } else {
                    internalState = RUNNING;
                }
            } else if (internalState == HEATUP_STAGE_1) {
                if (currentControlBoardParsedPacket.brew_boiler_temperature > 128) {
                    transitionToHeatupStage2();
                }
            } else if (internalState == HEATUP_STAGE_2) {
                if (absolute_time_diff_us(heatupStage2Timer.value(), get_absolute_time()) > 4*60*1000*1000) {
                    finishHeatup();
                }
            }
        }

        // Reset the current raw packet.
        currentControlBoardRawPacket = ControlBoardRawPacket();

        handleCommands();

        if (!isBailed()) {
            currentLccParsedPacket = handleControlBoardPacket(currentControlBoardParsedPacket);
        } else {
            currentLccParsedPacket = convert_lcc_raw_to_parsed(safeLccRawPacket);
        }

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
                .state = externalState(),
                .bailReason = bail_reason,
                .currentlyBrewing = currentControlBoardParsedPacket.brew_switch && currentLccParsedPacket.pump_on,
                .currentlyFillingServiceBoiler = currentLccParsedPacket.pump_on && currentLccParsedPacket.service_boiler_solenoid_open,
                .waterTankLow = currentControlBoardParsedPacket.water_tank_empty,
        };

        if (!outgoingQueue->isFull()) {
            outgoingQueue->tryAdd(&message);
        } else {
            printf("Core0 isn't handling our messages :/ \n");
            core0Alive = false;
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

    bool brewing = false;

    if (!waterTankEmptyLatch.get()) {
        if (latestParsedPacket.brew_switch) {
            lcc.pump_on = true;
            brewing = true;
        } else if (serviceBoilerLowLatch.get()) {
            lcc.pump_on = true;
            lcc.service_boiler_solenoid_open = true;
        }
    }

    if (brewing && !brewStartedAt.has_value()) {
        brewStartedAt = get_absolute_time();
    } else if (!brewing && brewStartedAt.has_value()) {
        brewStartedAt.reset();
    }

    /*
     * New algorithm:
     *
     * Cap BB and SB at 25 respectively. Divide time into 25 x 100 ms slots.
     *
     * If BB + SB < 25: Both get what they want.
     * Else: They get a proportional share of what they want.
     *   I.e. if BB = 17 and SB = 13, BB gets round((17/(17+13))*25) = 14 and SB gets round((13/(17+13))*25) = 11.
     */
    if (ssrStateQueue.isEmpty()) {
        float feedForward = 0.0f;
        if (brewStartedAt.has_value()) {
            feedForward = feedForwardK * ((float)absolute_time_diff_us(brewStartedAt.value(), get_absolute_time()) / 1000.f) + feedForwardM;
        }

        uint8_t bbSignal = brewBoilerController.getControlSignal(
                brewTempAverage.average(),
                brewing ? feedForward : 0.f,
                shouldForceHysteresisForBrewBoiler()
                );
        uint8_t sbSignal = serviceBoilerController.getControlSignal(serviceTempAverage.average());

        printf("Raw signals. BB: %u SB: %u\n", bbSignal, sbSignal);

        if (ecoMode) {
            sbSignal = 0;
        }

        // Power sharing
        if (bbSignal + sbSignal > 25) {
            if (!brewing) {
                // If we're brewing, prioritize the brew boiler fully
                // Otherwise, give the brew boiler slightly less than 75% priority
                bbSignal = floor((float)bbSignal * 0.75);
            }
            sbSignal = 25 - bbSignal;
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
        hardBail(BAIL_REASON_SSR_QUEUE_EMPTY);
        return lcc;
    }

    if (state == BREW_BOILER_SSR_ON) {
        lcc.brew_boiler_ssr_on = true;
    } else if (state == SERVICE_BOILER_SSR_ON) {
        lcc.service_boiler_ssr_on = true;
    }

    brewPidRuntimeParameters = brewBoilerController.getRuntimeParameters();
    //servicePidRuntimeParameters = serviceBoilerController.getRuntimeParameters();

    return lcc;
}

void SystemController::handleCommands() {
    SystemControllerCommand command;
    //printf("Q: %u\n", incomingQueue->getLevelUnsafe());
    while (!incomingQueue->isEmpty()) {
        incomingQueue->removeBlocking(&command);

        switch (command.type) {
            case COMMAND_SET_BREW_SET_POINT:
                targetBrewTemperature = command.float1;
                break;
            case COMMAND_SET_BREW_PID_PARAMETERS:
                brewPidParameters.Kp = command.float1;
                brewPidParameters.Ki = command.float2;
                brewPidParameters.Kd = command.float3;
                brewPidParameters.windupLow = command.float4;
                brewPidParameters.windupHigh = command.float5;
                break;
            case COMMAND_SET_SERVICE_SET_POINT:
                targetServiceTemperature = command.float1;
                break;
            case COMMAND_SET_SERVICE_PID_PARAMETERS:
                servicePidParameters.Kp = command.float1;
                servicePidParameters.Ki = command.float2;
                servicePidParameters.Kd = command.float3;
                servicePidParameters.windupLow = command.float4;
                servicePidParameters.windupHigh = command.float5;
                break;
            case COMMAND_SET_ECO_MODE:
                ecoMode = command.bool1;
                break;
            case COMMAND_SET_SLEEP_MODE:
                sleepModeRequested = command.bool1;
                break;
            case COMMAND_UNBAIL:
                unbail();
                break;
            case COMMAND_TRIGGER_FIRST_RUN:
                break;
            case COMMAND_ALLOW_LOCKOUT:
                multicore_lockout_victim_init();
                break;
        }
    }

    updateControllerSettings();
}

void SystemController::updateControllerSettings() {
    brewBoilerController.setPidParameters(brewPidParameters);
    //serviceBoilerController.setPidParameters(servicePidParameters);

    if (internalState == SLEEPING) {
        brewBoilerController.updateSetPoint(70.f);
        serviceBoilerController.updateSetPoint(70.f);
    } else if (internalState == HEATUP_STAGE_1) {
        brewBoilerController.updateSetPoint(130.f);
        serviceBoilerController.updateSetPoint(0.f);
    } else if (internalState == HEATUP_STAGE_2) {
        brewBoilerController.updateSetPoint(130.f);
        serviceBoilerController.updateSetPoint(targetServiceTemperature);
    } else {
        brewBoilerController.updateSetPoint(targetBrewTemperature);
        serviceBoilerController.updateSetPoint(targetServiceTemperature);
    }
}

void SystemController::softBail(SystemControllerBailReason reason) {
    if (internalState != HARD_BAILED) {
        internalState = SOFT_BAILED;
    }

    if (bail_reason == BAIL_REASON_NONE) {
        bail_reason = reason;
    }
}

void SystemController::hardBail(SystemControllerBailReason reason) {
    internalState = HARD_BAILED;
    bail_reason = reason;
}

SystemControllerState SystemController::externalState() {
    switch (internalState) {
        case UNDETERMINED:
            return SYSTEM_CONTROLLER_STATE_UNDETERMINED;
        case HEATUP_STAGE_1:
        case HEATUP_STAGE_2:
            return SYSTEM_CONTROLLER_STATE_HEATUP;
        case RUNNING:
            return areTemperaturesAtSetPoint() ? SYSTEM_CONTROLLER_STATE_WARM : SYSTEM_CONTROLLER_STATE_TEMPS_NORMALIZING;
        case SLEEPING:
            return SYSTEM_CONTROLLER_STATE_SLEEPING;
        case SOFT_BAILED:
        case HARD_BAILED:
            return SYSTEM_CONTROLLER_STATE_BAILED;
    }

    return SYSTEM_CONTROLLER_STATE_UNDETERMINED;
}

void SystemController::unbail() {
    internalState = UNDETERMINED;
    bail_reason = BAIL_REASON_NONE;
    unbailTimer.reset();
}

void SystemController::setSleepMode(bool sleepMode) {
    if (!sleepModeChangePossible()) {
        return;
    }

    if (sleepMode) {
        if (internalState == HEATUP_STAGE_2) {
            heatupStage2Timer.reset();
        }

        internalState = SLEEPING;
    } else {
        internalState = UNDETERMINED;
    }

    updateControllerSettings();
}

void SystemController::initiateHeatup() {
    internalState = HEATUP_STAGE_1;
    updateControllerSettings();
}

void SystemController::transitionToHeatupStage2() {
    internalState = HEATUP_STAGE_2;
    heatupStage2Timer = get_absolute_time();
    updateControllerSettings();
}

void SystemController::finishHeatup() {
    internalState = RUNNING;
    heatupStage2Timer.reset();
    updateControllerSettings();
}

bool SystemController::areTemperaturesAtSetPoint() const {
    float bbsplo = targetBrewTemperature - 2.f;
    float bbsphi = targetBrewTemperature + 2.f;
    float sbsplo = targetServiceTemperature - 4.f;
    float sbsphi = targetServiceTemperature + 4.f;

    if (currentControlBoardParsedPacket.brew_boiler_temperature < bbsplo || currentControlBoardParsedPacket.brew_boiler_temperature > bbsphi) {
        return false;
    }

    if (!ecoMode && (currentControlBoardParsedPacket.service_boiler_temperature < sbsplo || currentControlBoardParsedPacket.service_boiler_temperature > sbsphi)) {
        return false;
    }

    return true;
}