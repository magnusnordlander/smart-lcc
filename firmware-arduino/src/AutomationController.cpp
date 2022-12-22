//
// Created by Magnus Nordlander on 2022-02-16.
//

#include "AutomationController.h"

AutomationController::AutomationController(SystemStatus *status, SystemSettings *settings): status(status), settings(settings) {

}

void AutomationController::init() {

}

void AutomationController::loop() {
    if (status->hasBailed()) {
        return;
    }

    if (status->currentlyBrewing() && status->isInSleepMode()) {
        settings->setSleepMode(false);
    }

    if (settings->getAutoSleepMin() > 0) {
        uint32_t ms = (uint32_t)settings->getAutoSleepMin() * 60 * 1000;

        if (status->lastBrewStartedAt.has_value() && absolute_time_diff_us(status->getLastSleepModeExitAt(), status->lastBrewStartedAt.value()) > 0) {
            status->plannedAutoSleepAt = delayed_by_ms(status->lastBrewStartedAt.value(), ms);
        } else {
            status->plannedAutoSleepAt = delayed_by_ms(status->getLastSleepModeExitAt(), ms);
        }

        if (absolute_time_diff_us(status->plannedAutoSleepAt.value(), get_absolute_time()) > 0 && !status->isInSleepMode() && !isInSleepActivationGrace()) {
            DEBUGV("Entering auto-sleep. Current time is %u\n", to_ms_since_boot(get_absolute_time()));
            DEBUGV("Last sleep mode exit: %u\n", to_ms_since_boot(status->getLastSleepModeExitAt()));
            settings->setSleepMode(true);
            sleepActivationGrace = make_timeout_time_ms(10000);
        }
    } else {
        status->plannedAutoSleepAt.reset();
    }
}

bool AutomationController::isInSleepActivationGrace() {
    if (!sleepActivationGrace.has_value()) {
        return false;
    }

    return absolute_time_diff_us(sleepActivationGrace.value(), get_absolute_time()) <= 0;
}
