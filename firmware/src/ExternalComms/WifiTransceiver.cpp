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

#define TOPIC_PREFIX "lcc/"

enum topics {
    TOPIC_ONLINE,
    TOPIC_BAILED,
    TOPIC_STAT_TX,
    TOPIC_CONF_ECO_MODE,
    TOPIC_CONF_BREW_TEMP_TARGET,
    TOPIC_CONF_SERVICE_TEMP_TARGET,
    TOPIC_CONF_BREW_TEMP_OFFSET,
    TOPIC_STAT_RX,
    TOPIC_STAT_WATER_TANK_EMPTY,
    TOPIC_TEMP_BREW,
    TOPIC_TEMP_SERVICE,
    TOPIC_SET_CONF_ECO_MODE,
    TOPIC_SET_CONF_BREW_TEMP_TARGET,
    TOPIC_SET_CONF_SERVICE_TEMP_TARGET,
    TOPIC_SET_CONF_BREW_TEMP_OFFSET
};

static const char * const topic_names[] = {
    [TOPIC_ONLINE] = TOPIC_PREFIX "online",
    [TOPIC_BAILED] = TOPIC_PREFIX "stat/bailed",
    [TOPIC_STAT_TX] = TOPIC_PREFIX  "stat/tx",
    [TOPIC_CONF_ECO_MODE] = TOPIC_PREFIX  "conf/eco_mode",
    [TOPIC_CONF_BREW_TEMP_TARGET] = TOPIC_PREFIX  "conf/brew_temp_target",
    [TOPIC_CONF_SERVICE_TEMP_TARGET] = TOPIC_PREFIX  "conf/service_temp_target",
    [TOPIC_CONF_BREW_TEMP_OFFSET] = TOPIC_PREFIX  "conf/brew_temp_offset",
    [TOPIC_STAT_RX] = TOPIC_PREFIX  "stat/rx",
    [TOPIC_STAT_WATER_TANK_EMPTY] = TOPIC_PREFIX  "stat/water_tank_empty",
    [TOPIC_TEMP_BREW] = TOPIC_PREFIX  "temp/brew",
    [TOPIC_TEMP_SERVICE] = TOPIC_PREFIX  "temp/service",
    [TOPIC_SET_CONF_ECO_MODE] = TOPIC_PREFIX "conf/eco_mode/set",
    [TOPIC_SET_CONF_BREW_TEMP_TARGET] = TOPIC_PREFIX "conf/brew_temp_target/set",
    [TOPIC_SET_CONF_SERVICE_TEMP_TARGET] = TOPIC_PREFIX "conf/service_temp_target/set",
    [TOPIC_SET_CONF_BREW_TEMP_OFFSET] = TOPIC_PREFIX "conf/brew_temp_offset/set"
};

using namespace std::chrono_literals;

WifiTransceiver::WifiTransceiver(SystemStatus *systemStatus) : systemStatus(systemStatus) {}

void WifiTransceiver::run() {
    std::function<void(char*, uint8_t*, unsigned int)> func = [&] (char* topic, byte* payload, unsigned int length) {
        callback(topic, payload, length);
    };

    pubSubClient.setClient(wifiClient);
    pubSubClient.setServer(IPAddress(MQTT_HOST), MQTT_PORT);
    pubSubClient.setCallback(func);

    while (true) {
        auto start = rtos::Kernel::Clock::now();

        ensureConnectedToWifi();
        ensureConnectedToMqtt();

        publishStatus();

        // Unlikely to actually sleep, but make sure we're waiting at least 1 second between loops
        rtos::ThisThread::sleep_until(start + 1s);
    }
}

void WifiTransceiver::ensureConnectedToWifi() {
    auto status = wifi.status();
    while (status != WL_CONNECTED) {
        systemStatus->wifiConnected = false;
        printf("Attempting to connect to WiFi, status: %u\n", status);
        status = wifi.begin(WIFI_SSID, WIFI_PASS);

        rtos::ThisThread::sleep_for(5s);
    }

    //printf("Connected to WiFi\n");
    systemStatus->wifiConnected = true;
}

void WifiTransceiver::ensureConnectedToMqtt() {
    while (!pubSubClient.connected()) {
        systemStatus->mqttConnected = false;
        printf("Attempting to connect to MQTT\n");
        if (pubSubClient.connect("lcc", topic_names[TOPIC_ONLINE], 0, true, "false")) {
            systemStatus->mqttConnected = true;
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_ECO_MODE]);
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_BREW_TEMP_TARGET]);
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_SERVICE_TEMP_TARGET]);
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_BREW_TEMP_OFFSET]);

            return;
        }

        rtos::ThisThread::sleep_for(5s);
    }
}

void WifiTransceiver::callback(char *topic, byte *payload, unsigned int length) {
    printf("Received callback. Topic %s\n", topic);

    auto payloadZero = (uint8_t*)malloc((length+1)*sizeof(char));
    memcpy(payloadZero, payload, length);
    payloadZero[length] = '\0';

    printf("Data: %s\n", payloadZero);

    if (!strcmp(topic_names[TOPIC_SET_CONF_ECO_MODE], topic)) {
        systemStatus->ecoMode = !strcmp("true", reinterpret_cast<const char *>(payloadZero));
    } else if (!strcmp(topic_names[TOPIC_SET_CONF_BREW_TEMP_TARGET], topic)) {
        float target = strtof(reinterpret_cast<const char *>(payloadZero), nullptr);

        if (target > 10.0f && target < 110.0f) {
            systemStatus->targetBrewTemperature = target;
        }
    } else if (!strcmp(topic_names[TOPIC_SET_CONF_SERVICE_TEMP_TARGET], topic)) {
        float target = strtof(reinterpret_cast<const char *>(payloadZero), nullptr);

        if (target > 10.0f && target < 150.0f) {
            systemStatus->targetServiceTemperature = target;
        }
    } else if (!strcmp(topic_names[TOPIC_SET_CONF_BREW_TEMP_OFFSET], topic)) {
        float offset = strtof(reinterpret_cast<const char *>(payloadZero), nullptr);

        if (offset > -30.0f && offset < 30.0f) {
            systemStatus->brewTemperatureOffset = offset;
        }
    }

    free(payloadZero);
}

void WifiTransceiver::publishStatus() {
    publish(topic_names[TOPIC_ONLINE], true);
    publish(topic_names[TOPIC_BAILED], systemStatus->has_bailed);

    publish(topic_names[TOPIC_STAT_TX], systemStatus->hasSentLccPacket);

    publish(topic_names[TOPIC_CONF_ECO_MODE], systemStatus->ecoMode);
    publish(topic_names[TOPIC_CONF_BREW_TEMP_TARGET], systemStatus->targetBrewTemperature);
    publish(topic_names[TOPIC_CONF_SERVICE_TEMP_TARGET], systemStatus->targetServiceTemperature);
    publish(topic_names[TOPIC_CONF_BREW_TEMP_OFFSET], systemStatus->brewTemperatureOffset);

    if (systemStatus->hasReceivedControlBoardPacket) {
        publish(topic_names[TOPIC_STAT_RX], true);

        publish(topic_names[TOPIC_STAT_WATER_TANK_EMPTY], systemStatus->controlBoardPacket.water_tank_empty);
        publish(topic_names[TOPIC_TEMP_BREW], systemStatus->controlBoardPacket.brew_boiler_temperature);
        publish(topic_names[TOPIC_TEMP_SERVICE], systemStatus->controlBoardPacket.service_boiler_temperature);
    } else {
        publish(topic_names[TOPIC_STAT_RX],false);
    }
}

void WifiTransceiver::publish(const char *topic, bool payload) {
    pubSubClient.publish(topic,payload ? "true" : "false");
    handleYield();
}

void WifiTransceiver::publish(const char *topic, float payload) {
    uint8_t floatString[FLOAT_MAX_LEN];
    unsigned int len = snprintf(reinterpret_cast<char *>(floatString), FLOAT_MAX_LEN, "%.2f", payload);
    pubSubClient.publish(topic, floatString, len);
    handleYield();
}

void WifiTransceiver::handleYield() {
    pubSubClient.loop();
    rtos::ThisThread::yield();
}

