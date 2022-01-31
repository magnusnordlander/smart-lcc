//
// Created by Magnus Nordlander on 2022-01-30.
//

#ifndef FIRMWARE_ARDUINO_NETWORKCONTROLLER_H
#define FIRMWARE_ARDUINO_NETWORKCONTROLLER_H

#define DEBUG_WIFI_WEBSERVER_PORT       Serial
#define WIFININA_DEBUG_OUTPUT           Serial
#define _WIFININA_LOGLEVEL_             4
#define DRD_GENERIC_DEBUG               true
#define USING_CUSTOMS_STYLE           true
#define USING_CUSTOMS_HEAD_ELEMENT    true
#define USING_CORS_FEATURE            true
#define USE_WIFI_NINA                 true
#define RESET_IF_CONFIG_TIMEOUT                   true
#define RETRY_TIMES_RECONNECT_WIFI                3
#define CONFIG_TIMEOUT_RETRYTIMES_BEFORE_RESET    5
#define CONFIG_TIMEOUT                      120000L
#define REQUIRE_ONE_SET_SSID_PW             true
#define USE_DYNAMIC_PARAMETERS              true
#define SCAN_WIFI_NETWORKS                  true
#define MANUAL_SSID_INPUT_ALLOWED           true
#define MAX_SSID_IN_LIST                    8

#include <WiFiNINA_Pinout_Generic.h>
#include <WiFiManager_NINA_Lite_RP2040.h>
#include <WiFi_Generic.h>
#include <ArduinoOTA.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>

#define AIO_SERVER_LEN       20
#define AIO_SERVERPORT_LEN   6
#define AIO_USERNAME_LEN     20
#define AIO_KEY_LEN          40

char AIO_SERVER     [AIO_SERVER_LEN + 1]        = "io.adafruit.com";
char AIO_SERVERPORT [AIO_SERVERPORT_LEN + 1]    = "1883";     //1883, or 8883 for SSL
char AIO_USERNAME   [AIO_USERNAME_LEN + 1]      = "private";
char AIO_KEY        [AIO_KEY_LEN + 1]           = "private";

MenuItem myMenuItems [] =
        {
                { "svr", "AIO_SERVER",      AIO_SERVER,     AIO_SERVER_LEN },
                { "prt", "AIO_SERVERPORT",  AIO_SERVERPORT, AIO_SERVERPORT_LEN },
                { "usr", "AIO_USERNAME",    AIO_USERNAME,   AIO_USERNAME_LEN },
                { "key", "AIO_KEY",         AIO_KEY,        AIO_KEY_LEN },
        };

uint16_t NUM_MENU_ITEMS = sizeof(myMenuItems) / sizeof(MenuItem);  //MenuItemSize;

bool LOAD_DEFAULT_CONFIG_DATA = false;
WiFiNINA_Configuration defaultConfig;

typedef enum {
    NETWORK_CONTROLLER_MODE_NORMAL,
    NETWORK_CONTROLLER_MODE_FORCE_CONFIG,
    NETWORK_CONTROLLER_MODE_OTA
} NetworkControllerMode;

class NetworkController {
public:
    explicit NetworkController(NetworkControllerMode mode);

    void loop();

private:
    NetworkControllerMode mode;

    ArduinoOTAMdnsClass <WiFiServer, WiFiClient, WiFiUDP> ArduinoOTA;

    WiFiClient *client{};

    Adafruit_MQTT_Client *mqtt{};

    WiFiManager_NINA_Lite* wifiManager;

    bool otaInited = false;

    void initOTA();
};


#endif //FIRMWARE_ARDUINO_NETWORKCONTROLLER_H
