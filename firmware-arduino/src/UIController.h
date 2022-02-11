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
    UIController(SystemStatus *status, U8G2 *display);
    void loop();

private:
    SystemStatus* status;
    U8G2* display;
    uint8_t blip;

    void newline();
    void drawStatusIcons();
    void drawBrewScreen(BrewScreen screen, uint8_t seconds);
    void drawProgressBar();
};


#endif //FIRMWARE_UICONTROLLER_H
