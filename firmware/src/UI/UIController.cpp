//
// Created by Magnus Nordlander on 2021-07-04.
//

#include "UIController.h"

using namespace std::chrono_literals;

UIController::UIController(SystemStatus *status, Adafruit_SSD1306_Spi *display) : status(status), display(display) {}

void UIController::run() {
    display->setRotation(2);
    display->setTextXOffset(18);

    uint8_t blip = 0;

    while(true)
    {
        auto start = rtos::Kernel::Clock::now();

        display->clearDisplay();
        display->setTextCursor(18,18);

        display->printf("%s %s %s %s %s %s\r\n",
                        status->hasSentLccPacket ? "TX" : "tx",
                        status->hasReceivedControlBoardPacket ? "RX" : "rx",
                        status->has_bailed ? "Bail" : "",
                        status->ecoMode ? "E" : "e",
                        status->wifiConnected ? "W" : "w",
                        status->mqttConnected ? "M" : "m");

        display->printf("CT: %d ST: %d\r\n", (int)round(status->getOffsetTargetBrewTemperature()), (int)round(status->targetServiceTemperature));

        if (status->hasReceivedControlBoardPacket) {
//            display->printf("H: %u L: %u\r\n", triplet_to_int(status->controlBoardRawPacket.brew_boiler_temperature_high_gain), triplet_to_int(status->controlBoardRawPacket.brew_boiler_temperature_low_gain));
            display->printf("CB: %d SB: %d\r\n", (int)round(status->getOffsetBrewTemperature()), (int)round(status->controlBoardPacket.service_boiler_temperature));
            display->printf("Br: %s SL: %s WT: %s\r\n", status->controlBoardPacket.brew_switch ? "Y" : "N", status->controlBoardPacket.service_boiler_low ? "Y" : "N", status->controlBoardPacket.water_tank_empty ? "Y" : "N");
        }

        if (status->hasSentLccPacket) {
            display->printf("%s %s %s %s\r\n", status->lccPacket.brew_boiler_ssr_on ? "BSSR" : "bssr", status->lccPacket.service_boiler_ssr_on ? "SSSR" : "sssr", status->lccPacket.pump_on ? "P" : "p", status->lccPacket.service_boiler_solenoid_open ? "BS" : "bs");
        }

        display->setTextCursor(100, 56);
        display->printf("%s", blip < 5 ? "o" : " ");
        blip = (blip + 1) % 10;

        display->display();

        // Cap at 25 fps
        rtos::ThisThread::sleep_until(start + 40ms);
    }
}
