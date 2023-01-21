//
// Created by Magnus Nordlander on 2023-01-04.
//

#ifndef SMART_LCC_ESPFIRMWARE_H
#define SMART_LCC_ESPFIRMWARE_H


#include "hardware/uart.h"
#include "esp-protocol.h"
#include "utils/ClearUartCruft.h"
#include "utils/UartReadBlockingTimeout.h"
#include "types.h"
#include "utils/PicoQueue.h"
#include "ringbuffer.hpp"
#include "SystemStatus.h"

class EspFirmware {
public:
    explicit EspFirmware(uart_inst_t *uart, PicoQueue<SystemControllerCommand> *commandQueue, SystemStatus* status);

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
    SystemStatus* status;

    bool waitForAck(uint32_t id);
    void handleESPStatus(ESPMessageHeader *header);
    void handleCommand(ESPMessageHeader *header);

    void sendAck(uint32_t messageId);
    void sendNack(uint32_t messageId, ESPError error);

    static bool readFromRingBufferBlockingWithTimeout(uint8_t *dst, size_t len, absolute_time_t timeout_time);

    static inline ESPSystemInternalState getInternalState(SystemControllerInternalState state) {
        switch(state) {
            case NOT_STARTED_YET:
                return ESP_SYSTEM_INTERNAL_STATE_NOT_STARTED_YET;
            case RUNNING:
                return ESP_SYSTEM_INTERNAL_STATE_RUNNING;
            case SOFT_BAIL:
                return ESP_SYSTEM_INTERNAL_STATE_SOFT_BAIL;
            case HARD_BAIL:
            default:
                return ESP_SYSTEM_INTERNAL_STATE_HARD_BAIL;
        }
    }

    static inline ESPSystemRunState getRunState(SystemControllerRunState state) {
        switch (state) {
            case RUN_STATE_HEATUP_STAGE_1:
                return ESP_SYSTEM_RUN_STATE_HEATUP_STAGE_1;
            case RUN_STATE_HEATUP_STAGE_2:
                return ESP_SYSTEM_RUN_STATE_HEATUP_STAGE_2;
            case RUN_STATE_NORMAL:
                return ESP_SYSTEM_RUN_STATE_NORMAL;
            case RUN_STATE_UNDETEMINED:
            default:
                return ESP_SYSTEM_RUN_STATE_UNDETEMINED;
        }
    }

    static inline ESPSystemCoalescedState getCoalescedState(SystemControllerCoalescedState state) {
        switch (state) {
            case SYSTEM_CONTROLLER_COALESCED_STATE_HEATUP:
                return ESP_SYSTEM_COALESCED_STATE_HEATUP;
            case SYSTEM_CONTROLLER_COALESCED_STATE_TEMPS_NORMALIZING:
                return ESP_SYSTEM_COALESCED_STATE_TEMPS_NORMALIZING;
            case SYSTEM_CONTROLLER_COALESCED_STATE_WARM:
                return ESP_SYSTEM_COALESCED_STATE_WARM;
            case SYSTEM_CONTROLLER_COALESCED_STATE_SLEEPING:
                return ESP_SYSTEM_COALESCED_STATE_SLEEPING;
            case SYSTEM_CONTROLLER_COALESCED_STATE_BAILED:
                return ESP_SYSTEM_COALESCED_STATE_BAILED;
            case SYSTEM_CONTROLLER_COALESCED_STATE_FIRST_RUN:
                return ESP_SYSTEM_COALESCED_STATE_FIRST_RUN;
            case SYSTEM_CONTROLLER_COALESCED_STATE_UNDETERMINED:
            default:
                return ESP_SYSTEM_COALESCED_STATE_UNDETERMINED;
        }
    }
};


#endif //SMART_LCC_ESPFIRMWARE_H
