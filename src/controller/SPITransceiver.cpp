//
// Created by Magnus Nordlander on 2021-03-17.
//

#include "SPITransceiver.h"

SPITransceiver::SPITransceiver(SoftSPI *softSpi, pin_size_t ssPin) : softSpi(softSpi), ssPin(ssPin) {
    pinMode(ssPin, OUTPUT);
}

void SPITransceiver::send(uint8_t *message, uint16_t len) {
    digitalWrite(ssPin, LOW);

    uint8_t received;

    for (int i = 0; i < len; i++) {
        received = softSpi->transfer(message[i]);
        if (received != PERIPHERAL_NONE) {

        }
    }
}

void SPITransceiver::receive(uint8_t bytes) {

}
