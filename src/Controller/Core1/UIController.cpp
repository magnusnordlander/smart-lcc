//
// Created by Magnus Nordlander on 2021-07-04.
//

#include <u8g2.h>
#include <string>
#include <hardware/gpio.h>
#include <cmath>
#include "UIController.h"
#include "utils/lccmacros.h"
#include "bssr_on.h"
#include "eco_mode.h"
#include "no_water.h"
#include "sssr_on.h"
#include "wifi_mqtt.h"
#include "wifi_no_mqtt.h"
#include "pump_on.h"
#include "cup_smoke_1.h"
#include "cup_smoke_2.h"
#include "cup_no_smoke.h"

UIController::UIController(SystemStatus *status, PicoQueue<SystemControllerCommand> *commandQueue, u8g2_t *display, uint minus_gpio, uint plus_gpio):
status(status), commandQueue(commandQueue), display(display), minus_gpio(minus_gpio), plus_gpio(plus_gpio) {
    blip = 0;
}

#define X_START 15
#define Y_START 14
#define S_WIDTH 95
#define S_HEIGHT 50
#define X_END (X_START + S_WIDTH)
#define Y_END (Y_START + S_HEIGHT)
#define X_START_MARGIN (X_START + 2)
#define Y_START_MARGIN (Y_START + 2)
#define X_END_MARGIN (X_END - 2)
#define Y_END_MARGIN (Y_END - 2)

void UIController::loop() {
    previousMinus = minus;
    previousPlus = plus;

    minus = gpio_get(minus_gpio);
    plus = gpio_get(plus_gpio);

//    if (status->mode == SYSTEM_MODE_NORMAL) {
        // Exit sleep mode on any button press
        // @todo This is a candidate for the random sleep exits
        if (status->isInSleepMode() && ((plus && !previousPlus) || (minus && !previousMinus))) {
            SystemControllerCommand exitSleepCommand;
            exitSleepCommand.type = COMMAND_SET_SLEEP_MODE;
            exitSleepCommand.bool1 = false;
            commandQueue->tryAdd(&exitSleepCommand);
            sleepModeButtonPressBlock = make_timeout_time_ms(2000);
        }

        if (status->getState() == SYSTEM_CONTROLLER_COALESCED_STATE_HEATUP || status->getState() == SYSTEM_CONTROLLER_COALESCED_STATE_TEMPS_NORMALIZING || status->getState() == SYSTEM_CONTROLLER_COALESCED_STATE_WARM) {
            if (!minusStartedAt.has_value() && minus) {
                minusStartedAt = get_absolute_time();
            } else if (minusStartedAt.has_value() && !minus) {
                int64_t minusPressTime = absolute_time_diff_us(minusStartedAt.value(), get_absolute_time());

                if (minusPressTime > 5000 && minusPressTime < 3000000 && allowedByTimeout(sleepModeButtonPressBlock)) {
                    SystemControllerCommand setPointCommand;
                    setPointCommand.type = COMMAND_SET_BREW_SET_POINT;
                    setPointCommand.float1 = status->getTargetBrewTemp() - 0.5f;
                    commandQueue->tryAdd(&setPointCommand);
                }

                minusStartedAt.reset();
            }

            if (!plusStartedAt.has_value() && plus) {
                plusStartedAt = get_absolute_time();
            } else if (plusStartedAt.has_value() && !plus) {
                int64_t plusPressTime = absolute_time_diff_us(plusStartedAt.value(), get_absolute_time());

                if (plusPressTime > 5000 && plusPressTime < 3000000 && allowedByTimeout(sleepModeButtonPressBlock)) {
                    SystemControllerCommand setPointCommand;
                    setPointCommand.type = COMMAND_SET_BREW_SET_POINT;
                    setPointCommand.float1 = status->getTargetBrewTemp() + 0.5f;
                    commandQueue->tryAdd(&setPointCommand);
                }

                plusStartedAt.reset();
            }

            if (
                    plusStartedAt.has_value() &&
                    absolute_time_diff_us(plusStartedAt.value(), get_absolute_time()) > 3000000 &&
                    allowedByTimeout(sleepModeSetBlock)
                    ) {
                if (!status->isInSleepMode()) {
                    SystemControllerCommand goToSleepCommand;
                    goToSleepCommand.type = COMMAND_SET_SLEEP_MODE;
                    goToSleepCommand.bool1 = true;
                    commandQueue->tryAdd(&goToSleepCommand);
                }

                sleepModeSetBlock = make_timeout_time_ms(3000);
            }
        }
//    }

    u8g2_ClearBuffer(display);
    u8g2_SetFont(display, u8g2_font_5x7_tf);

    // Bounds frame, debug only
    // display->drawFrame(X_START, Y_START, S_WIDTH, S_HEIGHT);

    drawStatusIcons();

    if (status->mode == SYSTEM_MODE_CONFIG) {
        u8g2_SetFont(display, u8g2_font_9x15_tf);
        u8g2_DrawStr(display, X_START + 10, Y_START + 20, "Network");
        u8g2_DrawStr(display, X_START + 10, Y_START + 35, "Config");
    } else if (status->mode == SYSTEM_MODE_OTA) {
        u8g2_SetFont(display, u8g2_font_9x15_tf);
        u8g2_DrawStr(display, X_START + 10, Y_START + 20, "OTA mode");
/*        if (status->ipAddress.has_value()) {
            IPAddress ip = status->ipAddress.value();

            display->setFont(u8g2_font_5x7_tf);
            std::string ipString = std::to_string(ip[0]) + "." + std::to_string(ip[1]) + "." + std::to_string(ip[2]) + "." + std::to_string(ip[3]);
            u8g2_DrawStr(display, X_START + 10, Y_START + 43, ipString.c_str());
        }*/
    } else if (status->getState() == SYSTEM_CONTROLLER_COALESCED_STATE_UNDETERMINED) {
        u8g2_SetFont(display, u8g2_font_9x15_tf);
        u8g2_DrawStr(display, X_START + 10, Y_START + 20, "Undetermined");
    } else if (status->getState() == SYSTEM_CONTROLLER_COALESCED_STATE_BAILED) {
        u8g2_SetFont(display, u8g2_font_9x15_tf);
        u8g2_DrawStr(display, X_START + 10, Y_START + 20, "Bailed");
    } else if (status->getState() == SYSTEM_CONTROLLER_COALESCED_STATE_FIRST_RUN) {
        u8g2_SetFont(display, u8g2_font_9x15_tf);
        u8g2_DrawStr(display, X_START + 10, Y_START + 20, "First run");
    } else if (status->currentlyBrewing()) {
        auto micros = absolute_time_diff_us(status->lastBrewStartedAt.value(), get_absolute_time());
        auto seconds = (uint8_t)round((float)micros / 1000000.f);

        if (seconds % 2 == 0) {
            drawBrewScreen(BREW_SCREEN_MOVING_1, seconds);
        } else {
            drawBrewScreen(BREW_SCREEN_MOVING_2, seconds);
        }
    } else if (status->lastBrewEndedAt.has_value() && absolute_time_diff_us(status->lastBrewEndedAt.value(), get_absolute_time()) < 7500000) {
        auto micros = absolute_time_diff_us(status->lastBrewStartedAt.value(), status->lastBrewEndedAt.value());
        auto seconds = (uint8_t)round((float)micros / 1000000.f);

        drawBrewScreen(BREW_SCREEN_IDLE, seconds);
    } else if (status->getState() == SYSTEM_CONTROLLER_COALESCED_STATE_SLEEPING) {
        u8g2_SetFont(display, u8g2_font_9x15_tf);
        u8g2_DrawStr(display, X_START + 10, Y_START + 20, "Sleeping");

        u8g2_SetFont(display, u8g2_font_5x7_tf);
        char degreeBuf[32];

        snprintf(degreeBuf, sizeof(degreeBuf), "Brew temp %u°C", (uint8_t)round(status->getOffsetBrewTemperature()));
        u8g2_DrawUTF8(display, X_START_MARGIN, Y_END - 9, degreeBuf);

        snprintf(degreeBuf, sizeof(degreeBuf), "Serv. temp %u°C", (uint8_t)round(status->getServiceTemperature()));
        u8g2_DrawUTF8(display, X_START_MARGIN, Y_END - 1, degreeBuf);
    } else {
        u8g2_SetFont(display, u8g2_font_helvB14_tn);

        char degreeBuf[9];

        uint8_t progressOffset = 1;

        if (status->getState() == SYSTEM_CONTROLLER_COALESCED_STATE_HEATUP || status->getState() == SYSTEM_CONTROLLER_COALESCED_STATE_TEMPS_NORMALIZING) {
            drawProgressBar();
            progressOffset = 5;
        }

        uint8_t brewXMargin = X_START_MARGIN;

        if (status->isInEcoMode()) {
            brewXMargin = X_START_MARGIN + 26;
        }

        uint8_t serviceMajorStart = X_START_MARGIN + 39;
        uint8_t serviceMinorStart = X_START_MARGIN + 69;

        if (status->getServiceTemperature() < 100.f) {
            serviceMajorStart = X_START_MARGIN + 44;
            serviceMinorStart = X_START_MARGIN + 64;
        }

        snprintf(degreeBuf, sizeof(degreeBuf), "%u", (uint8_t)status->getOffsetBrewTemperature());
        u8g2_DrawStr(display, brewXMargin + 4, Y_START + 23 + progressOffset, degreeBuf);

        if (!status->isInEcoMode()) {
            snprintf(degreeBuf, sizeof(degreeBuf), "%u", (uint8_t) status->getServiceTemperature());
            u8g2_DrawStr(display, serviceMajorStart, Y_START + 23 + progressOffset, degreeBuf);
        }

        u8g2_SetFont(display, u8g2_font_helvR08_tn);

        if (status->getOffsetBrewTemperature() < 100.0f) {
            snprintf(degreeBuf, sizeof(degreeBuf), ".%u", FUCKED_UP_FLOAT_1_DECIMAL(status->getOffsetBrewTemperature()));
            u8g2_DrawStr(display, brewXMargin + 24, Y_START + 23 + progressOffset, degreeBuf);
        }

        if (!status->isInEcoMode()) {
            snprintf(degreeBuf, sizeof(degreeBuf), ".%u", FUCKED_UP_FLOAT_1_DECIMAL(status->getServiceTemperature()));
            u8g2_DrawStr(display, serviceMinorStart, Y_START + 23 + progressOffset, degreeBuf);
        }

        u8g2_SetFont(display, u8g2_font_4x6_tf);

        if (status->getOffsetBrewTemperature() < 100.0f) {
            u8g2_DrawUTF8(display, brewXMargin + 26, Y_START + 14 + progressOffset, "°C");
        }
        if (!status->isInEcoMode()) {
            u8g2_DrawUTF8(display, serviceMinorStart + 2, Y_START + 14 + progressOffset, "°C");
        }

        u8g2_DrawLine(display, brewXMargin + 4, Y_START + 26 + progressOffset, brewXMargin + 32, Y_START + 26 + progressOffset);
        if (!status->isInEcoMode()) {
            u8g2_DrawLine(display, X_START_MARGIN + 39, Y_START + 26 + progressOffset, X_START_MARGIN + 77,
                              Y_START + 26 + progressOffset);
        }
        u8g2_SetFont(display, u8g2_font_5x7_tf);

        snprintf(degreeBuf, sizeof(degreeBuf), FUCKED_UP_FLOAT_1_FMT "°C", FUCKED_UP_FLOAT_1_ARG(status->getOffsetTargetBrewTemperature()));
        u8g2_DrawUTF8(display, brewXMargin + 4, Y_START + 37 + progressOffset, degreeBuf);
        if (!status->isInEcoMode()) {
            snprintf(degreeBuf, sizeof(degreeBuf), FUCKED_UP_FLOAT_1_FMT "°C",
                     FUCKED_UP_FLOAT_1_ARG(status->getTargetServiceTemp()));
            u8g2_DrawUTF8(display, X_START_MARGIN + 41, Y_START + 37 + progressOffset, degreeBuf);
        }
    }

    if (blip < 25) {
        u8g2_DrawDisc(display, 104, 10, 2, U8G2_DRAW_ALL);
    }
    blip = (blip + 1) % 50;

    u8g2_SendBuffer(display);
}

void UIController::drawStatusIcons() {
    if (status->isWifiConnected() && status->mqttConnected) {
        u8g2_DrawXBM(display, X_END_MARGIN - wifi_mqtt_width, Y_START_MARGIN, wifi_mqtt_width, wifi_mqtt_height, wifi_mqtt_bits);
    } else if (status->isWifiConnected()) {
        u8g2_DrawXBM(display, X_END_MARGIN - wifi_no_mqtt_width, Y_START_MARGIN, wifi_no_mqtt_width, wifi_no_mqtt_height, wifi_no_mqtt_bits);
    }

    if (status->isInEcoMode()) {
        u8g2_DrawXBM(display, X_END_MARGIN - eco_mode_width, Y_START_MARGIN + wifi_mqtt_height + 1, eco_mode_width, eco_mode_height, eco_mode_bits);
    }

    if (status->isWaterTankEmpty()) {
        u8g2_DrawXBM(display, X_END_MARGIN - eco_mode_width, Y_START_MARGIN + wifi_mqtt_height + 1 + eco_mode_height + 1, no_water_width, no_water_height, no_water_bits);
    }

    if (status->currentlyFillingServiceBoiler()) {
        u8g2_DrawXBM(display, X_END_MARGIN - pump_on_width, Y_START_MARGIN + wifi_mqtt_height + 1 + eco_mode_height + 1 + no_water_height + 1, pump_on_width, pump_on_height, pump_on_bits);
    }

    if (status->isBrewSsrOn()) {
        u8g2_DrawXBM(display, X_END_MARGIN - bssr_on_width - 1, Y_END_MARGIN - bssr_on_height, bssr_on_width, bssr_on_height, bssr_on_bits);
    } else if (status->isServiceSsrOn()) {
        u8g2_DrawXBM(display, X_END_MARGIN - sssr_on_width - 1, Y_END_MARGIN - sssr_on_height, sssr_on_width, sssr_on_height, sssr_on_bits);
    }

}

void UIController::drawBrewScreen(BrewScreen screen, uint8_t seconds) {
    switch (screen) {
        case BREW_SCREEN_IDLE:
            u8g2_DrawXBM(display, X_START + 5, Y_START + 6, cup_no_smoke_width, cup_no_smoke_height, cup_no_smoke_bits);
            break;
        case BREW_SCREEN_MOVING_1:
            u8g2_DrawXBM(display, X_START + 5, Y_START + 6, cup_smoke_1_width, cup_smoke_1_height, cup_smoke_1_bits);
            break;
        case BREW_SCREEN_MOVING_2:
            u8g2_DrawXBM(display, X_START + 5, Y_START + 6, cup_smoke_2_width, cup_smoke_2_height, cup_smoke_2_bits);
            break;
    }

    u8g2_SetFont(display, u8g2_font_logisoso20_tr);

    char secondBuf[5];
    snprintf(secondBuf, sizeof(secondBuf), "%u", seconds);

    uint8_t secPos = seconds > 9 ? X_START + 42 : X_START + 52;

    u8g2_DrawStr(display, secPos, Y_END - 12, secondBuf);
    u8g2_DrawStr(display, X_START + 72, Y_END - 12, "s");

    u8g2_SetFont(display, u8g2_font_5x7_tf);

    char degreeBuf[9];
    snprintf(degreeBuf, sizeof(degreeBuf), FUCKED_UP_FLOAT_1_FMT "°C", FUCKED_UP_FLOAT_1_ARG(status->getOffsetBrewTemperature()));

    u8g2_DrawStr(display, X_START + 5, Y_END - 1, "Brew temp");
    u8g2_DrawUTF8(display, X_START + 52, Y_END - 1, degreeBuf);
}

void UIController::drawProgressBar() {
    u8g2_DrawFrame(display, X_START_MARGIN + 3, Y_START_MARGIN + 3, 76, 5);

    uint8_t x = X_START_MARGIN + 3;
    uint8_t y = Y_START_MARGIN + 3;

    unsigned short doublemilli = (to_us_since_boot(get_absolute_time()) / 1000) % 2000;

    // Goes between 0 and 57
    unsigned short frame = doublemilli / 35;

    unsigned short start;
    unsigned short width;

    if (frame <= 19) {
        start = x;
        width = 2 * frame;
    } else if (frame <= 38) {
        start = x + (frame - 19) * 2;
        width = 38;
    } else {
        start = x + (frame - 19) * 2;
        width = 38 - (frame - 38) * 2;
    }

    u8g2_DrawBox(display, start, y, width, 5);
}

inline bool UIController::allowedByTimeout(nonstd::optional<absolute_time_t> timeout) {
    return !timeout.has_value() || absolute_time_diff_us(timeout.value(), get_absolute_time()) > 0;
}
