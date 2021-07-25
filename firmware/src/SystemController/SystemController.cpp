//
// Created by Magnus Nordlander on 2021-07-06.
//

#include "SystemController.h"

using namespace std::chrono_literals;

SystemController::SystemController(SystemStatus *status) :
        status(status),
        brewBoilerController(status->targetBrewTemperature, 20.0f, PidParameters{.Kp = 100.0, .Ki = 0.0, .Kd = -200.0}, 2.0f),
        serviceBoilerController(status->targetServiceTemperature, 20.0f, PidParameters{.Kp = 100.0, .Ki = 0.0, .Kd = -200.0}, 2.0f){

}

void SystemController::run() {
    while (true) {
        updateFromSystemStatus();

        if (status->hasReceivedControlBoardPacket) {
            status->lccPacket = handleControlBoardPacket(status->controlBoardPacket);
        }

        // TODO: Align this to the control board thread timing
        rtos::ThisThread::sleep_for(90ms);
    }
}

LccParsedPacket SystemController::handleControlBoardPacket(ControlBoardParsedPacket latestParsedPacket) {
    LccParsedPacket lcc;

    waterTankEmptyLatch.set(latestParsedPacket.water_tank_empty);
    serviceBoilerLowLatch.set(latestParsedPacket.service_boiler_low);

    if (brewBoilerController.getControlSignal(latestParsedPacket.brew_boiler_temperature)) {
        lcc.brew_boiler_ssr_on = true;
    }

    if (!lcc.brew_boiler_ssr_on && serviceBoilerController.getControlSignal(latestParsedPacket.service_boiler_temperature) && !status->ecoMode) {
        lcc.service_boiler_ssr_on = true;
    }

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

void SystemController::updateFromSystemStatus() {
    brewBoilerController.updateSetPoint(status->getOffsetTargetBrewTemperature());
    serviceBoilerController.updateSetPoint(status->targetServiceTemperature);
}
