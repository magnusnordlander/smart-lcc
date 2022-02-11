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

class UIController {
public:
    UIController(SystemStatus *status, U8G2 *display);
    void loop();

private:
    SystemStatus* status;
    U8G2* display;
    uint8_t blip;

    void newline();
};


#endif //FIRMWARE_UICONTROLLER_H
