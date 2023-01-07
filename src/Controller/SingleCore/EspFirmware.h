//
// Created by Magnus Nordlander on 2023-01-04.
//

#ifndef SMART_LCC_ESPFIRMWARE_H
#define SMART_LCC_ESPFIRMWARE_H


#include <hardware/uart.h>
#include "esp-protocol.h"
#include "Controller/Core0/Utils/ClearUartCruft.h"
#include "Controller/Core0/Utils/UartReadBlockingTimeout.h"
#include "types.h"
#include "utils/PicoQueue.h"
#include <ringbuffer.hpp>

class EspFirmware {
public:
    explicit EspFirmware(uart_inst_t *uart, PicoQueue<SystemControllerCommand> *commandQueue);

    void loop();

    static void initInterrupts(uart_inst_t *uart);
    static void onUartRx();
    static jnk0le::Ringbuffer<uint8_t, 1024> ringbuffer;
    static uart_inst_t *interruptedUart;

    bool pingBlocking();
    bool sendStatus(SystemControllerStatusMessage *systemControllerStatusMessage);

private:
    uart_inst_t *uart;
    PicoQueue<SystemControllerCommand> *commandQueue;

    bool waitForAck(uint32_t id);
    void handleCommand(ESPMessageHeader *header);

    void sendAck(uint32_t messageId);
    void sendNack(uint32_t messageId, ESPError error);

    static bool readFromRingBufferBlockingWithTimeout(uint8_t *dst, size_t len, absolute_time_t timeout_time);
};


#endif //SMART_LCC_ESPFIRMWARE_H
