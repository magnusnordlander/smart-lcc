//
// Created by Magnus Nordlander on 2021-07-16.
//

#ifndef FIRMWARE_WIFITRANSCEIVER_H
#define FIRMWARE_WIFITRANSCEIVER_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <SystemStatus.h>
#include <SystemSettings.h>
#include <ArduinoJson.h>

#define TOPIC_LENGTH 49

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
    bool topicsInitialized = false;

    char identifier[24];

    char TOPIC_ONLINE[TOPIC_LENGTH];

    char TOPIC_STATE[TOPIC_LENGTH];

    char TOPIC_STAT_INTERNAL[TOPIC_LENGTH];
    char TOPIC_STAT_WIFI[TOPIC_LENGTH];

    char TOPIC_CONF_ECO_MODE[TOPIC_LENGTH];
    char TOPIC_CONF_SLEEP_MODE[TOPIC_LENGTH];
    char TOPIC_CONF_BREW_TEMP_TARGET[TOPIC_LENGTH];
    char TOPIC_CONF_SERVICE_TEMP_TARGET[TOPIC_LENGTH];
    char TOPIC_CONF_BREW_TEMP_OFFSET[TOPIC_LENGTH];

    char TOPIC_CONF_BREW_PID[TOPIC_LENGTH];
    char TOPIC_CONF_SERVICE_PID[TOPIC_LENGTH];

    char TOPIC_STAT_WATER_TANK_EMPTY[TOPIC_LENGTH];

    char TOPIC_STAT_BREW_PID[TOPIC_LENGTH];
    char TOPIC_STAT_SERVICE_PID[TOPIC_LENGTH];

    char TOPIC_TEMP_BREW[TOPIC_LENGTH];
    char TOPIC_TEMP_SERVICE[TOPIC_LENGTH];

    char TOPIC_SET_CONF_ECO_MODE[TOPIC_LENGTH];
    char TOPIC_SET_CONF_SLEEP_MODE[TOPIC_LENGTH];
    char TOPIC_SET_CONF_BREW_TEMP_TARGET[TOPIC_LENGTH];
    char TOPIC_SET_CONF_SERVICE_TEMP_TARGET[TOPIC_LENGTH];
    char TOPIC_SET_CONF_BREW_TEMP_OFFSET[TOPIC_LENGTH];
    char TOPIC_SET_CONF_BREW_PID_KP[TOPIC_LENGTH];
    char TOPIC_SET_CONF_BREW_PID_KI[TOPIC_LENGTH];
    char TOPIC_SET_CONF_BREW_PID_KD[TOPIC_LENGTH];
    char TOPIC_SET_CONF_BREW_PID_WINDUP_LOW[TOPIC_LENGTH];
    char TOPIC_SET_CONF_BREW_PID_WINDUP_HIGH[TOPIC_LENGTH];
    char TOPIC_SET_CONF_SERVICE_PID_KP[TOPIC_LENGTH];
    char TOPIC_SET_CONF_SERVICE_PID_KI[TOPIC_LENGTH];
    char TOPIC_SET_CONF_SERVICE_PID_KD[TOPIC_LENGTH];
    char TOPIC_SET_CONF_SERVICE_PID_WINDUP_LOW[TOPIC_LENGTH];
    char TOPIC_SET_CONF_SERVICE_PID_WINDUP_HIGH[TOPIC_LENGTH];

    char TOPIC_AUTOCONF_BREW_BOILER_SENSOR[128];
    char TOPIC_AUTOCONF_SERVICE_BOILER_SENSOR[128];
    char TOPIC_AUTOCONF_ECO_MODE_SWITCH[128];
    char TOPIC_AUTOCONF_SLEEP_MODE_SWITCH[128];
    char TOPIC_AUTOCONF_BREW_TEMPERATURE_TARGET_NUMBER[128];
    char TOPIC_AUTOCONF_SERVICE_TEMPERATURE_TARGET_NUMBER[128];

    std::chrono::time_point<rtos::Kernel::Clock> lastFullSend;
    bool doFullSend = false;

    InternalStateMessage DEDUP_STAT_INTERNAL;
    SystemControllerState DEDUP_STATE;

    bool DEDUP_CONF_ECO_MODE;
    bool DEDUP_CONF_SLEEP_MODE;
    float DEDUP_CONF_BREW_TEMP_TARGET;
    float DEDUP_CONF_SERVICE_TEMP_TARGET;
    float DEDUP_CONF_BREW_TEMP_OFFSET;

    PidSettings DEDUP_CONF_BREW_PID;
    PidSettings DEDUP_CONF_SERVICE_PID;

    bool DEDUP_STAT_WATER_TANK_EMPTY;

    PidRuntimeParameters DEDUP_STAT_BREW_PID;
    PidRuntimeParameters DEDUP_STAT_SERVICE_PID;

    float DEDUP_TEMP_BREW;
    float DEDUP_TEMP_SERVICE;

    void ensureConnectedToWifi();
    void ensureConnectedToMqtt();
    void publishStatus();

    void formatTopics();
    void publishAutoconfig();
    void sendWifiStats();

    void callback(char* topic, byte* payload, unsigned int length);

    void publish(const char* topic, bool payload, bool &dedup, bool onOff = false);
    void publish(const char* topic, float payload, float &dedup);
    void publish(const char* topic, uint8_t payload, uint8_t &dedup);
    void publish(const char *topic, double payload, double &dedup);
    void publish(const char *topic, PidRuntimeParameters payload, PidRuntimeParameters &dedup);
    void publish(const char *topic, PidSettings payload, PidSettings &dedup);
    void publish(const char *topic, InternalStateMessage payload, InternalStateMessage &dedup);
    void publish(const char *topic, SystemControllerState payload, SystemControllerState &dedup);

    void handleYield();
};


#endif //FIRMWARE_WIFITRANSCEIVER_H
