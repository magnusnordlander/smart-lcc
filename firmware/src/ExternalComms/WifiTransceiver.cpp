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
        if (pubSubClient.connect(identifier, &TOPIC_ONLINE[0], 0, true, "offline")) {
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
        systemSettings->setEcoMode(!strcmp("ON", reinterpret_cast<const char *>(payloadZero)));
    } else if (!strcmp(&TOPIC_SET_CONF_SLEEP_MODE[0], topic)) {
        systemSettings->setSleepMode(!strcmp("ON", reinterpret_cast<const char *>(payloadZero)));
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
    if ((rtos::Kernel::Clock::now() - lastFullSend) > 10s) {
        doFullSend = true;
        lastFullSend = rtos::Kernel::Clock::now();

        sendWifiStats();
    }

    pubSubClient.publish(&TOPIC_ONLINE[0], "online");
    handleYield();

    publish(&TOPIC_STATE[0], systemStatus->getState(), DEDUP_STATE);

    InternalStateMessage internalStateMessage = {
            .bailed = systemStatus->hasBailed(),
            .bailReason = systemStatus->bailReason(),
            .resetReason = mbed::ResetReason::get(),
            .rx = systemStatus->hasReceivedControlBoardPacket,
            .tx = systemStatus->hasSentLccPacket
    };

    publish(&TOPIC_STAT_INTERNAL[0], internalStateMessage, DEDUP_STAT_INTERNAL);

    publish(&TOPIC_CONF_ECO_MODE[0], systemStatus->isInEcoMode(), DEDUP_CONF_ECO_MODE, true);
    publish(&TOPIC_CONF_SLEEP_MODE[0], systemStatus->isInSleepMode(), DEDUP_CONF_SLEEP_MODE, true);
    publish(&TOPIC_CONF_BREW_TEMP_TARGET[0], systemStatus->getOffsetTargetBrewTemperature(), DEDUP_CONF_BREW_TEMP_TARGET);
    publish(&TOPIC_CONF_SERVICE_TEMP_TARGET[0], systemStatus->getTargetServiceTemp(), DEDUP_CONF_SERVICE_TEMP_TARGET);
    publish(&TOPIC_CONF_BREW_TEMP_OFFSET[0], systemStatus->getBrewTempOffset(), DEDUP_CONF_BREW_TEMP_OFFSET);

    publish(&TOPIC_CONF_BREW_PID[0], systemStatus->getBrewPidSettings(), DEDUP_CONF_BREW_PID);
    publish(&TOPIC_CONF_SERVICE_PID[0], systemStatus->getServicePidSettings(), DEDUP_CONF_SERVICE_PID);

    publish(&TOPIC_STAT_BREW_PID[0], systemStatus->getBrewPidRuntimeParameters(), DEDUP_STAT_BREW_PID);
    publish(&TOPIC_STAT_SERVICE_PID[0], systemStatus->getServicePidRuntimeParameters(), DEDUP_STAT_SERVICE_PID);

    if (systemStatus->hasReceivedControlBoardPacket) {
        publish(&TOPIC_STAT_WATER_TANK_EMPTY[0], systemStatus->isWaterTankEmpty(), DEDUP_STAT_WATER_TANK_EMPTY);
        publish(&TOPIC_TEMP_BREW[0], systemStatus->getOffsetBrewTemperature(), DEDUP_TEMP_BREW);
        publish(&TOPIC_TEMP_SERVICE[0], systemStatus->getServiceTemperature(), DEDUP_TEMP_SERVICE);
    }

    doFullSend = false;
}

void WifiTransceiver::publish(const char *topic, bool payload, bool &dedup, bool onOff) {
    if(payload == dedup && !doFullSend) {
        return;
    }

    dedup = payload;
    if (onOff) {
        pubSubClient.publish(topic,payload ? "ON" : "OFF");
    } else {
        pubSubClient.publish(topic,payload ? "true" : "false");
    }
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
    if(payload == dedup && !doFullSend) { // FIXME: Floating point comparison
        return;
    }

    dedup = payload;

    char floatString[FLOAT_MAX_LEN];

    unsigned int len = snprintf(floatString, FLOAT_MAX_LEN, FUCKED_UP_FLOAT_2_FMT, FUCKED_UP_FLOAT_2_ARG(payload));

    pubSubClient.publish(topic, floatString, len);
    handleYield();
}

void WifiTransceiver::publish(const char *topic, double payload, double &dedup) {
    if(payload == dedup && !doFullSend) { // FIXME: Floating point comparison
        return;
    }

    dedup = payload;

    char floatString[FLOAT_MAX_LEN];

    unsigned int len = snprintf(floatString, FLOAT_MAX_LEN, FUCKED_UP_FLOAT_2_FMT, FUCKED_UP_FLOAT_2_ARG(payload));

    pubSubClient.publish(topic, floatString, len);
    handleYield();
}

void WifiTransceiver::publish(const char *topic, PidRuntimeParameters payload, PidRuntimeParameters &dedup) {
    if(!doFullSend &&
        payload.hysteresisMode == dedup.hysteresisMode &&
        payload.p == dedup.p && // FIXME: Floating point comparison
        payload.i == dedup.i &&
        payload.d == dedup.d &&
        payload.integral == dedup.integral
      ) {
        return;
    }

    dedup = payload;

    char data[127];
    StaticJsonDocument<96> doc;

    doc["p"] = payload.p;
    doc["i"] = payload.i;
    doc["d"] = payload.d;
    doc["integral"] = payload.integral;
    doc["hysteresisMode"] = payload.hysteresisMode;

    size_t len = serializeJson(doc, data);
    pubSubClient.publish(topic, data, len);

    handleYield();
}

void WifiTransceiver::publish(const char *topic, PidSettings payload, PidSettings &dedup) {
    if(!doFullSend &&
       payload.Kp == dedup.Kp &&
       payload.Ki == dedup.Ki && // FIXME: Floating point comparison
       payload.Kd == dedup.Kd &&
       payload.windupHigh == dedup.windupHigh &&
       payload.windupLow == dedup.windupLow
            ) {
        return;
    }

    dedup = payload;

    char data[127];
    StaticJsonDocument<96> doc;

    doc["Kp"] = payload.Kp;
    doc["Ki"] = payload.Ki;
    doc["Kd"] = payload.Kd;
    doc["windupHigh"] = payload.windupHigh;
    doc["windupLow"] = payload.windupLow;

    size_t len = serializeJson(doc, data);
    pubSubClient.publish(topic, data, len);

    handleYield();
}

void WifiTransceiver::publish(const char *topic, InternalStateMessage payload, InternalStateMessage &dedup) {
    if(!doFullSend &&
       payload.rx == dedup.rx &&
       payload.tx == dedup.tx &&
       payload.bailed == dedup.bailed &&
       payload.bailReason == dedup.bailReason &&
       payload.resetReason == dedup.resetReason
            ) {
        return;
    }

    dedup = payload;

    char data[127];
    StaticJsonDocument<128> doc;

    doc["rx"] = payload.rx;
    doc["tx"] = payload.tx;
    doc["bailed"] = payload.bailed;

    switch (payload.bailReason) {
        case BAIL_REASON_NONE:
            doc["bail_reason"] = "NONE";
            break;
        case BAIL_REASON_CB_UNRESPONSIVE:
            doc["bail_reason"] = "CB_UNRESPONSIVE";
            break;
        case BAIL_REASON_CB_PACKET_INVALID:
            doc["bail_reason"] = "CB_PACKET_INVALID";
            break;
        case BAIL_REASON_LCC_PACKET_INVALID:
            doc["bail_reason"] = "LCC_PACKET_INVALID";
            break;
        case BAIL_REASON_SSR_QUEUE_EMPTY:
            doc["bail_reason"] = "SSR_QUEUE_EMPTY";
            break;
    }

    switch (payload.resetReason) {
        case RESET_REASON_POWER_ON:
            doc["reset_reason"] = "POWER_ON";
            break;
        case RESET_REASON_PIN_RESET:
            doc["reset_reason"] = "PIN_RESET";
            break;
        case RESET_REASON_BROWN_OUT:
            doc["reset_reason"] = "BROWN_OUT";
            break;
        case RESET_REASON_SOFTWARE:
            doc["reset_reason"] = "SOFTWARE";
            break;
        case RESET_REASON_WATCHDOG:
            doc["reset_reason"] = "WATCHDOG";
            break;
        case RESET_REASON_LOCKUP:
            doc["reset_reason"] = "LOCKUP";
            break;
        case RESET_REASON_WAKE_LOW_POWER:
            doc["reset_reason"] = "WAKE_LOW_POWER";
            break;
        case RESET_REASON_ACCESS_ERROR:
            doc["reset_reason"] = "ACCESS_ERROR";
            break;
        case RESET_REASON_BOOT_ERROR:
            doc["reset_reason"] = "BOOT_ERROR";
            break;
        case RESET_REASON_MULTIPLE:
            doc["reset_reason"] = "MULTIPLE";
            break;
        case RESET_REASON_PLATFORM:
            doc["reset_reason"] = "PLATFORM";
            break;
        case RESET_REASON_UNKNOWN:
            doc["reset_reason"] = "UNKNOWN";
            break;
    }

    size_t len = serializeJson(doc, data);
    pubSubClient.publish(topic, data, len);

    handleYield();
}

void WifiTransceiver::publish(const char *topic, SystemControllerState state, SystemControllerState &dedup) {
    if(state == dedup && !doFullSend) {
        return;
    }

    dedup = state;

    switch (state) {
        case SYSTEM_CONTROLLER_STATE_UNDETERMINED:
            pubSubClient.publish(topic, "Undetermined");
            break;
        case SYSTEM_CONTROLLER_STATE_HEATUP:
            pubSubClient.publish(topic, "Heating up");
            break;
        case SYSTEM_CONTROLLER_STATE_TEMPS_NORMALIZING:
            pubSubClient.publish(topic, "Temperatures normalizing");
            break;
        case SYSTEM_CONTROLLER_STATE_WARM:
            pubSubClient.publish(topic, "Warm");
            break;
        case SYSTEM_CONTROLLER_STATE_SLEEPING:
            pubSubClient.publish(topic, "Sleeping");
            break;
        case SYSTEM_CONTROLLER_STATE_BAILED:
            pubSubClient.publish(topic, "Error");
            break;
        case SYSTEM_CONTROLLER_STATE_FIRST_RUN:
            pubSubClient.publish(topic, "First run");
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

    snprintf(TOPIC_ONLINE, TOPIC_LENGTH - 1, "%s/%s/status", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STATE, TOPIC_LENGTH - 1, "%s/%s/stat/state", TOPIC_PREFIX, identifier);

    snprintf(TOPIC_CONF_ECO_MODE, TOPIC_LENGTH - 1,  "%s/%s/conf/eco_mode", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_SLEEP_MODE, TOPIC_LENGTH - 1,  "%s/%s/conf/sleep_mode", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_BREW_TEMP_TARGET, TOPIC_LENGTH - 1,  "%s/%s/conf/brew_temp_target", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_SERVICE_TEMP_TARGET, TOPIC_LENGTH - 1,  "%s/%s/conf/service_temp_target", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_BREW_TEMP_OFFSET, TOPIC_LENGTH - 1,  "%s/%s/conf/brew_temp_offset", TOPIC_PREFIX, identifier);

    snprintf(TOPIC_CONF_BREW_PID, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_CONF_SERVICE_PID, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid", TOPIC_PREFIX, identifier);

    snprintf(TOPIC_STAT_INTERNAL, TOPIC_LENGTH - 1, "%s/%s/stat/internal", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_WIFI, TOPIC_LENGTH - 1, "%s/%s/stat/wifi", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_WATER_TANK_EMPTY, TOPIC_LENGTH - 1,  "%s/%s/stat/water_tank_empty", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_BREW_PID, TOPIC_LENGTH - 1, "%s/%s/stat/brew_pid", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_STAT_SERVICE_PID, TOPIC_LENGTH - 1, "%s/%s/stat/service_pid", TOPIC_PREFIX, identifier);

    snprintf(TOPIC_TEMP_BREW, TOPIC_LENGTH - 1,  "%s/%s/temp/brew", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_TEMP_SERVICE, TOPIC_LENGTH - 1,  "%s/%s/temp/service", TOPIC_PREFIX, identifier);

    snprintf(TOPIC_SET_CONF_ECO_MODE, TOPIC_LENGTH - 1, "%s/%s/conf/eco_mode/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SLEEP_MODE, TOPIC_LENGTH - 1, "%s/%s/conf/sleep_mode/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_TEMP_TARGET, TOPIC_LENGTH - 1, "%s/%s/conf/brew_temp_target/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SERVICE_TEMP_TARGET, TOPIC_LENGTH - 1, "%s/%s/conf/service_temp_target/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_TEMP_OFFSET, TOPIC_LENGTH - 1, "%s/%s/conf/brew_temp_offset/set", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_PID_KP, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/set/kp", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_PID_KI, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/set/ki", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_PID_KD, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/set/kd", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_PID_WINDUP_LOW, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/set/windup_low", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_BREW_PID_WINDUP_HIGH, TOPIC_LENGTH - 1, "%s/%s/conf/brew_pid/set/windup_high", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SERVICE_PID_KP, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/set/kp", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SERVICE_PID_KI, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/set/ki", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SERVICE_PID_KD, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/set/kd", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SERVICE_PID_WINDUP_LOW, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/set/windup_low", TOPIC_PREFIX, identifier);
    snprintf(TOPIC_SET_CONF_SERVICE_PID_WINDUP_HIGH, TOPIC_LENGTH - 1, "%s/%s/conf/service_pid/set/windup_high", TOPIC_PREFIX, identifier);

    snprintf(TOPIC_AUTOCONF_STATE_SENSOR, 127, "homeassistant/sensor/%s/%s_state/config", FIRMWARE_PREFIX, identifier);
    snprintf(TOPIC_AUTOCONF_BREW_BOILER_SENSOR, 127, "homeassistant/sensor/%s/%s_brew_temp/config", FIRMWARE_PREFIX, identifier);
    snprintf(TOPIC_AUTOCONF_SERVICE_BOILER_SENSOR, 127, "homeassistant/sensor/%s/%s_serv_temp/config", FIRMWARE_PREFIX, identifier);
    snprintf(TOPIC_AUTOCONF_ECO_MODE_SWITCH, 127, "homeassistant/switch/%s/%s_eco_mode/config", FIRMWARE_PREFIX, identifier);
    snprintf(TOPIC_AUTOCONF_SLEEP_MODE_SWITCH, 127, "homeassistant/switch/%s/%s_sleep_mode/config", FIRMWARE_PREFIX, identifier);
    snprintf(TOPIC_AUTOCONF_BREW_TEMPERATURE_TARGET_NUMBER, 127, "homeassistant/number/%s/%s_brew_temp_target/config", FIRMWARE_PREFIX, identifier);
    snprintf(TOPIC_AUTOCONF_SERVICE_TEMPERATURE_TARGET_NUMBER, 127, "homeassistant/number/%s/%s_serv_temp_target/config", FIRMWARE_PREFIX, identifier);
    snprintf(TOPIC_AUTOCONF_WATER_TANK_LOW_BINARY_SENSOR, 127, "homeassistant/binary_sensor/%s/%s_water_tank_low/config", FIRMWARE_PREFIX, identifier);
    snprintf(TOPIC_AUTOCONF_WIFI_SENSOR, 127, "homeassistant/sensor/%s/%s_wifi/config", FIRMWARE_PREFIX, identifier);

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
    autoconfPayload["stat_t"] = TOPIC_STATE;
    autoconfPayload["name"] = identifier + String(" State");
    autoconfPayload["uniq_id"] = identifier + String("_state");
    autoconfPayload["ic"] = "mdi:eye";
    autoconfPayload["json_attr_t"] = TOPIC_STAT_INTERNAL;
    autoconfPayload["json_attr_tpl"] = R"({"bail_reason": "{{value_json.bail_reason}}", "reset_reason": "{{value_json.reset_reason}}"})";

    len = serializeJson(autoconfPayload, mqttPayload, 2048);
    printf("Payload: %s\n", mqttPayload);
    pubSubClient.publish(&TOPIC_AUTOCONF_STATE_SENSOR[0], mqttPayload, len);
    handleYield();
    autoconfPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_ONLINE;
    autoconfPayload["stat_t"] = TOPIC_TEMP_BREW;
    autoconfPayload["name"] = identifier + String(" Brew Boiler Temperature");
    autoconfPayload["uniq_id"] = identifier + String("_brew_temp");
    autoconfPayload["unit_of_meas"] = "°C";
    autoconfPayload["ic"] = "mdi:thermometer";
    autoconfPayload["dev_cla"] = "temperature";
    autoconfPayload["json_attr_t"] = TOPIC_STAT_BREW_PID;
    autoconfPayload["json_attr_tpl"] = R"({"p": "{{value_json.p}}", "i": "{{value_json.i}}", "d": "{{value_json.d}}", "integral": "{{value_json.integral}}",  "hysteresis_mode": "{{value_json.hysteresisMode}}"})";


    len = serializeJson(autoconfPayload, mqttPayload, 2048);
    printf("Payload: %s\n", mqttPayload);
    pubSubClient.publish(&TOPIC_AUTOCONF_BREW_BOILER_SENSOR[0], mqttPayload, len);
    handleYield();
    autoconfPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_ONLINE;
    autoconfPayload["stat_t"] = TOPIC_TEMP_SERVICE;
    autoconfPayload["name"] = identifier + String(" Service Boiler Temperature");
    autoconfPayload["uniq_id"] = identifier + String("_serv_temp");
    autoconfPayload["unit_of_meas"] = "°C";
    autoconfPayload["ic"] = "mdi:thermometer";
    autoconfPayload["dev_cla"] = "temperature";
    autoconfPayload["json_attr_t"] = TOPIC_STAT_SERVICE_PID;
    autoconfPayload["json_attr_tpl"] = R"({"p": "{{value_json.p}}", "i": "{{value_json.i}}", "d": "{{value_json.d}}", "integral": "{{value_json.integral}}",  "hysteresis_mode": "{{value_json.hysteresisMode}}"})";

    len = serializeJson(autoconfPayload, mqttPayload, 2048);
    printf("Payload: %s\n", mqttPayload);
    pubSubClient.publish(&TOPIC_AUTOCONF_SERVICE_BOILER_SENSOR[0], mqttPayload, len);
    handleYield();
    autoconfPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_ONLINE;
    autoconfPayload["stat_t"] = TOPIC_CONF_ECO_MODE;
    autoconfPayload["name"] = identifier + String(" Eco Mode");
    autoconfPayload["uniq_id"] = identifier + String("_eco_mode");
    autoconfPayload["ic"] = "hass:leaf";
    autoconfPayload["cmd_t"] = TOPIC_SET_CONF_ECO_MODE;

    len = serializeJson(autoconfPayload, mqttPayload, 2048);
    printf("Payload: %s\n", mqttPayload);
    pubSubClient.publish(&TOPIC_AUTOCONF_ECO_MODE_SWITCH[0], mqttPayload, len);
    handleYield();
    autoconfPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_ONLINE;
    autoconfPayload["stat_t"] = TOPIC_CONF_SLEEP_MODE;
    autoconfPayload["name"] = identifier + String(" Sleep Mode");
    autoconfPayload["uniq_id"] = identifier + String("_sleep_mode");
    autoconfPayload["ic"] = "hass:sleep";
    autoconfPayload["cmd_t"] = TOPIC_SET_CONF_SLEEP_MODE;

    len = serializeJson(autoconfPayload, mqttPayload, 2048);
    printf("Payload: %s\n", mqttPayload);
    pubSubClient.publish(&TOPIC_AUTOCONF_SLEEP_MODE_SWITCH[0], mqttPayload, len);
    handleYield();
    autoconfPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_ONLINE;
    autoconfPayload["stat_t"] = TOPIC_CONF_BREW_TEMP_TARGET;
    autoconfPayload["name"] = identifier + String(" Brew Boiler Temperature Target");
    autoconfPayload["uniq_id"] = identifier + String("_brew_temp_target");
    autoconfPayload["unit_of_meas"] = "°C";
    autoconfPayload["ic"] = "mdi:thermometer";
    autoconfPayload["cmd_t"] = TOPIC_SET_CONF_BREW_TEMP_TARGET;
    autoconfPayload["step"] = 0.01;

    len = serializeJson(autoconfPayload, mqttPayload, 2048);
    printf("Payload: %s\n", mqttPayload);
    pubSubClient.publish(&TOPIC_AUTOCONF_BREW_TEMPERATURE_TARGET_NUMBER[0], mqttPayload, len);
    handleYield();
    autoconfPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_ONLINE;
    autoconfPayload["stat_t"] = TOPIC_CONF_SERVICE_TEMP_TARGET;
    autoconfPayload["name"] = identifier + String(" Service Boiler Temperature Target");
    autoconfPayload["uniq_id"] = identifier + String("_serv_temp_target");
    autoconfPayload["unit_of_meas"] = "°C";
    autoconfPayload["ic"] = "mdi:thermometer";
    autoconfPayload["cmd_t"] = TOPIC_SET_CONF_SERVICE_TEMP_TARGET;
    autoconfPayload["max"] = 150;
    autoconfPayload["step"] = 0.01;

    len = serializeJson(autoconfPayload, mqttPayload, 2048);
    printf("Payload: %s\n", mqttPayload);
    pubSubClient.publish(&TOPIC_AUTOCONF_SERVICE_TEMPERATURE_TARGET_NUMBER[0], mqttPayload, len);
    handleYield();
    autoconfPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_ONLINE;
    autoconfPayload["stat_t"] = TOPIC_STAT_WATER_TANK_EMPTY;
    autoconfPayload["name"] = identifier + String(" Water Tank Low");
    autoconfPayload["uniq_id"] = identifier + String("_water_tank_low");
    autoconfPayload["ic"] = "mdi:water-alert";
    autoconfPayload["pl_on"] = "true";
    autoconfPayload["pl_off"] = "false";
    autoconfPayload["dev_cla"] = "problem";


    len = serializeJson(autoconfPayload, mqttPayload, 2048);
    pubSubClient.publish(&TOPIC_AUTOCONF_WATER_TANK_LOW_BINARY_SENSOR[0], mqttPayload, len);
    handleYield();
    autoconfPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_ONLINE;
    autoconfPayload["stat_t"] = TOPIC_STAT_WIFI;
    autoconfPayload["name"] = identifier + String(" WiFi");
    autoconfPayload["uniq_id"] = identifier + String("_wifi");
    autoconfPayload["val_tpl"] = "{{value_json.rssi}}";
    autoconfPayload["unit_of_meas"] = "dBm";
    autoconfPayload["json_attr_t"] = TOPIC_STAT_WIFI;
    autoconfPayload["json_attr_tpl"] = R"({"ssid": "{{value_json.ssid}}", "ip": "{{value_json.ip}}"})";
    autoconfPayload["ic"] = "mdi:wifi";

    serializeJson(autoconfPayload, mqttPayload);
    pubSubClient.publish(&TOPIC_AUTOCONF_WIFI_SENSOR[0], mqttPayload, len);
    handleYield();

    printf("Done\n");
}

void WifiTransceiver::sendWifiStats() {
    char data[127];
    StaticJsonDocument<96> doc;

    auto ip = WiFi.localIP();
    String ipString = String(ip[0]) + "." + ip[1] + "." + ip[2] + "." + ip[3];

    doc["ssid"] = WiFi.SSID();
    doc["ip"] = ipString;
    doc["rssi"] = WiFi.RSSI();

    size_t len = serializeJson(doc, data);

    pubSubClient.publish(&TOPIC_STAT_WIFI[0], data, len);
    handleYield();
}
