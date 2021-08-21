//
// Created by Magnus Nordlander on 2021-07-02.
//

#include "SystemStatus.h"
#include <kvstore_global_api.h>

#define SETTING_VERSION ((uint8_t)1)

SystemStatus::SystemStatus() {
}

void SystemStatus::updateStatusMessage(SystemControllerStatusMessage message) {
    if (!latestStatusMessage.currentlyBrewing && message.currentlyBrewing) {
        lastBrewStartedAt = rtos::Kernel::Clock::now();
        lastBrewEndedAt.reset();
    } else if (latestStatusMessage.currentlyBrewing && !message.currentlyBrewing) {
        lastBrewEndedAt = rtos::Kernel::Clock::now();
    }

    latestStatusMessage = message;

}
