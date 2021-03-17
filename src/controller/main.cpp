#include <Arduino.h>
#include "wiring_private.h"
#include "BiancaUart.h"
#include <SPI.h>
#include <SoftSPI.h>
#include <protocol.h>
#include "SPITransceiver.h"

BiancaUart<3> lccSerial (&sercom0, 5, 6, SERCOM_RX_PAD_1, UART_TX_PAD_0);

void SERCOM0_Handler()
{
    lccSerial.IrqHandler();
}

BiancaUart<16> controlBoardSerial( &sercom5, PIN_SERIAL1_RX, PIN_SERIAL1_TX, PAD_SERIAL1_RX, PAD_SERIAL1_TX ) ;

void SERCOM5_Handler()
{
    controlBoardSerial.IrqHandler();
}

SoftSPI softSpi(14, 15, 16);
SPITransceiver transceiver(&softSpi, 17);

void setup() {
    pinPeripheral(5, PIO_SERCOM_ALT);
    pinPeripheral(6, PIO_SERCOM_ALT);

    Serial.begin(115200);
    controlBoardSerial.begin(9600);
    lccSerial.begin(9600);

    softSpi.begin();

    Serial.println("Ready");
    Serial.println();
}

unsigned long previousControlBoardDatagram = 0;
unsigned long previousLccDatagram = 0;

uint8_t controlBoardDatagram[16];
uint8_t lccDatagram[3];

uint16_t transformHextripet(uint8_t byte0, uint8_t byte1, uint8_t byte2) {
    uint32_t triplet = ((uint32_t)byte0 << 16) | ((uint32_t)byte1 << 8) | (uint32_t)byte2;

    uint8_t lsb = (triplet & 0x00FE00) >> 9;

    uint8_t hig = (triplet & 0x060000) >> 16;
    uint8_t mid = (triplet & 0x000002) >> 1;

    uint8_t msb = (hig | mid) - 1;

    return (uint16_t)lsb | ((uint16_t)msb << 7);
}

SensorReading createSensorReading() {
    bool microswitchOn = (controlBoardDatagram[1] & 0x40) == 0x00;
    uint16_t brewboiler = transformHextripet(controlBoardDatagram[7], controlBoardDatagram[8], controlBoardDatagram[9]);
    uint16_t serviceboiler = transformHextripet(controlBoardDatagram[10], controlBoardDatagram[11], controlBoardDatagram[12]);
    uint16_t waterlevel = transformHextripet(controlBoardDatagram[13], controlBoardDatagram[14], controlBoardDatagram[15]);

    bool serviceboilerhe = (lccDatagram[0] & 0x0E) == 0x0A;
    bool brewboilerhe = (lccDatagram[1] & 0xF0) == 0xE0;
    bool pumpOn = (lccDatagram[1] & 0x0E) == 0x0E;

    return SensorReading {
            brewboiler,
            serviceboiler,
            waterlevel,
            brewboilerhe,
            serviceboilerhe,
            pumpOn,
            microswitchOn,
    };
}

void loop() {
    unsigned long controlBoardDatagramTime = controlBoardSerial.datagramUpdatedAt;
    if ((controlBoardDatagramTime - previousControlBoardDatagram) > 0) {
        controlBoardSerial.readDatagram(controlBoardDatagram);

        SensorReadingUnion reading = {.message = createSensorReading()};
        transceiver.send(reading.bytes, sizeof(SensorReading));

        ControlBoardDatagram dg{};
        dg.type = CONTROLLER_CONTROL_BOARD_DATAGRAM;
        memcpy(dg.datagram, controlBoardDatagram, sizeof(controlBoardDatagram));
        ControlBoardDatagramUnion dgu = {.message = dg};
        transceiver.send(dgu.bytes, sizeof(ControlBoardDatagram));
    }

    unsigned long lccDatagramTime = lccSerial.datagramUpdatedAt;
    if ((lccDatagramTime - previousLccDatagram) > 0) {
        lccSerial.readDatagram(lccDatagram);

        SensorReadingUnion reading = {.message = createSensorReading()};
        transceiver.send(reading.bytes, sizeof(SensorReading));

        LccDatagram dg{};
        dg.type = CONTROLLER_LCC_DATAGRAM;
        memcpy(dg.datagram, lccDatagram, sizeof(lccDatagram));
        LccDatagramUnion dgu = {.message = dg};
        transceiver.send(dgu.bytes, sizeof(LccDatagram));
    }
}