//
// Created by Magnus Nordlander on 2021-07-04.
//

#ifndef FIRMWARE_UICONTROLLER_H
#define FIRMWARE_UICONTROLLER_H

#include "SystemStatus.h"
#include <U8g2lib.h>

// Define the dimension of the U8x8log window
#define U8LOG_WIDTH 16
#define U8LOG_HEIGHT 8

typedef enum {
    BREW_SCREEN_IDLE,
    BREW_SCREEN_MOVING_1,
    BREW_SCREEN_MOVING_2
} BrewScreen;

class UIController {
public:
    UIController(SystemStatus *status, SystemSettings* settings, U8G2 *display, uint minus_gpio, uint plus_gpio);
    void loop();

private:
    SystemStatus* status;
    SystemSettings* settings;
    U8G2* display;

    uint minus_gpio;
    uint plus_gpio;

    bool minus;
    bool plus;
    bool previousMinus = false;
    bool previousPlus = false;

    nonstd::optional<absolute_time_t> sleepModeButtonPressBlock;
    nonstd::optional<absolute_time_t> sleepModeSetBlock;

    nonstd::optional<absolute_time_t> minusStartedAt;
    nonstd::optional<absolute_time_t> plusStartedAt;

    uint8_t blip;

    void drawStatusIcons();
    void drawBrewScreen(BrewScreen screen, uint8_t seconds);
    void drawProgressBar();

    inline bool allowedByTimeout(nonstd::optional<absolute_time_t> timeout);
};


#endif //FIRMWARE_UICONTROLLER_H
