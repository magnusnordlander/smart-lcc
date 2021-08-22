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

#define FLOAT_MAX_LEN 7

#define TOPIC_PREFIX "lcc/"

enum topics {
    TOPIC_ONLINE,
    TOPIC_BAILED,
    TOPIC_BAIL_REASON,
    TOPIC_STAT_TX,
    TOPIC_CONF_ECO_MODE,
    TOPIC_CONF_SLEEP_MODE,
    TOPIC_CONF_BREW_TEMP_TARGET,
    TOPIC_CONF_SERVICE_TEMP_TARGET,
    TOPIC_CONF_BREW_TEMP_OFFSET,
    TOPIC_CONF_BREW_PID_KP,
    TOPIC_CONF_BREW_PID_KI,
    TOPIC_CONF_BREW_PID_KD,
    TOPIC_CONF_SERVICE_PID_KP,
    TOPIC_CONF_SERVICE_PID_KI,
    TOPIC_CONF_SERVICE_PID_KD,
    TOPIC_STAT_RX,
    TOPIC_STAT_WATER_TANK_EMPTY,
    TOPIC_STAT_BREW_PID_P,
    TOPIC_STAT_BREW_PID_I,
    TOPIC_STAT_BREW_PID_D,
    TOPIC_STAT_BREW_PID_INTEGRAL,
    TOPIC_STAT_SERVICE_PID_P,
    TOPIC_STAT_SERVICE_PID_I,
    TOPIC_STAT_SERVICE_PID_D,
    TOPIC_STAT_SERVICE_PID_INTEGRAL,
    TOPIC_TEMP_BREW,
    TOPIC_TEMP_SERVICE,
    TOPIC_SET_CONF_ECO_MODE,
    TOPIC_SET_CONF_SLEEP_MODE,
    TOPIC_SET_CONF_BREW_TEMP_TARGET,
    TOPIC_SET_CONF_SERVICE_TEMP_TARGET,
    TOPIC_SET_CONF_BREW_TEMP_OFFSET,
    TOPIC_SET_CONF_BREW_PID_KP,
    TOPIC_SET_CONF_BREW_PID_KI,
    TOPIC_SET_CONF_BREW_PID_KD,
    TOPIC_SET_CONF_SERVICE_PID_KP,
    TOPIC_SET_CONF_SERVICE_PID_KI,
    TOPIC_SET_CONF_SERVICE_PID_KD
};

static const char * const topic_names[] = {
    [TOPIC_ONLINE] = TOPIC_PREFIX "online",
    [TOPIC_BAILED] = TOPIC_PREFIX "stat/bailed",
    [TOPIC_BAIL_REASON] = TOPIC_PREFIX "stat/bail_reason",
    [TOPIC_STAT_TX] = TOPIC_PREFIX  "stat/tx",
    [TOPIC_CONF_ECO_MODE] = TOPIC_PREFIX  "conf/eco_mode",
    [TOPIC_CONF_SLEEP_MODE] = TOPIC_PREFIX  "conf/sleep_mode",
    [TOPIC_CONF_BREW_TEMP_TARGET] = TOPIC_PREFIX  "conf/brew_temp_target",
    [TOPIC_CONF_SERVICE_TEMP_TARGET] = TOPIC_PREFIX  "conf/service_temp_target",
    [TOPIC_CONF_BREW_TEMP_OFFSET] = TOPIC_PREFIX  "conf/brew_temp_offset",
    [TOPIC_CONF_BREW_PID_KP] = TOPIC_PREFIX "conf/brew_pid/kp",
    [TOPIC_CONF_BREW_PID_KI] = TOPIC_PREFIX "conf/brew_pid/ki",
    [TOPIC_CONF_BREW_PID_KD] = TOPIC_PREFIX "conf/brew_pid/kd",
    [TOPIC_CONF_SERVICE_PID_KP] = TOPIC_PREFIX "conf/service_pid/kp",
    [TOPIC_CONF_SERVICE_PID_KI] = TOPIC_PREFIX "conf/service_pid/ki",
    [TOPIC_CONF_SERVICE_PID_KD] = TOPIC_PREFIX "conf/service_pid/kd",
    [TOPIC_STAT_RX] = TOPIC_PREFIX  "stat/rx",
    [TOPIC_STAT_WATER_TANK_EMPTY] = TOPIC_PREFIX  "stat/water_tank_empty",
    [TOPIC_STAT_BREW_PID_P] = TOPIC_PREFIX "stat/brew_pid/p",
    [TOPIC_STAT_BREW_PID_I] = TOPIC_PREFIX "stat/brew_pid/i",
    [TOPIC_STAT_BREW_PID_D] = TOPIC_PREFIX "stat/brew_pid/d",
    [TOPIC_STAT_BREW_PID_INTEGRAL] = TOPIC_PREFIX "stat/brew_pid/integral",
    [TOPIC_STAT_SERVICE_PID_P] = TOPIC_PREFIX "stat/service_pid/p",
    [TOPIC_STAT_SERVICE_PID_I] = TOPIC_PREFIX "stat/service_pid/i",
    [TOPIC_STAT_SERVICE_PID_D] = TOPIC_PREFIX "stat/service_pid/d",
    [TOPIC_STAT_SERVICE_PID_INTEGRAL] = TOPIC_PREFIX "stat/service_pid/integral",
    [TOPIC_TEMP_BREW] = TOPIC_PREFIX  "temp/brew",
    [TOPIC_TEMP_SERVICE] = TOPIC_PREFIX  "temp/service",
    [TOPIC_SET_CONF_ECO_MODE] = TOPIC_PREFIX "conf/eco_mode/set",
    [TOPIC_SET_CONF_SLEEP_MODE] = TOPIC_PREFIX "conf/sleep_mode/set",
    [TOPIC_SET_CONF_BREW_TEMP_TARGET] = TOPIC_PREFIX "conf/brew_temp_target/set",
    [TOPIC_SET_CONF_SERVICE_TEMP_TARGET] = TOPIC_PREFIX "conf/service_temp_target/set",
    [TOPIC_SET_CONF_BREW_TEMP_OFFSET] = TOPIC_PREFIX "conf/brew_temp_offset/set",
    [TOPIC_SET_CONF_BREW_PID_KP] = TOPIC_PREFIX "conf/brew_pid/kp/set",
    [TOPIC_SET_CONF_BREW_PID_KI] = TOPIC_PREFIX "conf/brew_pid/ki/set",
    [TOPIC_SET_CONF_BREW_PID_KD] = TOPIC_PREFIX "conf/brew_pid/kd/set",
    [TOPIC_SET_CONF_SERVICE_PID_KP] = TOPIC_PREFIX "conf/service_pid/kp/set",
    [TOPIC_SET_CONF_SERVICE_PID_KI] = TOPIC_PREFIX "conf/service_pid/ki/set",
    [TOPIC_SET_CONF_SERVICE_PID_KD] = TOPIC_PREFIX "conf/service_pid/kd/set"
};

using namespace std::chrono_literals;

WifiTransceiver::WifiTransceiver(SystemStatus *systemStatus,
                                 SystemSettings *systemSettings,
                                 PicoQueue<SystemControllerCommand>* commandQueue)
                                 :
                                 systemStatus(systemStatus),
                                 systemSettings(systemSettings),
                                 commandQueue(commandQueue) {}

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
        //printf("Attempting to connect to WiFi, status: %u\n", status);
        status = wifi.begin(WIFI_SSID, WIFI_PASS);

        rtos::ThisThread::sleep_for(5s);
    }

    //printf("Connected to WiFi\n");
    systemStatus->wifiConnected = true;
}

void WifiTransceiver::ensureConnectedToMqtt() {
    while (!pubSubClient.connected()) {
        systemStatus->mqttConnected = false;
        //printf("Attempting to connect to MQTT\n");
        if (pubSubClient.connect("lcc", topic_names[TOPIC_ONLINE], 0, true, "false")) {
            systemStatus->mqttConnected = true;
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_ECO_MODE]);
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_SLEEP_MODE]);
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_BREW_TEMP_TARGET]);
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_SERVICE_TEMP_TARGET]);
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_BREW_TEMP_OFFSET]);
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_BREW_PID_KP]);
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_BREW_PID_KI]);
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_BREW_PID_KD]);
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_SERVICE_PID_KP]);
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_SERVICE_PID_KI]);
            pubSubClient.subscribe(topic_names[TOPIC_SET_CONF_SERVICE_PID_KD]);
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

    //printf("Data: %s\n", payloadZero);

    if (!strcmp(topic_names[TOPIC_SET_CONF_ECO_MODE], topic)) {
        systemSettings->setEcoMode(!strcmp("true", reinterpret_cast<const char *>(payloadZero)));
    } else if (!strcmp(topic_names[TOPIC_SET_CONF_SLEEP_MODE], topic)) {
        bool sleep = !strcmp("true", reinterpret_cast<const char *>(payloadZero));

        SystemControllerCommand command{.type = COMMAND_SET_SLEEP_MODE, .bool1 = sleep};
        commandQueue->tryAdd(&command);
    } else if (!strcmp(topic_names[TOPIC_SET_CONF_BREW_TEMP_TARGET], topic)) {
        float target = strtof(reinterpret_cast<const char *>(payloadZero), nullptr);

        if (target > 10.0f && target < 120.0f) {
            systemSettings->setOffsetTargetBrewTemp(target);
        }
    } else if (!strcmp(topic_names[TOPIC_SET_CONF_SERVICE_TEMP_TARGET], topic)) {
        float target = strtof(reinterpret_cast<const char *>(payloadZero), nullptr);

        if (target > 10.0f && target < 150.0f) {
            systemSettings->setTargetServiceTemp(target);
        }
    } else if (!strcmp(topic_names[TOPIC_SET_CONF_BREW_TEMP_OFFSET], topic)) {
        float offset = strtof(reinterpret_cast<const char *>(payloadZero), nullptr);

        if (offset > -30.0f && offset < 30.0f) {
            systemSettings->setBrewTemperatureOffset(offset);
        }
    } else if (!strcmp(topic_names[TOPIC_SET_CONF_BREW_PID_KP], topic)) {
        float param = strtof(reinterpret_cast<const char *>(payloadZero), nullptr);

        auto oldParams = systemStatus->getBrewPidSettings();
        auto newParams = PidSettings();
        newParams.Kp = param;
        newParams.Ki = oldParams.Ki;
        newParams.Kd = oldParams.Kd;
        systemSettings->setBrewPidParameters(newParams);
    } else if (!strcmp(topic_names[TOPIC_SET_CONF_BREW_PID_KI], topic)) {
        float param = strtof(reinterpret_cast<const char *>(payloadZero), nullptr);

        auto oldParams = systemStatus->getBrewPidSettings();
        auto newParams = PidSettings();
        newParams.Kp = oldParams.Kp;
        newParams.Ki = param;
        newParams.Kd = oldParams.Kd;
        systemSettings->setBrewPidParameters(newParams);
    } else if (!strcmp(topic_names[TOPIC_SET_CONF_BREW_PID_KD], topic)) {
        float param = strtof(reinterpret_cast<const char *>(payloadZero), nullptr);

        auto oldParams = systemStatus->getBrewPidSettings();
        auto newParams = PidSettings();
        newParams.Kp = oldParams.Kp;
        newParams.Ki = oldParams.Ki;
        newParams.Kd = param;
        systemSettings->setBrewPidParameters(newParams);
    } else if (!strcmp(topic_names[TOPIC_SET_CONF_SERVICE_PID_KP], topic)) {
        float param = strtof(reinterpret_cast<const char *>(payloadZero), nullptr);

        auto oldParams = systemStatus->getServicePidSettings();
        auto newParams = PidSettings();
        newParams.Kp = param;
        newParams.Ki = oldParams.Ki;
        newParams.Kd = oldParams.Kd;
        systemSettings->setServicePidParameters(newParams);
    } else if (!strcmp(topic_names[TOPIC_SET_CONF_SERVICE_PID_KI], topic)) {
        float param = strtof(reinterpret_cast<const char *>(payloadZero), nullptr);

        auto oldParams = systemStatus->getServicePidSettings();
        auto newParams = PidSettings();
        newParams.Kp = oldParams.Kp;
        newParams.Ki = param;
        newParams.Kd = oldParams.Kd;
        systemSettings->setServicePidParameters(newParams);
    } else if (!strcmp(topic_names[TOPIC_SET_CONF_SERVICE_PID_KD], topic)) {
        float param = strtof(reinterpret_cast<const char *>(payloadZero), nullptr);

        auto oldParams = systemStatus->getServicePidSettings();
        auto newParams = PidSettings();
        newParams.Kp = oldParams.Kp;
        newParams.Ki = oldParams.Ki;
        newParams.Kd = param;
        systemSettings->setServicePidParameters(newParams);
    }

    free(payloadZero);
}

void WifiTransceiver::publishStatus() {
    publish(topic_names[TOPIC_ONLINE], true);
    publish(topic_names[TOPIC_BAILED], systemStatus->hasBailed());
    publish(topic_names[TOPIC_BAIL_REASON], (uint8_t)systemStatus->bailReason());

    publish(topic_names[TOPIC_STAT_TX], systemStatus->hasSentLccPacket);

    publish(topic_names[TOPIC_CONF_ECO_MODE], systemStatus->isInEcoMode());
    publish(topic_names[TOPIC_CONF_SLEEP_MODE], systemStatus->isInSleepMode());
    publish(topic_names[TOPIC_CONF_BREW_TEMP_TARGET], systemStatus->getOffsetTargetBrewTemperature());
    publish(topic_names[TOPIC_CONF_SERVICE_TEMP_TARGET], systemStatus->getTargetServiceTemp());
    publish(topic_names[TOPIC_CONF_BREW_TEMP_OFFSET], systemStatus->getBrewTempOffset());
    auto brewPidParams = systemStatus->getBrewPidSettings();
    publish(topic_names[TOPIC_CONF_BREW_PID_KP], brewPidParams.Kp);
    publish(topic_names[TOPIC_CONF_BREW_PID_KI], brewPidParams.Ki);
    publish(topic_names[TOPIC_CONF_BREW_PID_KD], brewPidParams.Kd);
    auto servicePidParams = systemStatus->getServicePidSettings();
    publish(topic_names[TOPIC_CONF_SERVICE_PID_KP], servicePidParams.Kp);
    publish(topic_names[TOPIC_CONF_SERVICE_PID_KI], servicePidParams.Ki);
    publish(topic_names[TOPIC_CONF_SERVICE_PID_KD], servicePidParams.Kd);

    auto brewPidRuntimeParams = systemStatus->getBrewPidRuntimeParameters();

    publish(topic_names[TOPIC_STAT_BREW_PID_P], brewPidRuntimeParams.p);
    publish(topic_names[TOPIC_STAT_BREW_PID_I], brewPidRuntimeParams.i);
    publish(topic_names[TOPIC_STAT_BREW_PID_D], brewPidRuntimeParams.d);
    publish(topic_names[TOPIC_STAT_BREW_PID_INTEGRAL], brewPidRuntimeParams.integral);

    auto servicePidRuntimeParams = systemStatus->getServicePidRuntimeParameters();

    publish(topic_names[TOPIC_STAT_SERVICE_PID_P], servicePidRuntimeParams.p);
    publish(topic_names[TOPIC_STAT_SERVICE_PID_I], servicePidRuntimeParams.i);
    publish(topic_names[TOPIC_STAT_SERVICE_PID_D], servicePidRuntimeParams.d);
    publish(topic_names[TOPIC_STAT_SERVICE_PID_INTEGRAL], servicePidRuntimeParams.integral);

    if (systemStatus->hasReceivedControlBoardPacket) {
        publish(topic_names[TOPIC_STAT_RX], true);

        publish(topic_names[TOPIC_STAT_WATER_TANK_EMPTY], systemStatus->isWaterTankEmpty());
        publish(topic_names[TOPIC_TEMP_BREW], systemStatus->getOffsetBrewTemperature());
        publish(topic_names[TOPIC_TEMP_SERVICE], systemStatus->getServiceTemperature());
    } else {
        publish(topic_names[TOPIC_STAT_RX],false);
    }
}

void WifiTransceiver::publish(const char *topic, bool payload) {
    pubSubClient.publish(topic,payload ? "true" : "false");
    handleYield();
}

void WifiTransceiver::publish(const char *topic, uint8_t payload) {
    uint8_t intString[4];
    unsigned int len = snprintf(reinterpret_cast<char *>(intString), 4, "%u", payload);
    pubSubClient.publish(topic, intString, len);
    handleYield();
}

void WifiTransceiver::publish(const char *topic, float payload) {
    uint8_t floatString[FLOAT_MAX_LEN];
    unsigned int len = snprintf(reinterpret_cast<char *>(floatString), FLOAT_MAX_LEN, "%.2f", payload);
    pubSubClient.publish(topic, floatString, len);
    handleYield();
}

void WifiTransceiver::publish(const char *topic, double payload) {
    uint8_t floatString[FLOAT_MAX_LEN];
    unsigned int len = snprintf(reinterpret_cast<char *>(floatString), FLOAT_MAX_LEN, "%.2f", payload);
    pubSubClient.publish(topic, floatString, len);
    handleYield();
}

void WifiTransceiver::handleYield() {
    pubSubClient.loop();
    rtos::ThisThread::yield();
}

