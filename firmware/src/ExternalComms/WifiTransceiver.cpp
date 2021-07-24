//
// Created by Magnus Nordlander on 2021-07-16.
//

#include "WifiTransceiver.h"

#include <mbed.h>
#include <functional>

#define WIFI_PASS "pass"
#define WIFI_SSID "IoT Transform"
#define MQTT_HOST 192,168,10,66
#define MQTT_PORT 1883

#define FLOAT_MAX_LEN 6

using namespace std::chrono_literals;

void WifiTransceiver::run() {
    std::function<void(char*, uint8_t*, unsigned int)> func = [&] (char* topic, byte* payload, unsigned int length) {
        callback(topic, payload, length);
    };

    pubSubClient.setClient(wifiClient);
    pubSubClient.setServer(IPAddress(MQTT_HOST), MQTT_PORT);
    pubSubClient.setCallback(func);

    while (true) {
        ensureConnectedToWifi();
        ensureConnectedToMqtt();

        publishStatus();

        pubSubClient.loop();

        rtos::ThisThread::sleep_for(5s);
    }
}

void WifiTransceiver::ensureConnectedToWifi() {
    auto status = wifi.status();
    while (status != WL_CONNECTED) {
        printf("Attempting to connect to WiFi, status: %u\n", status);
        status = wifi.begin(WIFI_SSID, WIFI_PASS);

        rtos::ThisThread::sleep_for(5s);
    }

    printf("Connected to WiFi\n");
}

void WifiTransceiver::ensureConnectedToMqtt() {
    while (!pubSubClient.connected()) {
        printf("Attempting to connect to MQTT\n");
        if (pubSubClient.connect("lcc")) {
            pubSubClient.publish("lcc/outTopic","hello world");
            pubSubClient.subscribe("lcc/inTopic");

            return;
        }

        rtos::ThisThread::sleep_for(5s);
    }
}

void WifiTransceiver::callback(char *topic, byte *payload, unsigned int length) {
    printf("Got a message\n");
}

void WifiTransceiver::publishStatus() {
    pubSubClient.publish("lcc/online","true");
    pubSubClient.publish("lcc/stat/bailed",systemStatus->has_bailed ? "true" : "false");

    pubSubClient.publish("lcc/stat/tx", systemStatus->hasSentLccPacket ? "true" : "false");

    if (systemStatus->hasReceivedControlBoardPacket) {
        pubSubClient.publish("lcc/stat/rx", "true");

        pubSubClient.publish("lcc/stat/water_tank_empty", systemStatus->controlBoardPacket.water_tank_empty ? "true" : "false");

        uint8_t brewTempString[FLOAT_MAX_LEN];
        unsigned int brewLen = snprintf(reinterpret_cast<char *>(brewTempString), FLOAT_MAX_LEN, "%.2f", systemStatus->controlBoardPacket.brew_boiler_temperature);
        pubSubClient.publish("lcc/temp/brew", brewTempString, brewLen);

        uint8_t serviceTempString[FLOAT_MAX_LEN];
        unsigned int serviceLen = snprintf(reinterpret_cast<char *>(serviceTempString), FLOAT_MAX_LEN, "%.2f", systemStatus->controlBoardPacket.service_boiler_temperature);
        pubSubClient.publish("lcc/temp/service", serviceTempString, serviceLen);
    } else {
        pubSubClient.publish("lcc/stat/rx","false");
    }
}

WifiTransceiver::WifiTransceiver(SystemStatus *systemStatus) : systemStatus(systemStatus) {}
