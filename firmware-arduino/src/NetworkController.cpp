//
// Created by Magnus Nordlander on 2022-01-30.
//

#include "NetworkController.h"
#include <CRC32.h>
#include <WiFiWebServer.h>
#include <ArduinoJson.h>

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

NetworkController::NetworkController(FS* _fileSystem, SystemStatus* _status): fileSystem(_fileSystem), status(_status) {
    client = new WiFiClient;
}

void NetworkController::init(NetworkControllerMode _mode) {
    mode = _mode;
    attemptReadConfig();

    uint8_t mac[WL_MAC_ADDR_LENGTH];
    WiFi.macAddress(mac);

    snprintf(identifier, sizeof(identifier), "LCC-%02X%02X%02X", mac[3], mac[4], mac[5]);

    switch (mode) {
        case NETWORK_CONTROLLER_MODE_NORMAL:
        case NETWORK_CONTROLLER_MODE_OTA:
            break;
        case NETWORK_CONTROLLER_MODE_CONFIG:
            initConfigMode();
            break;
    }
}

void NetworkController::loop() {
    int status = WiFi.status();
    if (status != previousWifiStatus) {
        DEBUGV("Wifi status transition. From %d to %d\n", previousWifiStatus, status);
        previousWifiStatus = status;
    }

    switch (mode) {
        case NETWORK_CONTROLLER_MODE_NORMAL:
            return loopNormal();
        case NETWORK_CONTROLLER_MODE_CONFIG:
            return loopConfig();
        case NETWORK_CONTROLLER_MODE_OTA:
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

            if (mqtt->connected()) {
                if (!mqttNextPublishTime.has_value() || absolute_time_diff_us(mqttNextPublishTime.value(), get_absolute_time()) > 0) {
                    publishMqtt();
                    mqttNextPublishTime = make_timeout_time_ms(1000);
                }
            }
        }
    }
}

void NetworkController::ensureMqttClient() {
    if (!mqtt)
    {
        mqtt = new Adafruit_MQTT_Client(client, config.value().mqttConfig.server, atoi(config.value().mqttConfig.port), config.value().mqttConfig.username, config.value().mqttConfig.password);
    }

    if (!mqtt->connected()) {
        if (!mqttConnectTimeoutTime.has_value()) {
            DEBUGV("Attempting to connect to MQTT\n");
            ensureTopicsFormatted();

            mqtt->will(TOPIC_LWT, "offline");
            int8_t ret = mqtt->connect();
            if (ret != 0) {
                DEBUGV("Couldn't connect to MQTT server: %d. Reconnecting in 5 s.\n", ret);
                mqtt->disconnect();
                mqttConnectTimeoutTime = make_timeout_time_ms(5000);
                return;
            }
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

bool NetworkController::isConnectedToWifi() {
    return _isConnectedToWifi;
}

bool NetworkController::isConnectedToMqtt() {
    if (!mqtt) {
        return false;
    }

    return mqtt->connected();
}

void NetworkController::attemptReadConfig() {
    DEBUGV("Reading network config\n");
    WiFiNINA_Configuration readConfig;
    File file = fileSystem->open(CONFIG_FILENAME, "r");

    if (!file)
    {
        config.reset();
        DEBUGV("No config file\n");
        return;
    }

    file.seek(0, SeekSet);

    uint8_t readVersion;
    file.read((uint8_t *) &readVersion, sizeof(readVersion));

    if (readVersion != CONFIG_VERSION) {
        file.close();
        config.reset();
        DEBUGV("Wrong config version\n");
        return;
    }

    file.read((uint8_t *) &readConfig, sizeof(readConfig));

    uint32_t readChecksum;
    file.read((uint8_t *) &readChecksum, sizeof(readChecksum));

    file.close();

    uint32_t calculatedChecksum = CRC32::calculate((uint8_t *) &readConfig, sizeof(readConfig));

    if (calculatedChecksum != readChecksum) {
        config.reset();
        DEBUGV("Config checksum mismatch\n");
        DEBUGV("Calculated config checksum %d, read %d\n", calculatedChecksum, readChecksum);
        return;
    }

    config = readConfig;
}

void NetworkController::writeConfig(WiFiNINA_Configuration newConfig) {
    File file = fileSystem->open(CONFIG_FILENAME, "w");

    if (file)
    {
        uint32_t calculatedChecksum = CRC32::calculate((uint8_t *) &newConfig, sizeof(newConfig));

        DEBUGV("Calculated config checksum %d\n", calculatedChecksum);

        file.seek(0, SeekSet);
        file.write(CONFIG_VERSION);
        file.write((uint8_t *) &newConfig, sizeof(newConfig));
        file.write((uint8_t *) &calculatedChecksum, sizeof(calculatedChecksum));
        file.close();

        DEBUGV("Writing network config\n");

        config = newConfig;

    }
    else {
        DEBUGV("Couldn't network config file for writing\n");
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
        topicsFormatted = true;
    }
}

void NetworkController::publishMqtt() {
    mqtt->publish(TOPIC_LWT, "online");

    StaticJsonDocument<1024> doc;

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

    JsonObject stat = doc.createNestedObject("stat");

    JsonObject stat_internal = stat.createNestedObject("internal");
    stat_internal["rx"] = true;
    stat_internal["tx"] = true;
    stat_internal["bailed"] = status->hasBailed();
    stat_internal["bail_reason"] = "NONE";
    stat_internal["reset_reason"] = "WATCHDOG";

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

    JsonObject stat_wifi = stat.createNestedObject("wifi");
    stat_wifi["ssid"] = "IoT Transform";
    stat_wifi["ip"] = "192.168.11.82";
    stat_wifi["rssi"] = -52;
    stat_wifi["mac"] = "AA:BB:CC:DD:EE:FF";

    stat["brew_temp"] = status->getBrewTemperature();
    stat["service_temp"] = status->getServiceTemperature();
    stat["water_tank_empty"] = status->isWaterTankEmpty();

    char output[4096];
    serializeJson(doc, &output, sizeof(output));
    mqtt->publish(TOPIC_STATE, (uint8_t*)&output, strlen(output));
}
