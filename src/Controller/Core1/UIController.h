//
// Created by Magnus Nordlander on 2021-07-04.
//

#ifndef FIRMWARE_UICONTROLLER_H
#define FIRMWARE_UICONTROLLER_H

#include "SystemStatus.h"
#include <u8g2.h>

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
    UIController(SystemStatus *status, PicoQueue<SystemControllerCommand> *commandQueue, u8g2_t *display, uint minus_gpio, uint plus_gpio);
    void loop();

private:
    PicoQueue<SystemControllerCommand> *commandQueue;

    SystemStatus* status;
    u8g2_t *display;

    uint minus_gpio;
    uint plus_gpio;

    bool minus = false;
    bool plus = false;
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
