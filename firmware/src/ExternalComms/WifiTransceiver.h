//
// Created by Magnus Nordlander on 2021-07-16.
//

#ifndef FIRMWARE_WIFITRANSCEIVER_H
#define FIRMWARE_WIFITRANSCEIVER_H

#include <WiFi.h>

class WifiTransceiver {
public:
    WifiTransceiver();
    [[noreturn]] void run();

private:
    WiFiClass wifi;
};


#endif //FIRMWARE_WIFITRANSCEIVER_H
