//
// Created by Magnus Nordlander on 2022-01-30.
//

#include "NetworkController.h"
#include <CRC32.h>
#include <WiFiWebServer.h>
#include <ArduinoJson.h>
#include <hardware/watchdog.h>
#include <functional>

#define CONFIG_FILENAME ("/fs/network-config.dat")
#define CONFIG_VERSION ((uint8_t)1)

// -- HTML page fragments

const char WIFININA_HTML_HEAD_START[] /*PROGMEM*/ = "<!DOCTYPE html><html><head><title>RP2040_WM_NINA_Lite</title>";

const char WIFININA_HTML_HEAD_STYLE[] /*PROGMEM*/ = "<style>div,input,select{padding:5px;font-size:1em;}input,select{width:95%;}body{text-align:center;}button{background-color:#16A1E7;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;}fieldset{border-radius:0.3rem;margin:0px;}</style>";

const char WIFININA_HTML_HEAD_END[]   /*PROGMEM*/ = "</head><div style='text-align:left;display:inline-block;min-width:260px;'>\
<fieldset><div><label>*WiFi SSID</label><div>[[input_id]]</div></div>\
<div><label>*PWD (8+ chars)</label><input value='[[pw]]' id='pw'><div></div></div></fieldset>\
<fieldset><div><label>MQTT Server</label><input value='[[ms]]' id='ms'><div></div></div>\
<div><label>MQTT Port</label><input value='[[mo]]' id='mo'><div></div></div>\
<div><label>MQTT Username</label><input value='[[mu]]' id='mu'><div></div></div>\
<div><label>MQTT Password</label><input value='[[mp]]' id='mp'><div></div></div>\
<div><label>MQTT Prefix</label><input value='[[mr]]' id='mr'><div></div></div>\
</fieldset>";

const char WIFININA_HTML_INPUT_ID[]   /*PROGMEM*/ = "<input value='[[id]]' id='id'>";

const char WIFININA_FLDSET_START[]  /*PROGMEM*/ = "<fieldset>";
const char WIFININA_FLDSET_END[]    /*PROGMEM*/ = "</fieldset>";
const char WIFININA_HTML_BUTTON[]   /*PROGMEM*/ = "<button onclick=\"sv()\">Save</button></div>";
const char WIFININA_HTML_SCRIPT[]   /*PROGMEM*/ = "<script id=\"jsbin-javascript\">\
function v(id){return document.getElementById(id).value;}\
function sv(){\
var request=new XMLHttpRequest();var url='/?data='+encodeURIComponent(JSON.stringify({\
id: v('id'), pw: v('pw'), ms: v('ms'), mo: v('mo'), mu: v('mu'), mp: v('mp'), mr: v('mr')\
}));\
request.open('GET',url,false);\
request.send(null);";

const char WIFININA_HTML_SCRIPT_END[]   /*PROGMEM*/ = "alert('Updated');}</script>";
const char WIFININA_HTML_END[]          /*PROGMEM*/ = "</html>";

const char WIFININA_SELECT_START[]      /*PROGMEM*/ = "<select id=";
const char WIFININA_SELECT_END[]        /*PROGMEM*/ = "</select>";
const char WIFININA_DATALIST_START[]    /*PROGMEM*/ = "<datalist id=";
const char WIFININA_DATALIST_END[]      /*PROGMEM*/ = "</datalist>";
const char WIFININA_OPTION_START[]      /*PROGMEM*/ = "<option>";
const char WIFININA_OPTION_END[]        /*PROGMEM*/ = "";			// "</option>"; is not required
const char WIFININA_NO_NETWORKS_FOUND[] /*PROGMEM*/ = "No suitable WiFi networks available!";

const char WM_HTTP_CACHE_CONTROL[]   PROGMEM = "Cache-Control";
const char WM_HTTP_NO_STORE[]        PROGMEM = "no-cache, no-store, must-revalidate";
const char WM_HTTP_PRAGMA[]          PROGMEM = "Pragma";
const char WM_HTTP_NO_CACHE[]        PROGMEM = "no-cache";
const char WM_HTTP_EXPIRES[]         PROGMEM = "Expires";

NetworkController::NetworkController(FileIO* _fileIO, SystemStatus* _status, SystemSettings* _settings):
    fileIO(_fileIO), status(_status), settings(_settings) {
}

void NetworkController::init(SystemMode _mode) {
    mode = _mode;
    attemptReadConfig();

    uint8_t mac[WL_MAC_ADDR_LENGTH];
    WiFi.macAddress(mac);

    snprintf(identifier, sizeof(identifier), "LCC-%02X%02X%02X", mac[2], mac[1], mac[0]);
    stdIdentifier = std::string(identifier);

    WiFi.setHostname(identifier);

    switch (mode) {
        case SYSTEM_MODE_NORMAL:
        case SYSTEM_MODE_OTA:
            break;
        case SYSTEM_MODE_CONFIG:
            initConfigMode();
            break;
    }
}

void NetworkController::loop() {
    int wifiStatus = WiFi.status();
    if (wifiStatus != previousWifiStatus) {
        DEBUGV("Wifi status transition. From %d to %d\n", previousWifiStatus, wifiStatus);
        previousWifiStatus = wifiStatus;
    }

    switch (mode) {
        case SYSTEM_MODE_NORMAL:
            return loopNormal();
        case SYSTEM_MODE_CONFIG:
            return loopConfig();
        case SYSTEM_MODE_OTA:
            return loopOta();
    }

    exit(255);
}

inline void NetworkController::initOTA() {
    if (!otaInited) {
        DEBUGV("Initializing OTA\n");
        ArduinoOTA.begin(WiFi.localIP(), "Arduino", "password", InternalStorage);
        otaInited = true;
    }
}

void NetworkController::loopNormal() {
    if (hasConfiguration()) {
        switch (WiFi.status()) {
            case WL_CONNECTED:
                _isConnectedToWifi = true;
                break;
            case WL_IDLE_STATUS:
            case WL_NO_SSID_AVAIL:
            case WL_SCAN_COMPLETED:
            case WL_CONNECT_FAILED:
            case WL_CONNECTION_LOST:
            case WL_DISCONNECTED:
                _isConnectedToWifi = false;
                return attemptWifiConnection();
            case WL_AP_LISTENING:
            case WL_AP_CONNECTED:
            case WL_AP_FAILED:
            case WL_NO_MODULE:
                _isConnectedToWifi = false;
                DEBUGV("Unexpected Wifi mode\n");
                return;
        }

        if (_isConnectedToWifi) {
            ensureMqttClient();
            mqtt.loop();

            if (mqtt.connected()) {
                if (!mqttNextPublishTime.has_value() || absolute_time_diff_us(mqttNextPublishTime.value(), get_absolute_time()) > 0) {
                    publishMqtt();
                    mqttNextPublishTime = make_timeout_time_ms(1000);
                }
            }
        }
    }
}

void NetworkController::ensureMqttClient() {
    if (!mqtt.connected()) {
        if (!mqttConnectTimeoutTime.has_value()) {
            ensureTopicsFormatted();
            DEBUGV("Attempting to connect to MQTT\n");

            mqtt.setServer(config.value().mqttConfig.server, atoi(config.value().mqttConfig.port));

            std::function<void(char*, uint8_t*, unsigned int)> func = [&] (char* topic, byte* payload, unsigned int length) {
                callback(topic, payload, length);
            };
            mqtt.setCallback(func);

            bool success = false;

            if (strlen(config.value().mqttConfig.username) > 0) {
                DEBUGV("Connecting using username and password\n");
                success = mqtt.connect(identifier, config.value().mqttConfig.username, config.value().mqttConfig.password, &TOPIC_LWT[0], 0, true, "offline");
            } else {
                DEBUGV("Connecting unauthenticated\n");
                success = mqtt.connect(identifier, &TOPIC_LWT[0], 0, true, "offline");
            }

            if (!success) {
                DEBUGV("Couldn't connect to MQTT server. Reconnecting in 5 s.\n");
                mqtt.disconnect();
                mqttConnectTimeoutTime = make_timeout_time_ms(5000);
                return;
            }
            DEBUGV("MQTT connection successful, changing buffer size.\n");
            mqtt.setBufferSize(4096);
            DEBUGV("Buffer size changed\n");

            mqtt.subscribe(&TOPIC_COMMAND[0]);

            publishAutoconfigure();
        } else {
            if (absolute_time_diff_us(mqttConnectTimeoutTime.value(), get_absolute_time()) > 0) {
                mqttConnectTimeoutTime.reset();
            }
        }
    }
}


void NetworkController::loopConfig() {
    if (server) {
        server->handleClient();
    }
}

void NetworkController::loopOta() {
    if (hasConfiguration()) {
        switch (WiFi.status()) {
            case WL_CONNECTED:
                _isConnectedToWifi = true;
                break;
            case WL_IDLE_STATUS:
            case WL_NO_SSID_AVAIL:
            case WL_SCAN_COMPLETED:
            case WL_CONNECT_FAILED:
            case WL_CONNECTION_LOST:
            case WL_DISCONNECTED:
                _isConnectedToWifi = false;
                return attemptWifiConnection();
            case WL_AP_LISTENING:
            case WL_AP_CONNECTED:
            case WL_AP_FAILED:
            case WL_NO_MODULE:
                _isConnectedToWifi = false;
                DEBUGV("Unexpected Wifi mode\n");
                return;
        }

        initOTA();
        ArduinoOTA.poll();
    }
}

void NetworkController::attemptWifiConnection() {
    if (hasConfiguration()) {
        if (!wifiConnectTimeoutTime.has_value()) {
            DEBUGV("Attempting to connect to Wifi\n");
            WiFiDrv::wifiSetPassphrase(config.value().wiFiCredentials.wifi_ssid, strlen(config.value().wiFiCredentials.wifi_ssid), config.value().wiFiCredentials.wifi_pw, strlen(config.value().wiFiCredentials.wifi_pw));
            wifiConnectTimeoutTime = make_timeout_time_ms(5000);
        } else {
            if (absolute_time_diff_us(wifiConnectTimeoutTime.value(), get_absolute_time()) > 0) {
                DEBUGV("Wifi connection timed out\n");
                wifiConnectTimeoutTime.reset();
            }
        }
    }
}

bool NetworkController::hasConfiguration() {
    return config.has_value();
}

bool NetworkController::isConnectedToWifi() const {
    return _isConnectedToWifi;
}

bool NetworkController::isConnectedToMqtt() {
    return mqtt.connected();
}

void NetworkController::attemptReadConfig() {
    nonstd::optional<WiFiNINA_Configuration> readConfig = fileIO->readWifiConfig(CONFIG_FILENAME, CONFIG_VERSION);

    if (readConfig.has_value()) {
        config = readConfig.value();
    } else {
        config.reset();
    }
}

void NetworkController::writeConfig(WiFiNINA_Configuration newConfig) {
    if (fileIO->saveWifiConfig(newConfig, CONFIG_FILENAME, CONFIG_VERSION)) {
        config = newConfig;
    }
}


void NetworkController::initConfigMode() {
    // Scan Wi-fi networks
    // scanNetworks();

    DEBUGV("Starting AP\n");

    WiFi.config(IPAddress(192, 168, 4, 1));
    WiFi.beginAP(identifier, identifier, 10);

    if (!server)
    {
        server = new WiFiWebServer;
    }

    server->on("/", [this](){ this->handleConfigHTTPRequest(); });
    server->begin();
}

void NetworkController::handleConfigHTTPRequest() {
    if (server)
    {
        String data = server->arg("data");

        if (data == "")
        {
            sendHTTPHeaders();

            String result;
            createHTML(result);

            if (hasConfiguration()) {
                result.replace("[[id]]", config.value().wiFiCredentials.wifi_ssid);
                result.replace("[[pw]]", config.value().wiFiCredentials.wifi_pw);
                result.replace("[[ms]]", config.value().mqttConfig.server);
                result.replace("[[mo]]", config.value().mqttConfig.port);
                result.replace("[[mu]]", config.value().mqttConfig.username);
                result.replace("[[mp]]", config.value().mqttConfig.password);
                result.replace("[[mr]]", config.value().mqttConfig.prefix);
            } else {
                result.replace("[[id]]",  "");
                result.replace("[[pw]]",  "");
                result.replace("[[ms]]", "");
                result.replace("[[mo]]", "1883");
                result.replace("[[mu]]", "");
                result.replace("[[mp]]", "");
                result.replace("[[mr]]", "lcc");
            }

            server->send(200, "text/html", result);

            return;
        } else {
            DEBUGV("Got new config to save\n");
            StaticJsonDocument<384> doc;

            DeserializationError error = deserializeJson(doc, data);

            if (error) {
                DEBUGV("Deserialize JSON failed\n");
                return;
            }

            const char* id = doc["id"];
            const char* pw = doc["pw"];
            const char* ms = doc["ms"];
            const char* mo = doc["mo"];
            const char* mu = doc["mu"];
            const char* mp = doc["mp"];
            const char* mr = doc["mr"];

            WiFiNINA_Configuration newConfig;

            strncpy(newConfig.wiFiCredentials.wifi_ssid, id, sizeof(newConfig.wiFiCredentials.wifi_ssid));
            strncpy(newConfig.wiFiCredentials.wifi_pw, pw, sizeof(newConfig.wiFiCredentials.wifi_pw));
            strncpy(newConfig.mqttConfig.server, ms, sizeof(newConfig.mqttConfig.server));
            strncpy(newConfig.mqttConfig.port, mo, sizeof(newConfig.mqttConfig.port));
            strncpy(newConfig.mqttConfig.username, mu, sizeof(newConfig.mqttConfig.username));
            strncpy(newConfig.mqttConfig.password, mp, sizeof(newConfig.mqttConfig.password));
            strncpy(newConfig.mqttConfig.prefix, mr, sizeof(newConfig.mqttConfig.prefix));

            DEBUGV("Network config processed, saving\n");
            writeConfig(newConfig);
            DEBUGV("Network config saved\n");
        }

        server->send(200, "text/html", "OK");
    }
}

void NetworkController::sendHTTPHeaders() {
    server->sendHeader(WM_HTTP_CACHE_CONTROL, WM_HTTP_NO_STORE);
    server->sendHeader(WM_HTTP_PRAGMA, WM_HTTP_NO_CACHE);
    server->sendHeader(WM_HTTP_EXPIRES, "-1");
}

void NetworkController::createHTML(String &root_html_template) {
    String pitem;

    root_html_template  = WIFININA_HTML_HEAD_START;
    root_html_template  += WIFININA_HTML_HEAD_STYLE;

#if SCAN_WIFI_NETWORKS
    WN_LOGDEBUG1(WiFiNetworksFound, F(" SSIDs found, generating HTML now"));
      // Replace HTML <input...> with <select...>, based on WiFi network scan in startConfigurationMode()

      ListOfSSIDs = "";

      for (int i = 0, list_items = 0; (i < WiFiNetworksFound) && (list_items < MAX_SSID_IN_LIST); i++)
      {
        if (indices[i] == -1)
          continue; 		// skip duplicates and those that are below the required quality

        ListOfSSIDs += WIFININA_OPTION_START + String(WiFi.SSID(indices[i])) + WIFININA_OPTION_END;
        list_items++;		// Count number of suitable, distinct SSIDs to be included in list
      }

      WN_LOGDEBUG(ListOfSSIDs);

      if (ListOfSSIDs == "")		// No SSID found or none was good enough
        ListOfSSIDs = WIFININA_OPTION_START + String(WIFININA_NO_NETWORKS_FOUND) + WIFININA_OPTION_END;

      pitem = String(WIFININA_HTML_HEAD_END);

#if MANUAL_SSID_INPUT_ALLOWED
      pitem.replace("[[input_id]]",  "<input id='id' list='SSIDs'>"  + String(WIFININA_DATALIST_START) + "'SSIDs'>" + ListOfSSIDs + WIFININA_DATALIST_END);
      pitem.replace("[[input_id1]]", "<input id='id1' list='SSIDs'>" + String(WIFININA_DATALIST_START) + "'SSIDs'>" + ListOfSSIDs + WIFININA_DATALIST_END);

#else
      pitem.replace("[[input_id]]",  "<select id='id'>"  + ListOfSSIDs + WIFININA_SELECT_END);
      pitem.replace("[[input_id1]]", "<select id='id1'>" + ListOfSSIDs + WIFININA_SELECT_END);
#endif

      root_html_template += pitem + WIFININA_FLDSET_START;
#else

    pitem = String(WIFININA_HTML_HEAD_END);
    pitem.replace("[[input_id]]",  WIFININA_HTML_INPUT_ID);
    root_html_template += pitem + WIFININA_FLDSET_START;
#endif    // SCAN_WIFI_NETWORKS

    root_html_template += String(WIFININA_FLDSET_END) + WIFININA_HTML_BUTTON + WIFININA_HTML_SCRIPT;

    root_html_template += String(WIFININA_HTML_SCRIPT_END) + WIFININA_HTML_END;
}

void NetworkController::ensureTopicsFormatted() {
    if (!topicsFormatted && config.has_value()) {
        snprintf(TOPIC_LWT, TOPIC_LENGTH - 1, "%s/%s/lwt", config->mqttConfig.prefix, identifier);
        snprintf(TOPIC_STATE, TOPIC_LENGTH - 1, "%s/%s/state", config->mqttConfig.prefix, identifier);
        snprintf(TOPIC_COMMAND, TOPIC_LENGTH - 1, "%s/%s/cmd", config->mqttConfig.prefix, identifier);

        snprintf(TOPIC_AUTOCONF_STATE_SENSOR, 127, "homeassistant/sensor/%s/%s_state/config", config->mqttConfig.prefix, identifier);
        snprintf(TOPIC_AUTOCONF_BREW_BOILER_SENSOR, 127, "homeassistant/sensor/%s/%s_brew_temp/config", config->mqttConfig.prefix, identifier);
        snprintf(TOPIC_AUTOCONF_SERVICE_BOILER_SENSOR, 127, "homeassistant/sensor/%s/%s_serv_temp/config", config->mqttConfig.prefix, identifier);
        snprintf(TOPIC_AUTOCONF_ECO_MODE_SWITCH, 127, "homeassistant/switch/%s/%s_eco_mode/config", config->mqttConfig.prefix, identifier);
        snprintf(TOPIC_AUTOCONF_SLEEP_MODE_SWITCH, 127, "homeassistant/switch/%s/%s_sleep_mode/config", config->mqttConfig.prefix, identifier);
        snprintf(TOPIC_AUTOCONF_BREW_TEMPERATURE_TARGET_NUMBER, 127, "homeassistant/number/%s/%s_brew_temp_target/config", config->mqttConfig.prefix, identifier);
        snprintf(TOPIC_AUTOCONF_SERVICE_TEMPERATURE_TARGET_NUMBER, 127, "homeassistant/number/%s/%s_serv_temp_target/config", config->mqttConfig.prefix, identifier);
        snprintf(TOPIC_AUTOCONF_WATER_TANK_LOW_BINARY_SENSOR, 127, "homeassistant/binary_sensor/%s/%s_water_tank_low/config", config->mqttConfig.prefix, identifier);
        snprintf(TOPIC_AUTOCONF_WIFI_SENSOR, 127, "homeassistant/sensor/%s/%s_wifi/config", config->mqttConfig.prefix, identifier);
        snprintf(TOPIC_AUTOCONF_AUTO_SLEEP_MIN, 127, "homeassistant/number/%s/%s_auto_sleep_min/config", config->mqttConfig.prefix, identifier);
        snprintf(TOPIC_AUTOCONF_PLANNED_AUTO_SLEEP_MIN, 127, "homeassistant/sensor/%s/%s_planned_auto_sleep_min/config", config->mqttConfig.prefix, identifier);

        topicsFormatted = true;
    }
}

void NetworkController::publishMqtt() {
    mqtt.publish(TOPIC_LWT, "online");

    DynamicJsonDocument doc(1024);

    JsonObject conf = doc.createNestedObject("conf");

    JsonObject conf_brew = conf.createNestedObject("brew");
    conf_brew["temp_target"] = status->getOffsetTargetBrewTemperature();
    conf_brew["temp_offset"] = status->getBrewTempOffset();

    JsonObject conf_brew_pid = conf_brew.createNestedObject("pid");
    conf_brew_pid["Kp"] = status->getBrewPidSettings().Kp;
    conf_brew_pid["Ki"] = status->getBrewPidSettings().Ki;
    conf_brew_pid["Kd"] = status->getBrewPidSettings().Kd;
    conf_brew_pid["windupHigh"] = status->getBrewPidSettings().windupHigh;
    conf_brew_pid["windupLow"] = status->getBrewPidSettings().windupLow;

    JsonObject conf_service = conf.createNestedObject("service");
    conf_service["temp_target"] = status->getTargetServiceTemp();

    JsonObject conf_service_pid = conf_service.createNestedObject("pid");
    conf_service_pid["Kp"] = status->getServicePidSettings().Kp;
    conf_service_pid["Ki"] = status->getServicePidSettings().Ki;
    conf_service_pid["Kd"] = status->getServicePidSettings().Kd;
    conf_service_pid["windupHigh"] = status->getServicePidSettings().windupHigh;
    conf_service_pid["windupLow"] = status->getServicePidSettings().windupLow;

    conf["eco_mode"] = status->isInEcoMode();
    conf["sleep_mode"] = status->isInSleepMode();
    conf["auto_sleep_min"] = settings->getAutoSleepMin();

    JsonObject stat = doc.createNestedObject("stat");

    switch (status->getState()) {
        case SYSTEM_CONTROLLER_STATE_UNDETERMINED:
            stat["state"] = "Undetermined";
            break;
        case SYSTEM_CONTROLLER_STATE_HEATUP:
            stat["state"] = "Heatup";
            break;
        case SYSTEM_CONTROLLER_STATE_TEMPS_NORMALIZING:
            stat["state"] = "Temperatures normalizing";
            break;
        case SYSTEM_CONTROLLER_STATE_WARM:
            stat["state"] = "Warm";
            break;
        case SYSTEM_CONTROLLER_STATE_SLEEPING:
            stat["state"] = "Sleeping";
            break;
        case SYSTEM_CONTROLLER_STATE_BAILED:
            stat["state"] = "Bailed";
            break;
        case SYSTEM_CONTROLLER_STATE_FIRST_RUN:
            stat["state"] = "First run";
            break;
        default:
            stat["state"] = std::string("Unknown (") + std::to_string((uint8_t)status->getState()) + ")";
            break;
    }

    JsonObject stat_internal = stat.createNestedObject("internal");
    stat_internal["rx"] = status->hasReceivedControlBoardPacket;
    stat_internal["tx"] = status->hasSentLccPacket;
    stat_internal["bailed"] = status->hasBailed();
    stat_internal["watchdog_reset"] = watchdog_enable_caused_reboot();

    switch (status->bailReason()) {
        case BAIL_REASON_NONE:
            stat_internal["bail_reason"] = "None";
            break;
        case BAIL_REASON_CB_UNRESPONSIVE:
            stat_internal["bail_reason"] = "CB unresponsive";
            break;
        case BAIL_REASON_CB_PACKET_INVALID:
            stat_internal["bail_reason"] = "CB packet invalid";
            break;
        case BAIL_REASON_LCC_PACKET_INVALID:
            stat_internal["bail_reason"] = "LCC packet invalid";
            break;
        case BAIL_REASON_SSR_QUEUE_EMPTY:
            stat_internal["bail_reason"] = "SSR queue empty";
            break;
        default:
            stat_internal["bail_reason"] = std::string("Unknown (") + std::to_string((uint8_t)status->bailReason()) + ")";
            break;
    }

    JsonObject stat_brew_pid = stat.createNestedObject("brew_pid");
    stat_brew_pid["p"] = status->getBrewPidRuntimeParameters().p;
    stat_brew_pid["i"] = status->getBrewPidRuntimeParameters().i;
    stat_brew_pid["d"] = status->getBrewPidRuntimeParameters().d;
    stat_brew_pid["integral"] = status->getBrewPidRuntimeParameters().integral;
    stat_brew_pid["hysteresisMode"] = status->getBrewPidRuntimeParameters().hysteresisMode;

    JsonObject stat_service_pid = stat.createNestedObject("service_pid");
    stat_service_pid["p"] = status->getServicePidRuntimeParameters().p;
    stat_service_pid["i"] = status->getServicePidRuntimeParameters().i;
    stat_service_pid["d"] = status->getServicePidRuntimeParameters().d;
    stat_service_pid["integral"] = status->getServicePidRuntimeParameters().integral;
    stat_service_pid["hysteresisMode"] = status->getServicePidRuntimeParameters().hysteresisMode;

    IPAddress ip = WiFi.localIP();
    uint8_t mac[WL_MAC_ADDR_LENGTH];
    WiFi.macAddress(mac);

    std::string ipString = std::to_string(ip[0]) + "." + std::to_string(ip[1]) + "." + std::to_string(ip[2]) + "." + std::to_string(ip[3]);

    char macAddress[19];
    snprintf(macAddress, sizeof(macAddress), "%02X:%02X:%02X:%02X:%02X:%02X", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);

    JsonObject stat_wifi = stat.createNestedObject("wifi");
    stat_wifi["ssid"] = WiFi.SSID();
    stat_wifi["ip"] = ipString;
    stat_wifi["rssi"] = WiFi.RSSI();
    stat_wifi["mac"] = macAddress;
    stat_wifi["nina_firmware"] = WiFi.firmwareVersion();

    stat["brew_temp"] = status->getOffsetBrewTemperature();
    stat["service_temp"] = status->getServiceTemperature();
    stat["water_tank_empty"] = status->isWaterTankEmpty();

    if (!status->plannedAutoSleepAt.has_value()) {
        stat["auto_sleep_in"] = false;
    } else {
        double sleepTime = (double)absolute_time_diff_us(get_absolute_time(), status->plannedAutoSleepAt.value()) / 60000000.f;
        if (sleepTime >= 0.f) {
            stat["auto_sleep_in"] = sleepTime;
        } else {
            stat["auto_sleep_in"] = false;
        }
    }


    if (status->hasPreviousBrew()) {
        stat["last_shot_duration"] = (float)status->previousBrewDurationMs() / 1000.f;
    } else {
        stat["last_shot_duration"] = false;
    }

    std::string output;
    serializeJson(doc, output);
    mqtt.publish(TOPIC_STATE, output.c_str(), false);
}

void NetworkController::callback(char *topic, byte *payload, unsigned int length) {
    DEBUGV("Received callback of length %u\n", length);

    StaticJsonDocument<128> doc;

    DeserializationError error = deserializeJson(doc, payload, length);

    if (error) {
        DEBUGV("deserializeJson() failed: %s\n", error.c_str());
        return;
    }

    std::string cmd = doc["cmd"];

    if (cmd == "set_brew_temp_target") {
        settings->setOffsetTargetBrewTemp(doc["float_value"]);
    } else if (cmd == "set_brew_temp_offset") {
        settings->setBrewTemperatureOffset(doc["float_value"]);
    } else if (cmd == "set_brew_pid_params") {
        PidSettings params{
            .Kp = doc["kp"],
            .Ki = doc["ki"],
            .Kd = doc["kd"],
            .windupLow = doc["windup_low"],
            .windupHigh = doc["windup_high"]
        };

        settings->setBrewPidParameters(params);
    } else if (cmd == "set_service_pid_params") {
        PidSettings params{
                .Kp = doc["kp"],
                .Ki = doc["ki"],
                .Kd = doc["kd"],
                .windupLow = doc["windup_low"],
                .windupHigh = doc["windup_high"]
        };

        settings->setServicePidParameters(params);
    } else if (cmd == "set_service_temp_target") {
        settings->setTargetServiceTemp(doc["float_value"]);
    } else if (cmd == "set_eco_mode") {
        settings->setEcoMode(doc["bool_value"]);
    } else if (cmd == "set_sleep_mode") {
        settings->setSleepMode(doc["bool_value"]);
    } else if (cmd == "set_auto_sleep_min") {
        settings->setAutoSleepMin(doc["int_value"]);
    } else {
        DEBUGV("Unknown command");
    }
}

void NetworkController::publishAutoconfigure() {
    std::string mqttPayload;
    DynamicJsonDocument device(256);
    DynamicJsonDocument autoconfPayload(1024);
    size_t len;

    char macString[19];
    uint8_t mac[WL_MAC_ADDR_LENGTH];
    WiFi.macAddress(mac);
    snprintf(macString, sizeof(macString), "%02x:%02x:%02x:%02x:%02x:%02x", mac[5], mac[4], mac[3], mac[2], mac[1], mac[0]);

    device["ids"][0] = stdIdentifier;
    device["cns"][0][0] = "mac";
    device["cns"][0][1] = macString;
    device["mf"] = "magnusnordlander";
    device["mdl"] = "smart-lcc";
    device["name"] = stdIdentifier;
    device["sw"] = "0.2.0";

    JsonObject devObj = device.as<JsonObject>();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_LWT;
    autoconfPayload["stat_t"] = TOPIC_STATE;
    autoconfPayload["json_attr_t"] = TOPIC_STATE;
    autoconfPayload["name"] = stdIdentifier + " State";
    autoconfPayload["uniq_id"] = stdIdentifier + "_state";
    autoconfPayload["ic"] = "mdi:eye";
    autoconfPayload["val_tpl"] = "{{ value_json.stat.state }}";
    autoconfPayload["json_attr_tpl"] = R"({"bail_reason": "{{value_json.stat.internal.bail_reason}}", "watchdog_reset": "{{value_json.stat.internal.watchdog_reset}}"})";

    serializeJson(autoconfPayload, mqttPayload);
    mqtt.publish(&TOPIC_AUTOCONF_STATE_SENSOR[0], mqttPayload.c_str(), true);
    autoconfPayload.clear();
    mqttPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_LWT;
    autoconfPayload["stat_t"] = TOPIC_STATE;
    autoconfPayload["json_attr_t"] = TOPIC_STATE;
    autoconfPayload["name"] = stdIdentifier + " Brew Boiler Temperature";
    autoconfPayload["uniq_id"] = stdIdentifier +"_brew_temp";
    autoconfPayload["unit_of_meas"] = "°C";
    autoconfPayload["ic"] = "mdi:thermometer";
    autoconfPayload["dev_cla"] = "temperature";
    autoconfPayload["val_tpl"] = "{{ value_json.stat.brew_temp | round(1) }}";
    autoconfPayload["json_attr_tpl"] = R"({"p": "{{value_json.stat.brew_pid.p}}", "i": "{{value_json.stat.brew_pid.i}}", "d": "{{value_json.stat.brew_pid.d}}", "integral": "{{value_json.stat.brew_pid.integral}}",  "hysteresis_mode": "{{value_json.stat.brew_pid.hysteresisMode}}"})";

    serializeJson(autoconfPayload, mqttPayload);
    mqtt.publish(&TOPIC_AUTOCONF_BREW_BOILER_SENSOR[0], mqttPayload.c_str(), true);
    autoconfPayload.clear();
    mqttPayload.clear();


    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_LWT;
    autoconfPayload["stat_t"] = TOPIC_STATE;
    autoconfPayload["json_attr_t"] = TOPIC_STATE;
    autoconfPayload["name"] = stdIdentifier + " Service Boiler Temperature";
    autoconfPayload["uniq_id"] = stdIdentifier +"_serv_temp";
    autoconfPayload["unit_of_meas"] = "°C";
    autoconfPayload["ic"] = "mdi:thermometer";
    autoconfPayload["dev_cla"] = "temperature";
    autoconfPayload["val_tpl"] = "{{ value_json.stat.service_temp | round(1) }}";
    autoconfPayload["json_attr_tpl"] = R"({"p": "{{value_json.stat.service_pid.p}}", "i": "{{value_json.stat.service_pid.i}}", "d": "{{value_json.stat.service_pid.d}}", "integral": "{{value_json.stat.service_pid.integral}}",  "hysteresis_mode": "{{value_json.stat.service_pid.hysteresisMode}}"})";

    serializeJson(autoconfPayload, mqttPayload);
    mqtt.publish(&TOPIC_AUTOCONF_SERVICE_BOILER_SENSOR[0], mqttPayload.c_str(), true);
    autoconfPayload.clear();
    mqttPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_LWT;
    autoconfPayload["stat_t"] = TOPIC_STATE;
    autoconfPayload["json_attr_t"] = TOPIC_STATE;
    autoconfPayload["name"] = stdIdentifier + " WiFi";
    autoconfPayload["uniq_id"] = stdIdentifier + "_wifi";
    autoconfPayload["val_tpl"] = "{{value_json.stat.wifi.rssi}}";
    autoconfPayload["unit_of_meas"] = "dBm";
    autoconfPayload["json_attr_tpl"] = R"({"ssid": "{{value_json.stat.wifi.ssid}}", "ip": "{{value_json.stat.wifi.ip}}"})";
    autoconfPayload["ic"] = "mdi:wifi";
    autoconfPayload["entity_category"] = "diagnostic";

    serializeJson(autoconfPayload, mqttPayload);
    mqtt.publish(&TOPIC_AUTOCONF_WIFI_SENSOR[0], mqttPayload.c_str(), true);
    autoconfPayload.clear();
    mqttPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_LWT;
    autoconfPayload["stat_t"] = TOPIC_STATE;
    autoconfPayload["json_attr_t"] = TOPIC_STATE;
    autoconfPayload["cmd_t"] = TOPIC_COMMAND;
    autoconfPayload["name"] = stdIdentifier + " Eco Mode";
    autoconfPayload["uniq_id"] = stdIdentifier + "_eco_mode";
    autoconfPayload["ic"] = "hass:leaf";
    autoconfPayload["val_tpl"] = R"({{ 'ON' if value_json.conf.eco_mode else 'OFF' }})";
    autoconfPayload["pl_on"] = R"({"cmd": "set_eco_mode", "bool_value": true})";
    autoconfPayload["pl_off"] = R"({"cmd": "set_eco_mode", "bool_value": false})";
    autoconfPayload["stat_on"] = "ON";
    autoconfPayload["stat_off"] = "OFF";
    autoconfPayload["entity_category"] = "config";

    serializeJson(autoconfPayload, mqttPayload);
    mqtt.publish(&TOPIC_AUTOCONF_ECO_MODE_SWITCH[0], mqttPayload.c_str(), true);
    autoconfPayload.clear();
    mqttPayload.clear();


    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_LWT;
    autoconfPayload["stat_t"] = TOPIC_STATE;
    autoconfPayload["json_attr_t"] = TOPIC_STATE;
    autoconfPayload["cmd_t"] = TOPIC_COMMAND;
    autoconfPayload["name"] = stdIdentifier + " Sleep Mode";
    autoconfPayload["uniq_id"] = stdIdentifier + "_sleep_mode";
    autoconfPayload["ic"] = "hass:sleep";
    autoconfPayload["val_tpl"] = R"({{ 'ON' if value_json.conf.sleep_mode else 'OFF' }})";
    autoconfPayload["pl_on"] = R"({"cmd": "set_sleep_mode", "bool_value": true})";
    autoconfPayload["pl_off"] = R"({"cmd": "set_sleep_mode", "bool_value": false})";
    autoconfPayload["stat_on"] = "ON";
    autoconfPayload["stat_off"] = "OFF";
    autoconfPayload["entity_category"] = "config";

    serializeJson(autoconfPayload, mqttPayload);
    mqtt.publish(&TOPIC_AUTOCONF_SLEEP_MODE_SWITCH[0], mqttPayload.c_str(), true);
    autoconfPayload.clear();
    mqttPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_LWT;
    autoconfPayload["stat_t"] = TOPIC_STATE;
    autoconfPayload["cmd_t"] = TOPIC_COMMAND;
    autoconfPayload["name"] = stdIdentifier + " Brew Boiler Temperature Target";
    autoconfPayload["uniq_id"] = stdIdentifier + "_brew_temp_target";
    autoconfPayload["unit_of_meas"] = "°C";
    autoconfPayload["ic"] = "mdi:thermometer";
    autoconfPayload["step"] = 0.1;
    autoconfPayload["min"] = 0;
    autoconfPayload["max"] = 100;
    autoconfPayload["val_tpl"] = R"({{ value_json.conf.brew.temp_target | round(1) }})";
    autoconfPayload["cmd_tpl"] = R"({"cmd": "set_brew_temp_target", "float_value": {{value}} })";
    autoconfPayload["entity_category"] = "config";

    serializeJson(autoconfPayload, mqttPayload);
    mqtt.publish(&TOPIC_AUTOCONF_BREW_TEMPERATURE_TARGET_NUMBER[0], mqttPayload.c_str(), true);
    autoconfPayload.clear();
    mqttPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_LWT;
    autoconfPayload["stat_t"] = TOPIC_STATE;
    autoconfPayload["cmd_t"] = TOPIC_COMMAND;
    autoconfPayload["name"] = stdIdentifier + " Service Boiler Temperature Target";
    autoconfPayload["uniq_id"] = stdIdentifier + "_service_temp_target";
    autoconfPayload["unit_of_meas"] = "°C";
    autoconfPayload["ic"] = "mdi:thermometer";
    autoconfPayload["step"] = 0.1;
    autoconfPayload["min"] = 0;
    autoconfPayload["max"] = 150;
    autoconfPayload["val_tpl"] = R"({{ value_json.conf.service.temp_target | round(1) }})";
    autoconfPayload["cmd_tpl"] = R"({"cmd": "set_service_temp_target", "float_value": {{value}} })";
    autoconfPayload["entity_category"] = "config";

    serializeJson(autoconfPayload, mqttPayload);
    mqtt.publish(&TOPIC_AUTOCONF_SERVICE_TEMPERATURE_TARGET_NUMBER[0], mqttPayload.c_str(), true);
    autoconfPayload.clear();
    mqttPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_LWT;
    autoconfPayload["stat_t"] = TOPIC_STATE;
    autoconfPayload["cmd_t"] = TOPIC_COMMAND;
    autoconfPayload["name"] = stdIdentifier + " Auto-sleep after";
    autoconfPayload["uniq_id"] = stdIdentifier + "_auto_sleep_min";
    autoconfPayload["unit_of_meas"] = "minutes";
    autoconfPayload["ic"] = "hass:sleep";
    autoconfPayload["step"] = 1;
    autoconfPayload["min"] = 0;
    autoconfPayload["max"] = 300;
    autoconfPayload["val_tpl"] = R"({{ value_json.conf.auto_sleep_min }})";
    autoconfPayload["cmd_tpl"] = R"({"cmd": "set_auto_sleep_min", "int_value": {{value}} })";
    autoconfPayload["entity_category"] = "config";

    serializeJson(autoconfPayload, mqttPayload);
    mqtt.publish(&TOPIC_AUTOCONF_AUTO_SLEEP_MIN[0], mqttPayload.c_str(), true);
    autoconfPayload.clear();
    mqttPayload.clear();

    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_LWT;
    autoconfPayload["stat_t"] = TOPIC_STATE;
    autoconfPayload["cmd_t"] = TOPIC_COMMAND;
    autoconfPayload["name"] = stdIdentifier + " Water Tank Low";
    autoconfPayload["uniq_id"] = stdIdentifier + "_water_tank_low";
    autoconfPayload["ic"] = "mdi:water-alert";
    autoconfPayload["dev_cla"] = "problem";
    autoconfPayload["val_tpl"] = R"({{ 'ON' if value_json.stat.water_tank_empty else 'OFF' }})";


    serializeJson(autoconfPayload, mqttPayload);
    mqtt.publish(&TOPIC_AUTOCONF_WATER_TANK_LOW_BINARY_SENSOR[0], mqttPayload.c_str(), true);
    autoconfPayload.clear();
    mqttPayload.clear();


    autoconfPayload["dev"] = devObj;
    autoconfPayload["avty_t"] = TOPIC_LWT;
    autoconfPayload["stat_t"] = TOPIC_STATE;
    autoconfPayload["json_attr_t"] = TOPIC_STATE;
    autoconfPayload["name"] = stdIdentifier + " Planned Auto Sleep in";
    autoconfPayload["uniq_id"] = stdIdentifier + "_planned_auto_sleep";
    autoconfPayload["val_tpl"] = "{{value_json.stat.auto_sleep_in | round(0) }}";
    autoconfPayload["unit_of_meas"] = "minutes";
    autoconfPayload["ic"] = "hass:sleep";
    autoconfPayload["entity_category"] = "diagnostic";

    serializeJson(autoconfPayload, mqttPayload);
    mqtt.publish(&TOPIC_AUTOCONF_PLANNED_AUTO_SLEEP_MIN[0], mqttPayload.c_str(), true);
    autoconfPayload.clear();
    mqttPayload.clear();

}

nonstd::optional<IPAddress> NetworkController::getIPAddress() {
    if (!isConnectedToWifi()) {
        return nonstd::optional<IPAddress>();
    }

    return nonstd::optional<IPAddress>(WiFi.localIP());
}
