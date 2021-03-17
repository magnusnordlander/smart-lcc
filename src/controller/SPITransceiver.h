//
// Created by Magnus Nordlander on 2021-03-17.
//

#ifndef SMART_LCC_SPITRANSCEIVER_H
#define SMART_LCC_SPITRANSCEIVER_H

#include <SoftSPI.h>
#include <protocol.h>

class SPITransceiver {
public:
    explicit SPITransceiver(SoftSPI *softSpi, pin_size_t ssPin);

    void send(uint8_t message[], uint16_t len);
    void receive(uint8_t bytes);
private:
    SoftSPI* softSpi;
    pin_size_t ssPin;

};


#endif //SMART_LCC_SPITRANSCEIVER_H
