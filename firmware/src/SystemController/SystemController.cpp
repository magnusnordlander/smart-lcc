//
// Created by Magnus Nordlander on 2021-07-06.
//

#include "SystemController.h"

using namespace std::chrono_literals;

SystemController::SystemController(SystemStatus *status) :
        status(status),
        brewBoilerController(status->targetBrewTemperature, 20.0f, PidParameters{.Kp = 2.5, .Ki = 0.3, .Kd = 4.0}, 2.0f, 200),
        serviceBoilerController(status->targetServiceTemperature, 20.0f, PidParameters{.Kp = 0.6, .Ki = 0.1, .Kd = 1.0}, 2.0f, 800){

}

void SystemController::run() {
    while (true) {
        updateFromSystemStatus();

#ifndef LCC_RELAY
        if (status->hasReceivedControlBoardPacket) {
            status->lccPacket = handleControlBoardPacket(status->controlBoardPacket);
        }
#endif

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

    status->p = brewBoilerController.pidController.Pout;
    status->i = brewBoilerController.pidController.Iout;
    status->d = brewBoilerController.pidController.Dout;
    status->integral = brewBoilerController.pidController._integral;

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
    brewBoilerController.updateSetPoint(status->targetBrewTemperature);
    serviceBoilerController.updateSetPoint(status->targetServiceTemperature);
}
