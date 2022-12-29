//
// Created by Magnus Nordlander on 2022-01-31.
//

#ifndef FIRMWARE_ARDUINO_SAFEPACKETSENDER_H
#define FIRMWARE_ARDUINO_SAFEPACKETSENDER_H

#include <hardware/uart.h>
#include <pico/time.h>
#include "lcc_protocol.h"

class SafePacketSender {
public:
    explicit SafePacketSender(
            uart_inst_t * _uart
            );
    void loop();
private:
    uart_inst_t * uart;
    LccRawPacket safeLccRawPacket;
    absolute_time_t nextSend;
};


#endif //FIRMWARE_ARDUINO_SAFEPACKETSENDER_H
