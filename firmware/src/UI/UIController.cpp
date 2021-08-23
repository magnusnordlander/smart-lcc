//
// Created by Magnus Nordlander on 2021-07-04.
//

#include "UIController.h"
#include <lccmacros.h>

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

        switch (status->getState()) {
            case SYSTEM_CONTROLLER_STATE_UNDETERMINED:
                display->printf("Unkn");
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

        display->printf(" %s %s %s\r\n",
                        status->isInEcoMode() ? "E" : "e",
                        status->wifiConnected ? "W" : "w",
                        status->mqttConnected ? "M" : "m");

#ifndef LCC_RELAY
        display->printf("CT:%d ST:%d\r\n", (int)round(status->getOffsetTargetBrewTemperature()), (int)round(status->getTargetServiceTemp()));
#endif

        if (status->hasReceivedControlBoardPacket) {
//            display->printf("H: %u L: %u\r\n", triplet_to_int(status->controlBoardRawPacket.brew_boiler_temperature_high_gain), triplet_to_int(status->controlBoardRawPacket.brew_boiler_temperature_low_gain));
            display->printf("CB:" FUCKED_UP_FLOAT_1_FMT " SB:" FUCKED_UP_FLOAT_1_FMT "\r\n", FUCKED_UP_FLOAT_1_ARG(status->getOffsetBrewTemperature()), FUCKED_UP_FLOAT_1_ARG(status->getServiceTemperature()));
            display->printf("Br:%s SL:%s WT:%s\r\n", status->currentlyBrewing() ? "Y" : "N", status->currentlyFillingServiceBoiler() ? "Y" : "N", status->isWaterTankEmpty() ? "Y" : "N");
        }

        if (status->lastBrewStartedAt.has_value()) {
            if (status->lastBrewEndedAt.has_value()) {
                auto millis = (uint16_t)std::chrono::duration_cast<std::chrono::milliseconds>(status->lastBrewEndedAt.value() - status->lastBrewStartedAt.value()).count();
                display->printf("Brewed: %u s\r\n", (uint8_t)round((float)millis / 1000.f));
            } else {
                auto millis = (uint16_t)std::chrono::duration_cast<std::chrono::milliseconds>(rtos::Kernel::Clock::now() - status->lastBrewStartedAt.value()).count();
                display->printf("Brewing: %u s\r\n", (uint8_t)round((float)millis / 1000.f));
            }
        }

        if (status->hasSentLccPacket) {
            display->printf("%s %s\r\n", status->isBrewSsrOn() ? "BSSR" : "bssr", status->isServiceSsrOn() ? "SSSR" : "sssr");
        }

#ifndef LCC_RELAY
//        display->printf("P%.1f I%.1f D%.1f S%.1f\r\n", status->p, status->i, status->d, status->integral);
#endif

#ifdef LCC_RELAY
        display->printf("LB %u LS %u\r\n", status->lastBssrCycleMs, status->lastSssrCycleMs);
        display->printf("BOn %u BOf %u\r\n", status->minBssrOnCycleMs, status->minBssrOffCycleMs);
#endif

        //display->printf("%lu", (unsigned long)start.time_since_epoch().count());

        display->setTextCursor(100, 18);
        display->printf("%s", blip < 5 ? "o" : " ");
        blip = (blip + 1) % 10;

        display->display();

        //sleep_ms(30);
        // Cap at 25 fps
        rtos::ThisThread::sleep_until(start + 40ms);
    }
}
