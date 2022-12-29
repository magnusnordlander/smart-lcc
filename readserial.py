import serial
import cstruct
import os
import pprint

class State(cstruct.CEnum):
    __size__ = 1
    __def__ = """
    enum {
    SYSTEM_CONTROLLER_STATE_UNDETERMINED = 0,
    SYSTEM_CONTROLLER_STATE_HEATUP,
    SYSTEM_CONTROLLER_STATE_TEMPS_NORMALIZING,
    SYSTEM_CONTROLLER_STATE_WARM,
    SYSTEM_CONTROLLER_STATE_SLEEPING,
    SYSTEM_CONTROLLER_STATE_BAILED,
    SYSTEM_CONTROLLER_STATE_FIRST_RUN
};
    """

class BailReason(cstruct.CEnum):
    __size__ = 1
    __def__ = """
enum {
    BAIL_REASON_NONE = 0,
    BAIL_REASON_CB_UNRESPONSIVE = 1,
    BAIL_REASON_CB_PACKET_INVALID = 2,
    BAIL_REASON_LCC_PACKET_INVALID = 3,
    BAIL_REASON_SSR_QUEUE_EMPTY = 4,
}
    """

class AbsoluteTime(cstruct.CStruct):
    __def__ = """
    struct absolute_time_t {
        uint64_t _private_us_since_boot;
    }
    """

class PidSettings(cstruct.CStruct):
    __def__ = """
    struct {
        float Kp;
        float Ki;
        float Kd;
        float windupLow;
        float windupHigh;
    }
    """

class PidRuntimeParams(cstruct.CStruct):
    __def__ = """
struct {
        uint8_t hysteresisMode;
        float p;
        float i;
        float d;
        float integral;
    }
    """

SystemControllerStatusMessage = cstruct.parse(
    """       
struct SystemControllerStatusMessage{
    struct AbsoluteTime timestamp;
    float brewTemperature;
    float brewSetPoint;
    struct PidSettings brewPidSettings;
    struct PidRuntimeParams brewPidParameters;
    float serviceTemperature;
    float serviceSetPoint;
    struct PidSettings servicePidSettings;
    struct PidRuntimeParams servicePidParameters;
    uint8_t brewSSRActive;
    uint8_t serviceSSRActive;
    uint8_t ecoMode;
    enum State state;
    enum BailReason bailReason;
    uint8_t currentlyBrewing;
    uint8_t currentlyFillingServiceBoiler;
    uint8_t waterTankLow;
    struct AbsoluteTime lastSleepModeExitAt;
};
    """
)

ser = serial.Serial('/dev/cu.usbmodem0002610145031', 115200, timeout=4)

while True:
    data = ser.read(SystemControllerStatusMessage.sizeof())
    msg = SystemControllerStatusMessage()
    msg.unpack(data)

    os.system('clear')

    pprint.pprint({
        "timestamp": msg.timestamp._private_us_since_boot,
        "brewTemperature": msg.brewTemperature,
        "brewSetPoint": msg.brewSetPoint,
        "brewPidSettings": msg.brewPidSettings,
        "brewPidParameters": msg.brewPidParameters,
        "serviceTemperature": msg.serviceTemperature,
        "serviceSetPoint": msg.serviceSetPoint,
        "servicePidSettings": msg.servicePidSettings,
        "servicePidParameters": msg.servicePidParameters,
        "brewSSRActive": msg.brewSSRActive != 0,
        "serviceSSRActive": msg.serviceSSRActive != 0,
        "ecoMode": msg.ecoMode != 0,
        "state": msg.state,
        "bailReason": msg.bailReason,
        "currentlyBrewing": msg.currentlyBrewing != 0,
        "currentlyFillingServiceBoiler": msg.currentlyFillingServiceBoiler != 0,
        "waterTankLow": msg.waterTankLow != 0,
        "lastSleepModeExitAt": msg.lastSleepModeExitAt._private_us_since_boot,
    }, sort_dicts=False, indent=4, width=5)

