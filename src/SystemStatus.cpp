//
// Created by Magnus Nordlander on 2021-07-02.
//

#include "SystemStatus.h"

SystemStatus::SystemStatus(SystemSettings *settings): settings(settings) {

}

void SystemStatus::updateStatusMessage(SystemControllerStatusMessage message) {
    if (!latestStatusMessage.currentlyBrewing && message.currentlyBrewing) {
        lastBrewStartedAt = get_absolute_time();
        lastBrewEndedAt.reset();
    } else if (latestStatusMessage.currentlyBrewing && !message.currentlyBrewing) {
        lastBrewEndedAt = get_absolute_time();
    }

    latestStatusMessage = message;
}