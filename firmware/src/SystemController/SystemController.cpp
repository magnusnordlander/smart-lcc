//
// Created by Magnus Nordlander on 2021-07-06.
//

#include "SystemController.h"

using namespace std::chrono_literals;

void SystemController::run() {
    while (true) {
        if (status->hasReceivedControlBoardPacket) {
            status->lccPacket = handleControlBoardPacket(status->controlBoardPacket);
        }

        // TODO: Align this to the control board thread timing
        rtos::ThisThread::sleep_for(90ms);
    }
}

SystemController::SystemController(SystemStatus *status) : status(status) {}

LccParsedPacket SystemController::handleControlBoardPacket(ControlBoardParsedPacket latestParsedPacket) {
    LccParsedPacket lcc;

    if (latestParsedPacket.brew_boiler_temperature < 94) {
        lcc.brew_boiler_ssr_on = true;
    }

    if (!lcc.brew_boiler_ssr_on && latestParsedPacket.service_boiler_temperature < 125) {
        lcc.service_boiler_ssr_on = true;
    }

    if (!latestParsedPacket.water_tank_empty) {
        if (latestParsedPacket.brew_switch) {
            lcc.pump_on = true;
        } else if (latestParsedPacket.service_boiler_low) {
            lcc.pump_on = true;
            lcc.service_boiler_solenoid_open = true;
        }
    }

    return lcc;
}
