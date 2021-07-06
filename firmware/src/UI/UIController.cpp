//
// Created by Magnus Nordlander on 2021-07-04.
//

#include "UIController.h"

using namespace std::chrono_literals;

UIController::UIController(SystemStatus *status, Adafruit_SSD1306_Spi *display) : status(status), display(display) {}

void UIController::run() {
    while(true)
    {
        display->clearDisplay();
        display->setTextCursor(0,0);

        display->printf("TX: %s RX: %s\r\n", status->hasSentLccPacket ? "Yes" : "No", status->hasReceivedControlBoardPacket ? "Yes" : "No");
        display->printf("Bailed: %s\r\n", status->has_bailed ? "Yes" : "No");

        if (status->hasReceivedControlBoardPacket) {
            display->printf("CB: %.02f SB: %.02f\r\n", status->controlBoardPacket.brew_boiler_temperature, status->controlBoardPacket.service_boiler_temperature);
        }

        display->display();
        rtos::ThisThread::sleep_for(1ms);
    }
}
