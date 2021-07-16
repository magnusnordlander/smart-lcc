//
// Created by Magnus Nordlander on 2021-07-16.
//

#include "WifiTransceiver.h"

#include <mbed.h>

#define WIFI_PASS "pass"

using namespace std::chrono_literals;

void WifiTransceiver::run() {
    while (wifi.status() != WL_CONNECTED) {
        printf("Attempting to connect to WiFi\n");
        wifi.begin("IoT Transform", WIFI_PASS);

        rtos::ThisThread::sleep_for(5s);
    }

    printf("Connected to WiFi\n");

    while (true) {
        printf("Pinging ping.sunet.se: \n");
        auto pingResult = WiFi.ping("ping.sunet.se");

        if (pingResult >= 0) {
            printf("Success, RTT %d ms\n", pingResult);
        } else {
            printf("Failed, error %d\n", pingResult);
        }

        rtos::ThisThread::sleep_for(5s);
    }
}

WifiTransceiver::WifiTransceiver() = default;
