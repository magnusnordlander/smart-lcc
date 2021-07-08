//
// Created by Magnus Nordlander on 2021-07-04.
//

#ifndef FIRMWARE_UICONTROLLER_H
#define FIRMWARE_UICONTROLLER_H

#include <SystemStatus.h>
#include <Adafruit_SSD1306.h>

class UIController {
public:
    UIController(SystemStatus *status, Adafruit_SSD1306_Spi *display);
    [[noreturn]] void run();

private:
    SystemStatus* status;
    Adafruit_SSD1306_Spi* display;
};


#endif //FIRMWARE_UICONTROLLER_H
