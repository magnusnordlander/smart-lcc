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
    char TOPIC_BAILED[TOPIC_LENGTH];
    char TOPIC_BAIL_REASON[TOPIC_LENGTH];
    char TOPIC_RESET_REASON[TOPIC_LENGTH];
    char TOPIC_STAT_TX[TOPIC_LENGTH];
    char TOPIC_CONF_ECO_MODE[TOPIC_LENGTH];
    char TOPIC_CONF_SLEEP_MODE[TOPIC_LENGTH];
    char TOPIC_CONF_BREW_TEMP_TARGET[TOPIC_LENGTH];
    char TOPIC_CONF_SERVICE_TEMP_TARGET[TOPIC_LENGTH];
    char TOPIC_CONF_BREW_TEMP_OFFSET[TOPIC_LENGTH];
    char TOPIC_CONF_BREW_PID_KP[TOPIC_LENGTH];
    char TOPIC_CONF_BREW_PID_KI[TOPIC_LENGTH];
    char TOPIC_CONF_BREW_PID_KD[TOPIC_LENGTH];
    char TOPIC_CONF_BREW_PID_WINDUP_LOW[TOPIC_LENGTH];
    char TOPIC_CONF_BREW_PID_WINDUP_HIGH[TOPIC_LENGTH];
    char TOPIC_CONF_SERVICE_PID_KP[TOPIC_LENGTH];
    char TOPIC_CONF_SERVICE_PID_KI[TOPIC_LENGTH];
    char TOPIC_CONF_SERVICE_PID_KD[TOPIC_LENGTH];
    char TOPIC_CONF_SERVICE_PID_WINDUP_LOW[TOPIC_LENGTH];
    char TOPIC_CONF_SERVICE_PID_WINDUP_HIGH[TOPIC_LENGTH];
    char TOPIC_STAT_RX[TOPIC_LENGTH];
    char TOPIC_STAT_WATER_TANK_EMPTY[TOPIC_LENGTH];
    char TOPIC_STAT_BREW_PID_P[TOPIC_LENGTH];
    char TOPIC_STAT_BREW_PID_I[TOPIC_LENGTH];
    char TOPIC_STAT_BREW_PID_D[TOPIC_LENGTH];
    char TOPIC_STAT_BREW_PID_INTEGRAL[TOPIC_LENGTH];
    char TOPIC_STAT_BREW_PID_HYSTERESIS[TOPIC_LENGTH];
    char TOPIC_STAT_SERVICE_PID_P[TOPIC_LENGTH];
    char TOPIC_STAT_SERVICE_PID_I[TOPIC_LENGTH];
    char TOPIC_STAT_SERVICE_PID_D[TOPIC_LENGTH];
    char TOPIC_STAT_SERVICE_PID_INTEGRAL[TOPIC_LENGTH];
    char TOPIC_STAT_SERVICE_PID_HYSTERESIS[TOPIC_LENGTH];
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

    std::chrono::time_point<rtos::Kernel::Clock> lastFullSend;
    bool doFullSend = false;

    bool DEDUP_ONLINE;
    SystemControllerState DEDUP_STATE;
    bool DEDUP_BAILED;
    SystemControllerBailReason DEDUP_BAIL_REASON;
    reset_reason_t DEDUP_RESET_REASON;
    bool DEDUP_STAT_TX;
    bool DEDUP_CONF_ECO_MODE;
    bool DEDUP_CONF_SLEEP_MODE;
    float DEDUP_CONF_BREW_TEMP_TARGET;
    float DEDUP_CONF_SERVICE_TEMP_TARGET;
    float DEDUP_CONF_BREW_TEMP_OFFSET;
    float DEDUP_CONF_BREW_PID_KP;
    float DEDUP_CONF_BREW_PID_KI;
    float DEDUP_CONF_BREW_PID_KD;
    float DEDUP_CONF_BREW_PID_WINDUP_LOW;
    float DEDUP_CONF_BREW_PID_WINDUP_HIGH;
    float DEDUP_CONF_SERVICE_PID_KP;
    float DEDUP_CONF_SERVICE_PID_KI;
    float DEDUP_CONF_SERVICE_PID_KD;
    float DEDUP_CONF_SERVICE_PID_WINDUP_LOW;
    float DEDUP_CONF_SERVICE_PID_WINDUP_HIGH;
    bool DEDUP_STAT_RX;
    bool DEDUP_STAT_WATER_TANK_EMPTY;
    float DEDUP_STAT_BREW_PID_P;
    float DEDUP_STAT_BREW_PID_I;
    float DEDUP_STAT_BREW_PID_D;
    float DEDUP_STAT_BREW_PID_INTEGRAL;
    bool DEDUP_STAT_BREW_PID_HYSTERESIS;
    float DEDUP_STAT_SERVICE_PID_P;
    float DEDUP_STAT_SERVICE_PID_I;
    float DEDUP_STAT_SERVICE_PID_D;
    float DEDUP_STAT_SERVICE_PID_INTEGRAL;
    bool DEDUP_STAT_SERVICE_PID_HYSTERESIS;
    float DEDUP_TEMP_BREW;
    float DEDUP_TEMP_SERVICE;

    void ensureConnectedToWifi();
    void ensureConnectedToMqtt();
    void publishStatus();

    void formatTopics();
    void publishAutoconfig();

    void callback(char* topic, byte* payload, unsigned int length);

    void publish(const char* topic, bool payload, bool &dedup);
    void publish(const char* topic, float payload, float &dedup);
    void publish(const char* topic, uint8_t payload, uint8_t &dedup);
    void publish(const char *topic, double payload, double &dedup);
    void publish(const char *topic, SystemControllerBailReason payload, SystemControllerBailReason &dedup);
    void publish(const char *topic, SystemControllerState payload, SystemControllerState &dedup);
    void publish(const char *topic, reset_reason_t payload, reset_reason_t &dedup);

    void handleYield();
};


#endif //FIRMWARE_WIFITRANSCEIVER_H
