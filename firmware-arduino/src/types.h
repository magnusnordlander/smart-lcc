//
// Created by Magnus Nordlander on 2021-08-20.
//

#ifndef FIRMWARE_TYPES_H
#define FIRMWARE_TYPES_H

#include <pico/time.h>

typedef enum {
    NETWORK_CONTROLLER_MODE_NORMAL,
    NETWORK_CONTROLLER_MODE_CONFIG,
    NETWORK_CONTROLLER_MODE_OTA
} NetworkControllerMode;

typedef enum {
    SYSTEM_CONTROLLER_STATE_UNDETERMINED,
    SYSTEM_CONTROLLER_STATE_HEATUP,
    SYSTEM_CONTROLLER_STATE_TEMPS_NORMALIZING,
    SYSTEM_CONTROLLER_STATE_WARM,
    SYSTEM_CONTROLLER_STATE_SLEEPING,
    SYSTEM_CONTROLLER_STATE_BAILED,
    SYSTEM_CONTROLLER_STATE_FIRST_RUN
} SystemControllerState;

typedef enum {
    BAIL_REASON_NONE = 0,
    BAIL_REASON_CB_UNRESPONSIVE = 1,
    BAIL_REASON_CB_PACKET_INVALID = 2,
    BAIL_REASON_LCC_PACKET_INVALID = 3,
    BAIL_REASON_SSR_QUEUE_EMPTY = 4,
} SystemControllerBailReason;

struct PidSettings {
    float Kp{};
    float Ki{};
    float Kd{};
    float windupLow{};
    float windupHigh{};
};

struct PidRuntimeParameters {
    bool hysteresisMode = false;
    float p = 0;
    float i = 0;
    float d = 0;
    float integral = 0;
};

struct SettingStruct {
    float brewTemperatureOffset = -10;
    bool sleepMode = false;
    bool ecoMode = false;
    float brewTemperatureTarget = 105;
    float serviceTemperatureTarget = 120;
    PidSettings brewPidParameters = PidSettings{.Kp = 0.8, .Ki = 0.12, .Kd = 12.0, .windupLow = -7.f, .windupHigh = 7.f};
    PidSettings servicePidParameters = PidSettings{.Kp = 0.6, .Ki = 0.1, .Kd = 1.0, .windupLow = -10.f, .windupHigh = 10.f};
};

#define SSID_MAX_LEN      32
#define PASS_MAX_LEN      64
#define MQTT_SERVER_LEN       20
#define MQTT_PORT_LEN         5
#define MQTT_USERNAME_LEN     20
#define MQTT_PASS_LEN         40
#define MQTT_PREFIX_LEN       20

struct WiFi_Credentials
{
    char wifi_ssid[SSID_MAX_LEN];
    char wifi_pw  [PASS_MAX_LEN];
};

struct MQTT_Configuration
{
    char server[MQTT_SERVER_LEN + 1] = "test.mosquitto.org";
    char port[MQTT_PORT_LEN + 1] = "1883";
    char username[MQTT_USERNAME_LEN + 1] = "";
    char password[MQTT_PASS_LEN + 1] = "";
    char prefix[MQTT_PREFIX_LEN + 1] = "lcc";
};

struct WiFiNINA_Configuration
{
    WiFi_Credentials wiFiCredentials{};
    MQTT_Configuration mqttConfig{};
};

struct InternalStateMessage {
    bool bailed{};
    SystemControllerBailReason  bailReason{};
    bool resetByWatchdog{};
    bool rx{};
    bool tx{};
};

struct SystemControllerStatusMessage{
    absolute_time_t timestamp{};
    float brewTemperature{};
    float brewSetPoint{};
    PidSettings brewPidSettings{};
    PidRuntimeParameters brewPidParameters{};
    float serviceTemperature{};
    float serviceSetPoint{};
    PidSettings servicePidSettings{};
    PidRuntimeParameters servicePidParameters{};
    bool brewSSRActive{};
    bool serviceSSRActive{};
    bool ecoMode{};
    SystemControllerState state{};
    SystemControllerBailReason bailReason{};
    bool currentlyBrewing{};
    bool currentlyFillingServiceBoiler{};
    bool waterTankLow{};
};

typedef enum {
    COMMAND_SET_BREW_SET_POINT,
    COMMAND_SET_BREW_PID_PARAMETERS,
    COMMAND_SET_SERVICE_SET_POINT,
    COMMAND_SET_SERVICE_PID_PARAMETERS,
    COMMAND_SET_ECO_MODE,
    COMMAND_SET_SLEEP_MODE,
    COMMAND_UNBAIL,
    COMMAND_TRIGGER_FIRST_RUN,
    COMMAND_INITIALIZE,
    COMMAND_VICTIMIZE
} SystemControllerCommandType;

struct SystemControllerCommand {
    SystemControllerCommandType type{};
    float float1{};
    float float2{};
    float float3{};
    float float4{};
    float float5{};
    bool bool1{};
};

#endif //FIRMWARE_TYPES_H
