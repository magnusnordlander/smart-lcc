//
// Created by Magnus Nordlander on 2021-07-16.
//

#ifndef FIRMWARE_WIFITRANSCEIVER_H
#define FIRMWARE_WIFITRANSCEIVER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <SystemStatus.h>
#include <SystemSettings.h>

class WifiTransceiver {
public:
    explicit WifiTransceiver(
            SystemStatus *systemStatus,
            SystemSettings *systemSettings,
            PicoQueue<SystemControllerCommand>* commandQueue
            );

    [[noreturn]] void run();
private:
    SystemStatus* systemStatus;
    SystemSettings* systemSettings;
    PicoQueue<SystemControllerCommand>* commandQueue;

    WiFiClass wifi;
    WiFiClient wifiClient;
    PubSubClient pubSubClient;

    void ensureConnectedToWifi();
    void ensureConnectedToMqtt();
    void publishStatus();

    void callback(char* topic, byte* payload, unsigned int length);

    void publish(const char* topic, bool payload);
    void publish(const char* topic, float payload);
    void publish(const char* topic, uint8_t payload);
    void publish(const char *topic, double payload);
    void publish(const char *topic, SystemControllerBailReason payload);
    void publish(const char *topic, SystemControllerState payload);
    void publish(const char *topic, reset_reason_t payload);

    void handleYield();
};


#endif //FIRMWARE_WIFITRANSCEIVER_H
