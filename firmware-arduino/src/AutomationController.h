//
// Created by Magnus Nordlander on 2022-02-16.
//

#ifndef FIRMWARE_ARDUINO_AUTOMATIONCONTROLLER_H
#define FIRMWARE_ARDUINO_AUTOMATIONCONTROLLER_H


#include "SystemStatus.h"
#include "SystemSettings.h"

class AutomationController {
public:
    explicit AutomationController(SystemStatus* status, SystemSettings* settings);

    void init();
    void loop();
private:
    SystemStatus* status;
    SystemSettings* settings;

    nonstd::optional<absolute_time_t> sleepActivationGrace;

    bool isInSleepActivationGrace();
};


#endif //FIRMWARE_ARDUINO_AUTOMATIONCONTROLLER_H
