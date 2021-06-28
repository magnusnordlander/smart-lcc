//
// Created by Magnus Nordlander on 2021-06-27.
//

#include "SystemController.h"

void SystemController::updateWithControlBoardPacket(ControlBoardParsedPacket packet) {
    latestParsedPacket = packet;
}

LccParsedPacket SystemController::createLccPacket() {
    LccParsedPacket lcc = LccParsedPacket();

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
