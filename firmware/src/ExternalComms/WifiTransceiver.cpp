//
// Created by Magnus Nordlander on 2021-07-16.
//

#include "WifiTransceiver.h"

#include <mbed.h>
#include <functional>
#include <lccmacros.h>

#define WIFI_PASS "pass"
#define WIFI_SSID "IoT Transform"
#define MQTT_HOST 192,168,10,66
#define MQTT_PORT 1883

#define FLOAT_MAX_LEN 7

#define TOPIC_PREFIX "lcc"
#define FIRMWARE_PREFIX "smart-lcc"

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
    pubSubClient.setBufferSize(1024);

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

    if (!topicsInitialized) {
        printf("Connected to WiFi\n");
        formatTopics();
        topicsInitialized = true;
    }
    systemStatus->wifiConnected = true;
}

void WifiTransceiver::ensureConnectedToMqtt() {
    if (!pubSubClient.connected() && systemStatus->mqttConnected) {
        printf("MQTT connection lost\n");
    }

    while (!pubSubClient.connected()) {
        systemStatus->mqttConnected = false;

        printf("Attempting to connect to MQTT\n");
        if (pubSubClient.connect("lcc", &TOPIC_ONLINE[0], 0, true, "false")) {
            printf("MQTT connected\n");
            systemStatus->mqttConnected = true;
            pubSubClient.subscribe(&TOPIC_SET_CONF_ECO_MODE[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_SLEEP_MODE[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_BREW_TEMP_TARGET[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_SERVICE_TEMP_TARGET[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_BREW_TEMP_OFFSET[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_BREW_PID_KP[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_BREW_PID_KI[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_BREW_PID_KD[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_BREW_PID_WINDUP_LOW[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_BREW_PID_WINDUP_HIGH[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_SERVICE_PID_KP[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_SERVICE_PID_KI[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_SERVICE_PID_KD[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_BREW_PID_WINDUP_LOW[0]);
            pubSubClient.subscribe(&TOPIC_SET_CONF_BREW_PID_WINDUP_HIGH[0]);

            publishAutoconfig();

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

    if (!strcmp(&TOPIC_SET_CONF_ECO_MODE[0], topic)) {
        systemSettings->setEcoMode(!strcmp("true", reinterpret_cast<const char *>(payloadZero)));
    } else if (!strcmp(&TOPIC_SET_CONF_SLEEP_MODE[0], topic)) {
        bool sleep = !strcmp("true", reinterpret_cast<const char *>(payloadZero));

        SystemControllerCommand command{.type = COMMAND_SET_SLEEP_MODE, .bool1 = sleep};
        commandQueue->tryAdd(&command);
    } else { // Payload is a float
        float floatPayload = strtof(reinterpret_cast<const char *>(payloadZero), nullptr);

        if (!strcmp(&TOPIC_SET_CONF_BREW_TEMP_TARGET[0], topic)) {
            if (floatPayload > 10.0f && floatPayload < 120.0f) {
                systemSettings->setOffsetTargetBrewTemp(floatPayload);
            }
        } else if (!strcmp(&TOPIC_SET_CONF_SERVICE_TEMP_TARGET[0], topic)) {
            if (floatPayload > 10.0f && floatPayload < 150.0f) {
                systemSettings->setTargetServiceTemp(floatPayload);
            }
        } else if (!strcmp(&TOPIC_SET_CONF_BREW_TEMP_OFFSET[0], topic)) {
            if (floatPayload > -30.0f && floatPayload < 30.0f) {
                systemSettings->setBrewTemperatureOffset(floatPayload);
            }
        } else if (!strcmp(&TOPIC_SET_CONF_BREW_PID_KP[0], topic)) {
            auto settings = systemStatus->getBrewPidSettings();
            settings.Kp = floatPayload;
            systemSettings->setBrewPidParameters(settings);
        } else if (!strcmp(&TOPIC_SET_CONF_BREW_PID_KI[0], topic)) {
            auto settings = systemStatus->getBrewPidSettings();
            settings.Ki = floatPayload;
            systemSettings->setBrewPidParameters(settings);
        } else if (!strcmp(&TOPIC_SET_CONF_BREW_PID_KD[0], topic)) {
            auto settings = systemStatus->getBrewPidSettings();
            settings.Kd = floatPayload;
            systemSettings->setBrewPidParameters(settings);
        } else if (!strcmp(&TOPIC_SET_CONF_BREW_PID_WINDUP_LOW[0], topic)) {
            auto settings = systemStatus->getBrewPidSettings();
            settings.windupLow = floatPayload;
            systemSettings->setBrewPidParameters(settings);
        } else if (!strcmp(&TOPIC_SET_CONF_BREW_PID_WINDUP_HIGH[0], topic)) {
            auto settings = systemStatus->getBrewPidSettings();
            settings.windupHigh = floatPayload;
            systemSettings->setBrewPidParameters(settings);
        } else if (!strcmp(&TOPIC_SET_CONF_SERVICE_PID_KP[0], topic)) {
            auto settings = systemStatus->getServicePidSettings();
            settings.Kp = floatPayload;
            systemSettings->setServicePidParameters(settings);
        } else if (!strcmp(&TOPIC_SET_CONF_SERVICE_PID_KI[0], topic)) {
            auto settings = systemStatus->getServicePidSettings();
            settings.Ki = floatPayload;
            systemSettings->setServicePidParameters(settings);
        } else if (!strcmp(&TOPIC_SET_CONF_SERVICE_PID_KD[0], topic)) {
            auto settings = systemStatus->getServicePidSettings();
            settings.Kd = floatPayload;
            systemSettings->setServicePidParameters(settings);
        } else if (!strcmp(&TOPIC_SET_CONF_SERVICE_PID_WINDUP_LOW[0], topic)) {
            auto settings = systemStatus->getServicePidSettings();
            settings.windupLow = floatPayload;
            systemSettings->setServicePidParameters(settings);
        } else if (!strcmp(&TOPIC_SET_CONF_SERVICE_PID_WINDUP_HIGH[0], topic)) {
            auto settings = systemStatus->getServicePidSettings();
            settings.windupHigh = floatPayload;
            systemSettings->setServicePidParameters(settings);
        }
    }

    free(payloadZero);
}

void WifiTransceiver::publishStatus() {
    if (rtos::Kernel::Clock::now() - lastFullSend > 10s) {
        doFullSend = true;
        lastFullSend = rtos::Kernel::Clock::now();
    }

    publish(&TOPIC_ONLINE[0], true, DEDUP_ONLINE);

    publish(&TOPIC_STATE[0], systemStatus->getState(), DEDUP_STATE);
    publish(&TOPIC_BAILED[0], systemStatus->hasBailed(), DEDUP_BAILED);
    publish(&TOPIC_BAIL_REASON[0], systemStatus->bailReason(), DEDUP_BAIL_REASON);
    publish(&TOPIC_RESET_REASON[0], mbed::ResetReason::get(), DEDUP_RESET_REASON);

    publish(&TOPIC_STAT_TX[0], systemStatus->hasSentLccPacket, DEDUP_STAT_TX);

    publish(&TOPIC_CONF_ECO_MODE[0], systemStatus->isInEcoMode(), DEDUP_CONF_ECO_MODE);
    publish(&TOPIC_CONF_SLEEP_MODE[0], systemStatus->isInSleepMode(), DEDUP_CONF_SLEEP_MODE);
    publish(&TOPIC_CONF_BREW_TEMP_TARGET[0], systemStatus->getOffsetTargetBrewTemperature(), DEDUP_CONF_BREW_TEMP_TARGET);
    publish(&TOPIC_CONF_SERVICE_TEMP_TARGET[0], systemStatus->getTargetServiceTemp(), DEDUP_CONF_SERVICE_TEMP_TARGET);
    publish(&TOPIC_CONF_BREW_TEMP_OFFSET[0], systemStatus->getBrewTempOffset(), DEDUP_CONF_BREW_TEMP_OFFSET);
    auto brewPidParams = systemStatus->getBrewPidSettings();
    publish(&TOPIC_CONF_BREW_PID_KP[0], brewPidParams.Kp, DEDUP_CONF_BREW_PID_KP);
    publish(&TOPIC_CONF_BREW_PID_KI[0], brewPidParams.Ki, DEDUP_CONF_BREW_PID_KI);
    publish(&TOPIC_CONF_BREW_PID_KD[0], brewPidParams.Kd, DEDUP_CONF_BREW_PID_KD);
    publish(&TOPIC_CONF_BREW_PID_WINDUP_LOW[0], brewPidParams.windupLow, DEDUP_CONF_BREW_PID_WINDUP_LOW);
    publish(&TOPIC_CONF_BREW_PID_WINDUP_HIGH[0], brewPidParams.windupHigh, DEDUP_CONF_BREW_PID_WINDUP_HIGH);
    auto servicePidParams = systemStatus->getServicePidSettings();
    publish(&TOPIC_CONF_SERVICE_PID_KP[0], servicePidParams.Kp, DEDUP_CONF_SERVICE_PID_KP);
    publish(&TOPIC_CONF_SERVICE_PID_KI[0], servicePidParams.Ki, DEDUP_CONF_SERVICE_PID_KI);
    publish(&TOPIC_CONF_SERVICE_PID_KD[0], servicePidParams.Kd, DEDUP_CONF_SERVICE_PID_KD);
    publish(&TOPIC_CONF_SERVICE_PID_WINDUP_LOW[0], servicePidParams.windupLow, DEDUP_CONF_SERVICE_PID_WINDUP_LOW);
    publish(&TOPIC_CONF_SERVICE_PID_WINDUP_HIGH[0], servicePidParams.windupHigh, DEDUP_CONF_SERVICE_PID_WINDUP_HIGH);

    auto brewPidRuntimeParams = systemStatus->getBrewPidRuntimeParameters();

    publish(&TOPIC_STAT_BREW_PID_P[0], brewPidRuntimeParams.p, DEDUP_STAT_BREW_PID_P);
    publish(&TOPIC_STAT_BREW_PID_I[0], brewPidRuntimeParams.i, DEDUP_STAT_BREW_PID_I);
    publish(&TOPIC_STAT_BREW_PID_D[0], brewPidRuntimeParams.d, DEDUP_STAT_BREW_PID_D);
    publish(&TOPIC_STAT_BREW_PID_INTEGRAL[0], brewPidRuntimeParams.integral, DEDUP_STAT_BREW_PID_INTEGRAL);
    publish(&TOPIC_STAT_BREW_PID_HYSTERESIS[0], brewPidRuntimeParams.hysteresisMode, DEDUP_STAT_BREW_PID_HYSTERESIS);

    auto servicePidRuntimeParams = systemStatus->getServicePidRuntimeParameters();

    publish(&TOPIC_STAT_SERVICE_PID_P[0], servicePidRuntimeParams.p, DEDUP_STAT_SERVICE_PID_P);
    publish(&TOPIC_STAT_SERVICE_PID_I[0], servicePidRuntimeParams.i, DEDUP_STAT_SERVICE_PID_I);
    publish(&TOPIC_STAT_SERVICE_PID_D[0], servicePidRuntimeParams.d, DEDUP_STAT_SERVICE_PID_D);
    publish(&TOPIC_STAT_SERVICE_PID_INTEGRAL[0], servicePidRuntimeParams.integral, DEDUP_STAT_SERVICE_PID_INTEGRAL);
    publish(&TOPIC_STAT_SERVICE_PID_HYSTERESIS[0], servicePidRuntimeParams.hysteresisMode, DEDUP_STAT_SERVICE_PID_HYSTERESIS);

    if (systemStatus->hasReceivedControlBoardPacket) {
        publish(&TOPIC_STAT_RX[0], true, DEDUP_STAT_RX);

        publish(&TOPIC_STAT_WATER_TANK_EMPTY[0], systemStatus->isWaterTankEmpty(), DEDUP_STAT_WATER_TANK_EMPTY);
        publish(&TOPIC_TEMP_BREW[0], systemStatus->getOffsetBrewTemperature(), DEDUP_TEMP_BREW);
        publish(&TOPIC_TEMP_SERVICE[0], systemStatus->getServiceTemperature(), DEDUP_TEMP_SERVICE);
    } else {
        publish(&TOPIC_STAT_RX[0],false, DEDUP_STAT_RX);
    }

    doFullSend = false;
}

void WifiTransceiver::publish(const char *topic, bool payload, bool &dedup) {
    if(payload == dedup && !doFullSend) {
        return;
    }

    dedup = payload;
    pubSubClient.publish(topic,payload ? "true" : "false");
    handleYield();
}

void WifiTransceiver::publish(const char *topic, uint8_t payload, uint8_t &dedup) {
    if(payload == dedup && !doFullSend) {
        return;
    }

    dedup = payload;
    char intString[4];
    unsigned int len = snprintf(intString, 4, "%u", payload);
    pubSubClient.publish(topic, intString, len);
    handleYield();
}

void WifiTransceiver::publish(const char *topic, float payload, float &dedup) {
    if(payload == dedup && !doFullSend) {
        return;
    }

    dedup = payload;

    char floatString[FLOAT_MAX_LEN];

    unsigned int len = snprintf(floatString, FLOAT_MAX_LEN, FUCKED_UP_FLOAT_2_FMT, FUCKED_UP_FLOAT_2_ARG(payload));

    pubSubClient.publish(topic, floatString, len);
    handleYield();
}

void WifiTransceiver::publish(const char *topic, double payload, double &dedup) {
    if(payload == dedup && !doFullSend) {
        return;
    }

    dedup = payload;

    char floatString[FLOAT_MAX_LEN];

    unsigned int len = snprintf(floatString, FLOAT_MAX_LEN, FUCKED_UP_FLOAT_2_FMT, FUCKED_UP_FLOAT_2_ARG(payload));

    pubSubClient.publish(topic, floatString, len);
    handleYield();
}

void WifiTransceiver::publish(const char *topic, SystemControllerBailReason bailReason, SystemControllerBailReason &dedup) {
    if(bailReason == dedup && !doFullSend) {
        return;
    }

    dedup = bailReason;

    switch (bailReason) {
        case BAIL_REASON_NONE:
            pubSubClient.publish(topic, "NONE");
            break;
        case BAIL_REASON_CB_UNRESPONSIVE:
            pubSubClient.publish(topic, "CB_UNRESPONSIVE");
            break;
        case BAIL_REASON_CB_PACKET_INVALID:
            pubSubClient.publish(topic, "CB_PACKET_INVALID");
            break;
        case BAIL_REASON_LCC_PACKET_INVALID:
            pubSubClient.publish(topic, "LCC_PACKET_INVALID");
            break;
        case BAIL_REASON_SSR_QUEUE_EMPTY:
            pubSubClient.publish(topic, "SSR_QUEUE_EMPTY");
            break;
    }

    handleYield();
}

void WifiTransceiver::publish(const char *topic, SystemControllerState state, SystemControllerState &dedup) {
    if(state == dedup && !doFullSend) {
        return;
    }

    dedup = state;

    switch (state) {
        case SYSTEM_CONTROLLER_STATE_UNDETERMINED:
            pubSubClient.publish(topic, "UNDETERMINED");
            break;
        case SYSTEM_CONTROLLER_STATE_HEATUP:
            pubSubClient.publish(topic, "HEATUP");
            break;
        case SYSTEM_CONTROLLER_STATE_TEMPS_NORMALIZING:
            pubSubClient.publish(topic, "TEMPS_NORMALIZING");
            break;
        case SYSTEM_CONTROLLER_STATE_WARM:
            pubSubClient.publish(topic, "WARM");
            break;
        case SYSTEM_CONTROLLER_STATE_SLEEPING:
            pubSubClient.publish(topic, "SLEEPING");
            break;
        case SYSTEM_CONTROLLER_STATE_BAILED:
            pubSubClient.publish(topic, "BAILED");
            break;
        case SYSTEM_CONTROLLER_STATE_FIRST_RUN:
            pubSubClient.publish(topic, "FIRST_RUN");
            break;
    }

    handleYield();
}

void WifiTransceiver::publish(const char *topic, reset_reason_t payload, reset_reason_t &dedup) {
    if(payload == dedup && !doFullSend) {
        return;
    }

    dedup = payload;

    switch (payload) {
        case RESET_REASON_POWER_ON:
            pubSubClient.publish(topic, "POWER_ON");
            break;
        case RESET_REASON_PIN_RESET:
            pubSubClient.publish(topic, "PIN_RESET");
            break;
        case RESET_REASON_BROWN_OUT:
            pubSubClient.publish(topic, "BROWN_OUT");
            break;
        case RESET_REASON_SOFTWARE:
            pubSubClient.publish(topic, "SOFTWARE");
            break;
        case RESET_REASON_WATCHDOG:
            pubSubClient.publish(topic, "WATCHDOG");
            break;
        case RESET_REASON_LOCKUP:
            pubSubClient.publish(topic, "LOCKUP");
            break;
        case RESET_REASON_WAKE_LOW_POWER:
            pubSubClient.publish(topic, "WAKE_LOW_POWER");
            break;
        case RESET_REASON_ACCESS_ERROR:
            pubSubClient.publish(topic, "ACCESS_ERROR");
            break;
        case RESET_REASON_BOOT_ERROR:
            pubSubClient.publish(topic, "BOOT_ERROR");
            break;
        case RESET_REASON_MULTIPLE:
            pubSubClient.publish(topic, "MULTIPLE");
            break;
        case RESET_REASON_PLATFORM:
            pubSubClient.publish(topic, "PLATFORM");
            break;
        case RESET_REASON_UNKNOWN:
            pubSubClient.publish(topic, "UNKNOWN");
            break;
    }

    handleYield();
}

void WifiTransceiver::handleYield() {
    pubSubClient.loop();
    rtos::ThisThread::yield();
}

void WifiTransceiver::formatTopics() {
    uint8_t mac[WL_MAC_ADDR_LENGTH];
    wifi.macAddress(mac);

    snprintf(identifier, sizeof(identifier), "LCC-%02X%02X%02X", mac[3], mac[4], mac[5]);

    snprintf(TOPIC_ONLINE, TOPIC_LENGTH - 1, "%s/%s/online", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STATE, TOPIC_LENGTH - 1, "%s/%s/stat/state", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_BAILED, TOPIC_LENGTH - 1, "%s/%s/stat/bailed", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_BAIL_REASON, TOPIC_LENGTH - 1, "%s/%s/stat/bail_reason", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_RESET_REASON, TOPIC_LENGTH - 1, "%s/%s/stat/reset_reason", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_TX, TOPIC_LENGTH - 1,  "%s/%s/stat/tx", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_ECO_MODE, TOPIC_LENGTH - 1,  "%s/%s/conf/eco_mode", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_SLEEP_MODE, TOPIC_LENGTH - 1,  "%s/%s/conf/sleep_mode", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_BREW_TEMP_TARGET, TOPIC_LENGTH - 1,  "%s/%s/conf/brew_temp_target", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_SERVICE_TEMP_TARGET, TOPIC_LENGTH - 1,  "%s/%s/conf/service_temp_target", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_BREW_TEMP_OFFSET, TOPIC_LENGTH - 1,  "%s/%s/conf/brew_temp_offset", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_BREW_PID_KP, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/kp", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_BREW_PID_KI, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/ki", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_BREW_PID_KD, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/kd", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_BREW_PID_WINDUP_LOW, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/windup_low", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_BREW_PID_WINDUP_HIGH, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/windup_high", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_SERVICE_PID_KP, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/kp", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_SERVICE_PID_KI, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/ki", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_SERVICE_PID_KD, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/kd", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_SERVICE_PID_WINDUP_LOW, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/windup_low", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_SERVICE_PID_WINDUP_HIGH, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/windup_high", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_RX, TOPIC_LENGTH - 1,  "%s/%s/stat/rx", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_WATER_TANK_EMPTY, TOPIC_LENGTH - 1,  "%s/%s/stat/water_tank_empty", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_BREW_PID_P, TOPIC_LENGTH - 1, "%s/%s/stat/brew_pid/p", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_BREW_PID_I, TOPIC_LENGTH - 1, "%s/%s/stat/brew_pid/i", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_BREW_PID_D, TOPIC_LENGTH - 1, "%s/%s/stat/brew_pid/d", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_BREW_PID_INTEGRAL, TOPIC_LENGTH - 1, "%s/%s/stat/brew_pid/integral", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_BREW_PID_HYSTERESIS, TOPIC_LENGTH - 1, "%s/%s/stat/brew_pid/hysteresis", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_SERVICE_PID_P, TOPIC_LENGTH - 1, "%s/%s/stat/service_pid/p", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_SERVICE_PID_I, TOPIC_LENGTH - 1, "%s/%s/stat/service_pid/i", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_SERVICE_PID_D, TOPIC_LENGTH - 1, "%s/%s/stat/service_pid/d", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_SERVICE_PID_INTEGRAL, TOPIC_LENGTH - 1, "%s/%s/stat/service_pid/integral", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_SERVICE_PID_HYSTERESIS, TOPIC_LENGTH - 1, "%s/%s/stat/service_pid/hysteresis", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_TEMP_BREW, TOPIC_LENGTH - 1,  "%s/%s/temp/brew", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_TEMP_SERVICE, TOPIC_LENGTH - 1,  "%s/%s/temp/service", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_ECO_MODE, TOPIC_LENGTH - 1, "%s/%s/conf/eco_mode/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SLEEP_MODE, TOPIC_LENGTH - 1, "%s/%s/conf/sleep_mode/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_TEMP_TARGET, TOPIC_LENGTH - 1, "%s/%s/conf/brew_temp_target/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SERVICE_TEMP_TARGET, TOPIC_LENGTH - 1, "%s/%s/conf/service_temp_target/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_TEMP_OFFSET, TOPIC_LENGTH - 1, "%s/%s/conf/brew_temp_offset/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_PID_KP, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/kp/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_PID_KI, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/ki/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_PID_KD, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/kd/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_PID_WINDUP_LOW, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/windup_low/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_PID_WINDUP_HIGH, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/windup_high/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SERVICE_PID_KP, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/kp/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SERVICE_PID_KI, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/ki/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SERVICE_PID_KD, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/kd/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SERVICE_PID_WINDUP_LOW, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/windup_low/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SERVICE_PID_WINDUP_HIGH, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/windup_high/set", TOPIC_PREFIX, identifier);

    snprintf(TOPIC_AUTOCONF_BREW_BOILER_SENSOR, 127, "homeassistant/sensor/%s/%s_brew_temp/config", FIRMWARE_PREFIX, identifier);
    snprintf(TOPIC_AUTOCONF_SERVICE_BOILER_SENSOR, 127, "homeassistant/sensor/%s/%s_serv_temp/config", FIRMWARE_PREFIX, identifier);
}

void WifiTransceiver::publishAutoconfig() {
    printf("Publishing autoconfig\n");

    char mqttPayload[1024];
    StaticJsonDocument<256> device;
    StaticJsonDocument<1024> autoconfPayload;
    size_t len;

    char macString[19];
    uint8_t mac[WL_MAC_ADDR_LENGTH];
    wifi.macAddress(mac);
    snprintf(macString, sizeof(macString), "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    device["ids"][0] = identifier;
    device["cns"][0][0] = "mac";
    device["cns"][0][1] = macString;
    device["mf"] = "magnusnordlander";
    device["mdl"] = "smart-lcc";
    device["name"] = identifier;
    device["sw"] = "0.1.0";

    JsonObject devObj = device.as<JsonObject>();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_ONLINE;
    autoconfPayload["pl_avail"] = "true";
    autoconfPayload["pl_not_avail"] = "false";
    autoconfPayload["stat_t"] = TOPIC_TEMP_BREW;
    autoconfPayload["name"] = identifier + String(" Brew Boiler Temperature");
    autoconfPayload["uniq_id"] = identifier + String("_brew_temp");
    autoconfPayload["unit_of_meas"] = "°C";
    autoconfPayload["ic"] = "mdi:thermometer";

    len = serializeJson(autoconfPayload, mqttPayload, 2048);
    printf("Payload: %s\n", mqttPayload);
    pubSubClient.publish(&TOPIC_AUTOCONF_BREW_BOILER_SENSOR[0], mqttPayload, len);
    autoconfPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_ONLINE;
    autoconfPayload["pl_avail"] = "true";
    autoconfPayload["pl_not_avail"] = "false";
    autoconfPayload["stat_t"] = TOPIC_TEMP_SERVICE;
    autoconfPayload["name"] = identifier + String(" Service Boiler Temperature");
    autoconfPayload["uniq_id"] = identifier + String("_serv_temp");
    autoconfPayload["unit_of_meas"] = "°C";
    autoconfPayload["ic"] = "mdi:thermometer";

    len = serializeJson(autoconfPayload, mqttPayload, 2048);
    printf("Payload: %s\n", mqttPayload);
    pubSubClient.publish(&TOPIC_AUTOCONF_SERVICE_BOILER_SENSOR[0], mqttPayload, len);
    autoconfPayload.clear();

    printf("Done\n");
}