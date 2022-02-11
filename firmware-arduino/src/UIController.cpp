//
// Created by Magnus Nordlander on 2021-07-04.
//

#include <U8g2lib.h>
#include "UIController.h"
#include "lccmacros.h"

UIController::UIController(SystemStatus *status, U8G2 *display) : status(status), display(display) {
    blip = 0;
}

#define LINE_HEIGHT 7
#define X_START 18
#define Y_START 18

void UIController::loop() {
    display->clearBuffer();
    display->setFont(u8g2_font_5x7_tf);
    display->setCursor(X_START, LINE_HEIGHT + Y_START);

    switch (status->getState()) {
        case SYSTEM_CONTROLLER_STATE_UNDETERMINED:
            switch (status->mode) {
                case SYSTEM_MODE_NORMAL:
                    display->printf("Unkn");
                    break;
                case SYSTEM_MODE_NETWORK_CONFIG:
                    display->printf("Conf");
                    break;
                case SYSTEM_MODE_RP2040_OTA:
                    display->printf("OTA");
                    break;
            }
            break;
        case SYSTEM_CONTROLLER_STATE_HEATUP:
            display->printf("Heating");
            break;
        case SYSTEM_CONTROLLER_STATE_TEMPS_NORMALIZING:
            display->printf("Norm");
            break;
        case SYSTEM_CONTROLLER_STATE_WARM:
            display->printf("Warm");
            break;
        case SYSTEM_CONTROLLER_STATE_SLEEPING:
            display->printf("Sleep");
            break;
        case SYSTEM_CONTROLLER_STATE_BAILED:
            display->printf("Bailed");
            break;
        case SYSTEM_CONTROLLER_STATE_FIRST_RUN:
            display->printf("First");
            break;
    }

    display->printf(" %s %s %s",
                    status->isInEcoMode() ? "E" : "e",
                    status->wifiConnected ? "W" : "w",
                    status->mqttConnected ? "M" : "m");

    newline();

#ifndef LCC_RELAY
    display->printf("CT:%d ST:%d", (int)round(status->getOffsetTargetBrewTemperature()), (int)round(status->getTargetServiceTemp()));
    newline();
#endif

    if (status->hasReceivedControlBoardPacket) {
//            display->printf("H: %u L: %u\r\n", triplet_to_int(status->controlBoardRawPacket.brew_boiler_temperature_high_gain), triplet_to_int(status->controlBoardRawPacket.brew_boiler_temperature_low_gain));
        display->printf("CB:" FUCKED_UP_FLOAT_1_FMT " SB:" FUCKED_UP_FLOAT_1_FMT, FUCKED_UP_FLOAT_1_ARG(status->getOffsetBrewTemperature()), FUCKED_UP_FLOAT_1_ARG(status->getServiceTemperature()));
        newline();
        display->printf("Br:%s SL:%s WT:%s", status->currentlyBrewing() ? "Y" : "N", status->currentlyFillingServiceBoiler() ? "Y" : "N", status->isWaterTankEmpty() ? "Y" : "N");
        newline();
    }

/*    if (status->lastBrewStartedAt.has_value()) {
        if (status->lastBrewEndedAt.has_value()) {
            auto millis = (uint16_t)std::chrono::duration_cast<std::chrono::milliseconds>(status->lastBrewEndedAt.value() - status->lastBrewStartedAt.value()).count();
            display->printf("Brewed: %u s\r\n", (uint8_t)round((float)millis / 1000.f));
        } else {
            auto millis = (uint16_t)std::chrono::duration_cast<std::chrono::milliseconds>(rtos::Kernel::Clock::now() - status->lastBrewStartedAt.value()).count();
            display->printf("Brewing: %u s\r\n", (uint8_t)round((float)millis / 1000.f));
        }
    }*/

    if (status->hasSentLccPacket) {
        display->printf("%s %s", status->isBrewSsrOn() ? "BSSR" : "bssr", status->isServiceSsrOn() ? "SSSR" : "sssr");
        newline();
    }

    if (blip < 25) {
        display->drawCircle(100, 18, 4);
    }
    blip = (blip + 1) % 50;

    display->sendBuffer();
}

void UIController::newline() {
    display->setCursor(X_START, display->ty + LINE_HEIGHT);
}
