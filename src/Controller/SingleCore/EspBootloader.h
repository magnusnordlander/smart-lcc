//
// Created by Magnus Nordlander on 2022-12-27.
//

#ifndef SMART_LCC_ESPBOOTLOADER_H
#define SMART_LCC_ESPBOOTLOADER_H


#include <cstdint>
#include <cstdlib>
#include <hardware/uart.h>

typedef uint8_t esp_bootloader_error_t;
typedef struct esp_bootloader_md5_t { uint8_t x[16]; } esp_bootloader_md5_t;

struct EspBootloaderCommandHeader {
    uint8_t direction;
    uint8_t command;
    uint16_t size;
    uint32_t checksum;
};

struct EspBootloaderResponseHeader {
    uint8_t direction;
    uint8_t command;
    uint16_t size;
    uint32_t value;
};

class EspBootloader {
public:
    explicit EspBootloader(uart_inst_t *uart);

    bool sync();
    esp_bootloader_error_t readReg(uint32_t addr, uint32_t *value);
    esp_bootloader_error_t spiSetParamsNinaW102();
    esp_bootloader_error_t spiAttach();
    esp_bootloader_error_t spiFlashMd5(uint32_t addr, uint32_t size, esp_bootloader_md5_t *md5);
    esp_bootloader_error_t memBegin(uint32_t size, uint32_t blocks, uint32_t blocksize, uint32_t offset);
    esp_bootloader_error_t memData(uint32_t seq, uint8_t *data, size_t actualLen, size_t padLen);
    esp_bootloader_error_t memEnd(uint32_t entrypoint);
    esp_bootloader_error_t uploadStub();
private:
    uart_inst_t *uart;

    esp_bootloader_error_t memBeginAndData(uint32_t addr, const uint8_t *data, size_t len);

    size_t sendCommand(uint8_t command, uint8_t *data, size_t dataLen, EspBootloaderResponseHeader *responseHeader, uint8_t *responseData, size_t responseDataLen, uint32_t checksum = 0, uint16_t timeoutMs = 3000);
    static bool validateHeader(EspBootloaderResponseHeader *responseHeader, uint8_t expectedCommand);
    static esp_bootloader_error_t checkResponse(size_t responseSize, EspBootloaderResponseHeader *responseHeader, uint8_t *responseData, size_t responseDataLen);

    static uint32_t checksum(const uint8_t *data, size_t len);
};


#endif //SMART_LCC_ESPBOOTLOADER_H
