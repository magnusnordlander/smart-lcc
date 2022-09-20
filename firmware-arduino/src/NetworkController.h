//
// Created by Magnus Nordlander on 2022-01-30.
//

#ifndef FIRMWARE_ARDUINO_NETWORKCONTROLLER_H
#define FIRMWARE_ARDUINO_NETWORKCONTROLLER_H

#include <WiFiNINA_Pinout_Generic.h>
#include <WiFi_Generic.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <FS.h>
#include <LittleFS.h>
#include <string>
#include "optional.hpp"
#include "types.h"
#include "SystemStatus.h"

// Because these libraries don't use .cpp files, we have to forward declare the class instead to linking errors.
class WiFiWebServer;

#define TOPIC_LENGTH 49

class NetworkController {
public:
    explicit NetworkController(FileIO* _fileIO, SystemStatus* _status, SystemSettings* _settings);

    void init(SystemMode mode);

    bool hasConfiguration();
    bool isConnectedToWifi() const;
    nonstd::optional<IPAddress> getIPAddress();
    bool isConnectedToMqtt();
    SystemMode getMode() {
        return mode;
    }

    void loop();
private:
    SystemMode mode;
    FileIO* fileIO;
    SystemStatus* status;
    SystemSettings* settings;

    uint8_t previousWifiStatus = 0;

    nonstd::optional<WiFiNINA_Configuration> config;
    nonstd::optional<absolute_time_t> wifiConnectTimeoutTime;
    nonstd::optional<absolute_time_t> mqttConnectTimeoutTime;
    nonstd::optional<absolute_time_t> mqttNextStatPublishTime;
    nonstd::optional<absolute_time_t> mqttNextInfoPublishTime;
    nonstd::optional<absolute_time_t> mqttNextConfigPublishTime;

    bool configChanged = true;

    ArduinoOTAMdnsClass <WiFiServer, WiFiClient, WiFiUDP> ArduinoOTA;
    WiFiClient client = WiFiClient();
    PubSubClient mqtt = PubSubClient(client);

    WiFiWebServer* server = nullptr;

    bool otaInited = false;
    bool _isConnectedToWifi = false;
    bool topicsFormatted = false;

    char identifier[24];
    std::string stdIdentifier;

    void initConfigMode();
    void initOTA();
    void loopNormal();
    void loopConfig();
    void loopOta();
    void attemptWifiConnection();
    void attemptReadConfig();
    void writeConfig(WiFiNINA_Configuration newConfig);

    void resetModule();

    bool ensureConnectedMqttClient();
    void ensureTopicsFormatted();

    void publishMqtt();
    void publishMqttStat();
    void publishMqttConf();
    void publishMqttInfo();

    void handleConfigHTTPRequest();
    void sendHTTPHeaders();
    void createHTML(String& root_html_template);

    char TOPIC_LWT[TOPIC_LENGTH];
    char TOPIC_STATE[TOPIC_LENGTH];
    char TOPIC_CONFIG[TOPIC_LENGTH];
    char TOPIC_INFO[TOPIC_LENGTH];
    char TOPIC_COMMAND[TOPIC_LENGTH];

    char TOPIC_AUTOCONF_STATE_SENSOR[128];
    char TOPIC_AUTOCONF_BREW_BOILER_SENSOR[128];
    char TOPIC_AUTOCONF_SERVICE_BOILER_SENSOR[128];
    char TOPIC_AUTOCONF_ECO_MODE_SWITCH[128];
    char TOPIC_AUTOCONF_SLEEP_MODE_SWITCH[128];
    char TOPIC_AUTOCONF_BREW_TEMPERATURE_TARGET_NUMBER[128];
    char TOPIC_AUTOCONF_SERVICE_TEMPERATURE_TARGET_NUMBER[128];
    char TOPIC_AUTOCONF_WATER_TANK_LOW_BINARY_SENSOR[128];
    char TOPIC_AUTOCONF_WIFI_SENSOR[128];
    char TOPIC_AUTOCONF_AUTO_SLEEP_MIN[128];
    char TOPIC_AUTOCONF_PLANNED_AUTO_SLEEP_MIN[128];

    void callback(char *topic, byte *payload, unsigned int length);

    void publishAutoconfigure();
};


#endif //FIRMWARE_ARDUINO_NETWORKCONTROLLER_H
