//
// Created by Magnus Nordlander on 2023-01-04.
//

#include <cstdlib>
#include "EspFirmware.h"
#include "pico/time.h"
#include "hardware/regs/rosc.h"
#include "hardware/regs/addressmap.h"
#include "utils/crc32.h"

jnk0le::Ringbuffer<uint8_t, 1024> EspFirmware::ringbuffer = {};
uart_inst_t* EspFirmware::interruptedUart = nullptr;

void EspFirmware::initInterrupts(uart_inst_t *uart) {
    EspFirmware::interruptedUart = uart;

    uart_set_fifo_enabled(uart, false);

    int UART_IRQ = uart == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, EspFirmware::onUartRx);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(uart, true, false);
}

void EspFirmware::onUartRx() {
    if (EspFirmware::interruptedUart == nullptr) {
        return;
    }

    while (uart_is_readable(EspFirmware::interruptedUart)) {
        uint8_t ch = uart_getc(EspFirmware::interruptedUart);
        ringbuffer.insert( ch);
    }
}

bool EspFirmware::readFromRingBufferBlockingWithTimeout(uint8_t *dst, size_t len, absolute_time_t timeout_time) {
    timeout_state_t ts;
    check_timeout_fn timeout_check = init_single_timeout_until(&ts, timeout_time);

    for (size_t i = 0; i < len; ++i) {
        while (ringbuffer.readAvailable() < len) {
            if (timeout_check(&ts)) {
                return false;
            }

            tight_loop_contents();
        }
    }

    size_t readLen = ringbuffer.readBuff(dst, len);

    return readLen == len;
}

EspFirmware::EspFirmware(uart_inst_t *uart, PicoQueue<SystemControllerCommand> *commandQueue) : uart(uart), commandQueue(commandQueue) {}

uint32_t rnd(void){
    int k, random=0;
    volatile uint32_t *rnd_reg=(uint32_t *)(ROSC_BASE + ROSC_RANDOMBIT_OFFSET);

    for(k=0;k<32;k++){

        random = random << 1;
        random=random + (0x00000001 & (*rnd_reg));

    }
    return random;
}


bool EspFirmware::pingBlocking() {
    ESPMessageHeader pingHeader{
            .direction = ESP_DIRECTION_RP2040_TO_ESP32,
            .id = 1,
            .responseTo = 0,
            .type = ESP_MESSAGE_PING,
            .error = ESP_ERROR_NONE,
            .length = sizeof(ESPPingMessage),
    };

    ESPPingMessage pingMessage{
            .version = 0x0001,
    };

    ringbuffer.consumerClear();

    uart_write_blocking(uart1, reinterpret_cast<const uint8_t *>(&pingHeader), sizeof(pingHeader));
    uart_write_blocking(uart1, reinterpret_cast<const uint8_t *>(&pingMessage), sizeof(pingMessage));

    ESPMessageHeader replyHeader{};

    bool success = readFromRingBufferBlockingWithTimeout(reinterpret_cast<uint8_t *>(&replyHeader), sizeof(ESPMessageHeader), make_timeout_time_ms(100));

    if (success && replyHeader.length > 0) {
        auto *response = static_cast<uint8_t *>(malloc(replyHeader.length));
        success = readFromRingBufferBlockingWithTimeout(response, replyHeader.length, make_timeout_time_ms(50));

        if (success) {
            if (replyHeader.type != ESP_MESSAGE_PONG || replyHeader.responseTo != 1 || replyHeader.error != ESP_ERROR_NONE) {
                free(response);

                return false;
            } else {
                auto *replyMessage = reinterpret_cast<ESPPongMessage *>(response);
                if (replyMessage->version != 0x0001) {
                    free(response);
                    return false;
                }
            }
        }

        free(response);
        return true;
    }

    return false;
}

bool EspFirmware::sendStatus(SystemControllerStatusMessage *systemControllerStatusMessage) {
    ESPMessageHeader statusHeader{
            .direction = ESP_DIRECTION_RP2040_TO_ESP32,
            .id = rnd(),
            .responseTo = 0,
            .type = ESP_MESSAGE_SYSTEM_STATUS,
            .error = ESP_ERROR_NONE,
            .length = sizeof(ESPSystemStatusMessage),
    };

    ESPSystemStatusMessage statusMessage{
            .brewBoilerTemperatureMilliCelsius = static_cast<uint32_t>(systemControllerStatusMessage->offsetBrewTemperature*1000),
            .brewBoilerSetPointMilliCelsius = static_cast<uint32_t>(systemControllerStatusMessage->offsetBrewSetPoint*1000),
            .serviceBoilerTemperatureMilliCelsius = static_cast<uint32_t>(systemControllerStatusMessage->serviceTemperature*1000),
            .serviceBoilerSetPointMilliCelsius = static_cast<uint32_t>(systemControllerStatusMessage->serviceSetPoint*1000),
            .brewTemperatureOffsetMilliCelsius = static_cast<int32_t>(systemControllerStatusMessage->brewTemperatureOffset*1000),
            .autoSleepAfter = 0,
            .currentlyBrewing = systemControllerStatusMessage->currentlyBrewing,
            .currentlyFillingServiceBoiler = systemControllerStatusMessage->currentlyFillingServiceBoiler,
            .ecoMode = systemControllerStatusMessage->ecoMode,
            .sleepMode = systemControllerStatusMessage->sleepMode,
            .waterTankLow = systemControllerStatusMessage->waterTankLow,
            .plannedAutoSleepInSeconds = 0,
            .rp2040TemperatureMilliCelsius = 0,
    };

    ringbuffer.consumerClear();

    uart_write_blocking(uart, reinterpret_cast<const uint8_t *>(&statusHeader), sizeof(statusHeader));
    uart_write_blocking(uart, reinterpret_cast<const uint8_t *>(&statusMessage), sizeof(statusMessage));

    return waitForAck(statusHeader.id);
}

bool EspFirmware::waitForAck(uint32_t id) {
    ESPMessageHeader replyHeader{};

    bool success = readFromRingBufferBlockingWithTimeout(reinterpret_cast<uint8_t *>(&replyHeader), sizeof(ESPMessageHeader), make_timeout_time_ms(100));

    if (success) {
        if (replyHeader.length > 0) {
            auto *response = static_cast<uint8_t *>(malloc(replyHeader.length));
            success = readFromRingBufferBlockingWithTimeout(response, replyHeader.length, make_timeout_time_ms(50));

            free(response);

            // Acks are zero length
            return false;
        }

        return replyHeader.type == ESP_MESSAGE_ACK && replyHeader.responseTo == id && replyHeader.error == ESP_ERROR_NONE;
    }

    return false;
}

void EspFirmware::loop() {
    if (!ringbuffer.isEmpty()) {
        gpio_put(21u, true);
        ESPMessageHeader header{};
        bool success = readFromRingBufferBlockingWithTimeout(reinterpret_cast<uint8_t *>(&header), sizeof(ESPMessageHeader), make_timeout_time_ms(10));

        if (success) {
            if (header.direction == ESP_DIRECTION_ESP32_TO_RP2040) {
                switch(header.type) {
                    case ESP_MESSAGE_SYSTEM_COMMAND:
                        return handleCommand(&header);
                    default:
                        ringbuffer.consumerClear();
                        return;
                }
            }
        }

        ringbuffer.consumerClear();
    } else {
        gpio_put(21u, false);
    }
}

void EspFirmware::handleCommand(ESPMessageHeader *header) {
    if (header->length == sizeof(ESPSystemCommandMessage)) {
        ESPSystemCommandMessage message{};
        bool success = readFromRingBufferBlockingWithTimeout(reinterpret_cast<uint8_t *>(&message), sizeof(ESPSystemCommandMessage), make_timeout_time_ms(50));

        if (success) {
            crc32_t crc;
            crc32(&message.payload, sizeof(ESPSystemCommandPayload), &crc);

            if (crc == message.checksum) {
                SystemControllerCommandType type;
                switch (message.payload.type) {
                    case ESP_SYSTEM_COMMAND_SET_SLEEP_MODE:
                        type = COMMAND_SET_SLEEP_MODE;
                        break;
                    case ESP_SYSTEM_COMMAND_SET_BREW_SET_POINT:
                        type = COMMAND_SET_BREW_SET_POINT;
                        break;
                    case ESP_SYSTEM_COMMAND_SET_BREW_PID_PARAMETERS:
                        type = COMMAND_SET_BREW_PID_PARAMETERS;
                        break;
                    case ESP_SYSTEM_COMMAND_SET_SERVICE_SET_POINT:
                        type = COMMAND_SET_SERVICE_SET_POINT;
                        break;
                    case ESP_SYSTEM_COMMAND_SET_SERVICE_PID_PARAMETERS:
                        type = COMMAND_SET_SERVICE_PID_PARAMETERS;
                        break;
                    case ESP_SYSTEM_COMMAND_SET_ECO_MODE:
                        type = COMMAND_SET_ECO_MODE;
                        break;
                }

                SystemControllerCommand command{
                    .type = type,
                    .float1 = message.payload.float1,
                    .float2 = message.payload.float2,
                    .float3 = message.payload.float3,
                    .float4 = message.payload.float4,
                    .float5 = message.payload.float5,
                    .bool1 = message.payload.bool1,
                };

                commandQueue->tryAdd(&command);

                sendAck(header->id);
            } else {
                sendNack(header->id, ESP_ERROR_INVALID_CHECKSUM);
            }
        } else {
            sendNack(header->id, ESP_ERROR_INCOMPLETE_DATA);
        }
    } else {
        sendNack(header->id, ESP_ERROR_UNEXPECTED_MESSAGE_LENGTH);
    }
}

void EspFirmware::sendAck(uint32_t messageId) {
    ESPMessageHeader ackHeader{
            .direction = ESP_DIRECTION_RP2040_TO_ESP32,
            .id = rnd(),
            .responseTo = messageId,
            .type = ESP_MESSAGE_ACK,
            .error = ESP_ERROR_NONE,
            .length = 0,
    };

    uart_write_blocking(uart, reinterpret_cast<const uint8_t *>(&ackHeader), sizeof(ackHeader));
}

void EspFirmware::sendNack(uint32_t messageId, ESPError error) {
    ESPMessageHeader ackHeader{
            .direction = ESP_DIRECTION_RP2040_TO_ESP32,
            .id = rnd(),
            .responseTo = messageId,
            .type = ESP_MESSAGE_NACK,
            .error = error,
            .length = 0,
    };

    uart_write_blocking(uart, reinterpret_cast<const uint8_t *>(&ackHeader), sizeof(ackHeader));
}
