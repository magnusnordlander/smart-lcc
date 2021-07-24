//
// Created by Magnus Nordlander on 2021-07-16.
//

#ifndef FIRMWARE_WIFITRANSCEIVER_H
#define FIRMWARE_WIFITRANSCEIVER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <SystemStatus.h>

class WifiTransceiver {
public:
    explicit WifiTransceiver(SystemStatus *systemStatus);

    [[noreturn]] void run();
private:
    SystemStatus* systemStatus;

    WiFiClass wifi;
    WiFiClient wifiClient;
    PubSubClient pubSubClient;

    void ensureConnectedToWifi();
    void ensureConnectedToMqtt();
    void publishStatus();

    void callback(char* topic, byte* payload, unsigned int length);

    void publish(const char* topic, bool payload);
    void publish(const char* topic, float payload);

    void handleYield();
};


#endif //FIRMWARE_WIFITRANSCEIVER_H
