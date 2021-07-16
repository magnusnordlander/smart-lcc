//
// Created by Magnus Nordlander on 2021-07-04.
//

#include "UIController.h"

using namespace std::chrono_literals;

UIController::UIController(SystemStatus *status, Adafruit_SSD1306_Spi *display) : status(status), display(display) {}

void UIController::run() {
    display->setRotation(2);
    while(true)
    {
        display->clearDisplay();
        display->setTextCursor(22,18);

        display->printf("TX: %s RX: %s\r\n", status->hasSentLccPacket ? "Yes" : "No", status->hasReceivedControlBoardPacket ? "Yes" : "No");
        display->printf("Bailed: %s\r\n", status->has_bailed ? "Yes" : "No");

        if (status->hasReceivedControlBoardPacket) {
            display->printf("CB: %.02f SB: %.02f\r\n", status->controlBoardPacket.brew_boiler_temperature, status->controlBoardPacket.service_boiler_temperature);
            display->printf("Br: %s SL: %s WT: %s\r\n", status->controlBoardPacket.brew_switch ? "Y" : "N", status->controlBoardPacket.service_boiler_low ? "Y" : "N", status->controlBoardPacket.water_tank_empty ? "Y" : "N");
        }

        if (status->hasSentLccPacket) {
            display->printf("BSSR: %s SSSR: %s\r\n", status->lccPacket.brew_boiler_ssr_on ? "Y" : "N", status->lccPacket.service_boiler_ssr_on ? "Y" : "N");
            display->printf("P: %s SBS: %s\r\n", status->lccPacket.pump_on ? "Y" : "N", status->lccPacket.service_boiler_solenoid_open ? "Y" : "N");
        }

        display->display();
        rtos::ThisThread::sleep_for(1ms);
    }
}
