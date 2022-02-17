//
// Created by Magnus Nordlander on 2022-02-16.
//

#include "AutomationController.h"

AutomationController::AutomationController(SystemStatus *status, SystemSettings *settings): status(status), settings(settings) {

}

void AutomationController::init() {

}

void AutomationController::loop() {
    if (status->currentlyBrewing() && status->isInSleepMode()) {
        settings->setSleepMode(false);
    }

    if (settings->getAutoSleepMin() > 0) {
        absolute_time_t timeout;
        uint32_t ms = (uint32_t)settings->getAutoSleepMin() * 60 * 1000;

        if (status->lastBrewStartedAt.has_value()) {
            timeout = delayed_by_ms(status->lastBrewStartedAt.value(), ms);
        } else {
            timeout = delayed_by_ms(nil_time, ms);
        }

        if (absolute_time_diff_us(timeout, get_absolute_time()) > 0) {
            settings->setSleepMode(true);
        }
    }
}
