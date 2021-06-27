//
// Created by Magnus Nordlander on 2021-03-16.
//

#ifndef SMART_LCC_PROTOCOL_H
#define SMART_LCC_PROTOCOL_H

typedef enum : uint8_t {
    CONTROLLER_NONE = 0x00,
    CONTROLLER_SENSOR_READINGS = 0x01,
    CONTROLLER_CONTROL_BOARD_DATAGRAM = 0xFE,
    CONTROLLER_LCC_DATAGRAM = 0xFF,
} ControllerPacketType;

typedef enum : uint8_t {
    PERIPHERAL_NONE = 0x00,
} PeripheralPacketType;

struct SensorReading {
    SensorReading(
            uint16_t bbtr,
            uint16_t sbtr,
            uint16_t wl,
            bool bbhe,
            bool sbhe,
            bool po,
            bool mo) :
            brewBoilerTempReading{bbtr},
            serviceBoilerTempReading{sbtr},
            waterLevel{wl},
            brewBoilerHE{bbhe},
            serviceboilerhe{sbhe},
            pumpOn{po},
            microswitchOn{mo}
            {};

    ControllerPacketType type = CONTROLLER_SENSOR_READINGS;
    uint16_t brewBoilerTempReading{};
    uint16_t serviceBoilerTempReading{};
    uint16_t waterLevel{};
    bool brewBoilerHE;
    bool serviceboilerhe;
    bool pumpOn;
    bool microswitchOn;
};

struct ControlBoardDatagram {
    ControllerPacketType type;
    uint8_t datagram[16];
};

struct LccDatagram {
    ControllerPacketType type;
    uint8_t datagram[3];
};

union SensorReadingUnion {
    SensorReading message;
    uint8_t bytes[sizeof(SensorReading)];
};

union ControlBoardDatagramUnion {
    ControlBoardDatagram message;
    uint8_t bytes[sizeof(ControlBoardDatagram)];
};

union LccDatagramUnion {
    LccDatagram message;
    uint8_t bytes[sizeof(LccDatagram)];
};

#endif //SMART_LCC_PROTOCOL_H
