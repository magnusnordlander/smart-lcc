//
// Created by Magnus Nordlander on 2021-07-04.
//

#include <U8g2lib.h>
#include <string>
#include "UIController.h"
#include "lccmacros.h"
#include "xbm/bssr_on.h"
#include "xbm/eco_mode.h"
#include "xbm/no_water.h"
#include "xbm/sssr_on.h"
#include "xbm/wifi_mqtt.h"
#include "xbm/wifi_no_mqtt.h"
#include "xbm/pump_on.h"
#include "xbm/cup_smoke_1.h"
#include "xbm/cup_smoke_2.h"
#include "xbm/cup_no_smoke.h"

UIController::UIController(SystemStatus *status, SystemSettings* settings, U8G2 *display, uint minus_gpio, uint plus_gpio):
status(status), settings(settings), display(display), minus_gpio(minus_gpio), plus_gpio(plus_gpio) {
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

    if (status->mode == NETWORK_CONTROLLER_MODE_NORMAL) {
        // Exit sleep mode on any button press
        if (status->isInSleepMode() && ((plus && !previousPlus) || (minus && !previousMinus))) {
            settings->setSleepMode(false);
            sleepModeButtonPressBlock = make_timeout_time_ms(2000);
        }

        if (status->getState() == SYSTEM_CONTROLLER_STATE_HEATUP || status->getState() == SYSTEM_CONTROLLER_STATE_TEMPS_NORMALIZING || status->getState() == SYSTEM_CONTROLLER_STATE_WARM) {
            if (!minusStartedAt.has_value() && minus) {
                minusStartedAt = get_absolute_time();
            } else if (minusStartedAt.has_value() && !minus) {
                int64_t minusPressTime = absolute_time_diff_us(minusStartedAt.value(), get_absolute_time());

                if (minusPressTime > 5000 && minusPressTime < 3000000 && allowedByTimeout(sleepModeButtonPressBlock)) {
                    settings->setTargetBrewTemp(status->getTargetBrewTemp() - 0.5f);
                }

                minusStartedAt.reset();
            }

            if (!plusStartedAt.has_value() && plus) {
                plusStartedAt = get_absolute_time();
            } else if (plusStartedAt.has_value() && !plus) {
                int64_t plusPressTime = absolute_time_diff_us(plusStartedAt.value(), get_absolute_time());

                if (plusPressTime > 5000 && plusPressTime < 3000000 && allowedByTimeout(sleepModeButtonPressBlock)) {
                    settings->setTargetBrewTemp(status->getTargetBrewTemp() + 0.5f);
                }

                plusStartedAt.reset();
            }

            if (
                    plusStartedAt.has_value() &&
                    absolute_time_diff_us(plusStartedAt.value(), get_absolute_time()) > 3000000 &&
                    allowedByTimeout(sleepModeSetBlock)
                    ) {
                if (!status->isInSleepMode()) {
                    settings->setSleepMode(true);
                }

                sleepModeSetBlock = make_timeout_time_ms(3000);
            }
        }
    }


    display->clearBuffer();
    display->setFont(u8g2_font_5x7_tf);

    // Bounds frame, debug only
    // display->drawFrame(X_START, Y_START, S_WIDTH, S_HEIGHT);

    drawStatusIcons();

    if (status->mode == NETWORK_CONTROLLER_MODE_CONFIG) {
        display->setFont(u8g2_font_9x15_tf);
        display->drawStr(X_START + 10, Y_START + 20, "Network");
        display->drawStr(X_START + 10, Y_START + 35, "Config");
    } else if (status->mode == NETWORK_CONTROLLER_MODE_OTA) {
        display->setFont(u8g2_font_9x15_tf);
        display->drawStr(X_START + 10, Y_START + 20, "OTA mode");
        if (status->ipAddress.has_value()) {
            IPAddress ip = status->ipAddress.value();

            display->setFont(u8g2_font_5x7_tf);
            std::string ipString = std::to_string(ip[0]) + "." + std::to_string(ip[1]) + "." + std::to_string(ip[2]) + "." + std::to_string(ip[3]);
            display->drawStr(X_START + 10, Y_START + 43, ipString.c_str());
        }
    } else if (status->getState() == SYSTEM_CONTROLLER_STATE_UNDETERMINED) {
        display->setFont(u8g2_font_9x15_tf);
        display->drawStr(X_START + 10, Y_START + 20, "Undetermined");
    } else if (status->getState() == SYSTEM_CONTROLLER_STATE_BAILED) {
        display->setFont(u8g2_font_9x15_tf);
        display->drawStr(X_START + 10, Y_START + 20, "Bailed");
    } else if (status->getState() == SYSTEM_CONTROLLER_STATE_FIRST_RUN) {
        display->setFont(u8g2_font_9x15_tf);
        display->drawStr(X_START + 10, Y_START + 20, "First run");
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
    } else if (status->getState() == SYSTEM_CONTROLLER_STATE_SLEEPING) {
        display->setFont(u8g2_font_9x15_tf);
        display->drawStr(X_START + 10, Y_START + 20, "Sleeping");

        display->setFont(u8g2_font_5x7_tf);
        char degreeBuf[32];

        snprintf(degreeBuf, sizeof(degreeBuf), "Brew temp %u°C", (uint8_t)round(status->getOffsetBrewTemperature()));
        display->drawUTF8(X_START_MARGIN, Y_END - 9, degreeBuf);

        snprintf(degreeBuf, sizeof(degreeBuf), "Serv. temp %u°C", (uint8_t)round(status->getServiceTemperature()));
        display->drawUTF8(X_START_MARGIN, Y_END - 1, degreeBuf);
    } else {
        display->setFont(u8g2_font_helvB14_tn);

        char degreeBuf[9];

        uint8_t progressOffset = 1;

        if (status->getState() == SYSTEM_CONTROLLER_STATE_HEATUP || status->getState() == SYSTEM_CONTROLLER_STATE_TEMPS_NORMALIZING) {
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
        display->drawStr(brewXMargin + 4, Y_START + 23 + progressOffset, degreeBuf);

        if (!status->isInEcoMode()) {
            snprintf(degreeBuf, sizeof(degreeBuf), "%u", (uint8_t) status->getServiceTemperature());
            display->drawStr(serviceMajorStart, Y_START + 23 + progressOffset, degreeBuf);
        }

        display->setFont(u8g2_font_helvR08_tn);

        if (status->getOffsetBrewTemperature() < 100.0f) {
            snprintf(degreeBuf, sizeof(degreeBuf), ".%u", FUCKED_UP_FLOAT_1_DECIMAL(status->getOffsetBrewTemperature()));
            display->drawStr(brewXMargin + 24, Y_START + 23 + progressOffset, degreeBuf);
        }

        if (!status->isInEcoMode()) {
            snprintf(degreeBuf, sizeof(degreeBuf), ".%u", FUCKED_UP_FLOAT_1_DECIMAL(status->getServiceTemperature()));
            display->drawStr(serviceMinorStart, Y_START + 23 + progressOffset, degreeBuf);
        }

        display->setFont(u8g2_font_4x6_tf);

        if (status->getOffsetBrewTemperature() < 100.0f) {
            display->drawUTF8(brewXMargin + 26, Y_START + 14 + progressOffset, "°C");
        }
        if (!status->isInEcoMode()) {
            display->drawUTF8(serviceMinorStart + 2, Y_START + 14 + progressOffset, "°C");
        }

        display->drawLine(brewXMargin + 4, Y_START + 26 + progressOffset, brewXMargin + 32, Y_START + 26 + progressOffset);
        if (!status->isInEcoMode()) {
            display->drawLine(X_START_MARGIN + 39, Y_START + 26 + progressOffset, X_START_MARGIN + 77,
                              Y_START + 26 + progressOffset);
        }
        display->setFont(u8g2_font_5x7_tf);

        snprintf(degreeBuf, sizeof(degreeBuf), FUCKED_UP_FLOAT_1_FMT "°C", FUCKED_UP_FLOAT_1_ARG(status->getOffsetTargetBrewTemperature()));
        display->drawUTF8(brewXMargin + 4, Y_START + 37 + progressOffset, degreeBuf);
        if (!status->isInEcoMode()) {
            snprintf(degreeBuf, sizeof(degreeBuf), FUCKED_UP_FLOAT_1_FMT "°C",
                     FUCKED_UP_FLOAT_1_ARG(status->getTargetServiceTemp()));
            display->drawUTF8(X_START_MARGIN + 41, Y_START + 37 + progressOffset, degreeBuf);
        }
    }

    if (blip < 25) {
        display->drawDisc(100, 5, 2);
    }
    blip = (blip + 1) % 50;

    display->sendBuffer();
}

void UIController::drawStatusIcons() {
    if (status->wifiConnected && status->mqttConnected) {
        display->drawXBM(X_END_MARGIN - wifi_mqtt_width, Y_START_MARGIN, wifi_mqtt_width, wifi_mqtt_height, wifi_mqtt_bits);
    } else if (status->wifiConnected) {
        display->drawXBM(X_END_MARGIN - wifi_no_mqtt_width, Y_START_MARGIN, wifi_no_mqtt_width, wifi_no_mqtt_height, wifi_no_mqtt_bits);
    }

    if (status->isInEcoMode()) {
        display->drawXBM(X_END_MARGIN - eco_mode_width, Y_START_MARGIN + wifi_mqtt_height + 1, eco_mode_width, eco_mode_height, eco_mode_bits);
    }

    if (status->isWaterTankEmpty()) {
        display->drawXBM(X_END_MARGIN - eco_mode_width, Y_START_MARGIN + wifi_mqtt_height + 1 + eco_mode_height + 1, no_water_width, no_water_height, no_water_bits);
    }

    if (status->currentlyFillingServiceBoiler()) {
        display->drawXBM(X_END_MARGIN - pump_on_width, Y_START_MARGIN + wifi_mqtt_height + 1 + eco_mode_height + 1 + no_water_height + 1, pump_on_width, pump_on_height, pump_on_bits);
    }

    if (status->isBrewSsrOn()) {
        display->drawXBM(X_END_MARGIN - bssr_on_width - 1, Y_END_MARGIN - bssr_on_height, bssr_on_width, bssr_on_height, bssr_on_bits);
    } else if (status->isServiceSsrOn()) {
        display->drawXBM(X_END_MARGIN - sssr_on_width - 1, Y_END_MARGIN - sssr_on_height, sssr_on_width, sssr_on_height, sssr_on_bits);
    }

}

void UIController::drawBrewScreen(BrewScreen screen, uint8_t seconds) {
    switch (screen) {
        case BREW_SCREEN_IDLE:
            display->drawXBM(X_START + 5, Y_START + 6, cup_no_smoke_width, cup_no_smoke_height, cup_no_smoke_bits);
            break;
        case BREW_SCREEN_MOVING_1:
            display->drawXBM(X_START + 5, Y_START + 6, cup_smoke_1_width, cup_smoke_1_height, cup_smoke_1_bits);
            break;
        case BREW_SCREEN_MOVING_2:
            display->drawXBM(X_START + 5, Y_START + 6, cup_smoke_2_width, cup_smoke_2_height, cup_smoke_2_bits);
            break;
    }

    display->setFont(u8g2_font_logisoso20_tr);

    char secondBuf[5];
    snprintf(secondBuf, sizeof(secondBuf), "%u", seconds);

    uint8_t secPos = seconds > 9 ? X_START + 42 : X_START + 52;

    display->drawStr(secPos, Y_END - 12, secondBuf);
    display->drawStr(X_START + 72, Y_END - 12, "s");

    display->setFont(u8g2_font_5x7_tf);

    char degreeBuf[9];
    snprintf(degreeBuf, sizeof(degreeBuf), FUCKED_UP_FLOAT_1_FMT "°C", FUCKED_UP_FLOAT_1_ARG(status->getOffsetBrewTemperature()));

    display->drawStr(X_START + 5, Y_END - 1, "Brew temp");
    display->drawUTF8(X_START + 52, Y_END - 1, degreeBuf);
}

void UIController::drawProgressBar() {
    display->drawFrame(X_START_MARGIN + 3, Y_START_MARGIN + 3, 76, 5);

    uint8_t x = X_START_MARGIN + 3;
    uint8_t y = Y_START_MARGIN + 3;

    unsigned short doublemilli = (micros() / 1000) % 2000;

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

    display->drawBox(start, y, width, 5);
}

inline bool UIController::allowedByTimeout(nonstd::optional<absolute_time_t> timeout) {
    return !timeout.has_value() || absolute_time_diff_us(timeout.value(), get_absolute_time()) > 0;
}
