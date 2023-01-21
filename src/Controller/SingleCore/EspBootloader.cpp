//
// Created by Magnus Nordlander on 2022-12-27.
//

#include "EspBootloader.h"
#include <slip.h>
#include <cstring>
#include <hardware/uart.h>
#include "utils/ClearUartCruft.h"
#include "utils/UartReadBlockingTimeout.h"
#include <stub.h>
#include <bootloader_dio_40m.bin.h>
#include <partitions.bin.h>
#include <boot_app0.bin.h>
#include <firmware.bin.h>
#include <firmware_crc.h>

// Must be less than uint16_t
#define COMMAND_PACKAGE_LENGTH 255

// Nina W102 has a 16 Mbit flash according to data sheet
#define NINA_W102_FLASH_SIZE 2*1024*1024

bool EspBootloader::sync() {
    EspBootloaderResponseHeader response{};
    uint8_t data[36] = { 0x07, 0x07, 0x12, 0x20, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
    uint8_t responseData[255];

    size_t responseSize = sendCommand(0x08, reinterpret_cast<uint8_t *>(&data), sizeof(data), &response, responseData, sizeof(responseData), 0, 100);

    if (responseSize == 0) {
        return false;
    }

    return true;
}

esp_bootloader_error_t EspBootloader::readReg(uint32_t addr, uint32_t *value) {
    EspBootloaderResponseHeader response{};
    uint8_t responseData[COMMAND_PACKAGE_LENGTH];

    size_t responseSize = sendCommand(0x0a, reinterpret_cast<uint8_t *>(&addr), sizeof(addr), &response, responseData, sizeof(responseData));

    esp_bootloader_error_t err = checkResponse(responseSize, &response, responseData, sizeof(responseData));

    if (err != 0) {
        return err;
    }

    memcpy(value, &response.value, sizeof(uint32_t));

    return 0x00;
}

esp_bootloader_error_t EspBootloader::spiSetParamsNinaW102() {
    EspBootloaderResponseHeader response{};
    uint32_t data[6] = {
            0x00, // fl_id
            NINA_W102_FLASH_SIZE, // total_size
            64*1024, // block_size
            4*1024, // sector_size
            256, // page_size
            0xFFFF // status_mask
    };
    uint8_t responseData[COMMAND_PACKAGE_LENGTH];

    size_t responseSize = sendCommand(0x0b, reinterpret_cast<uint8_t *>(&data), sizeof(data), &response, responseData, sizeof(responseData));

    esp_bootloader_error_t err = checkResponse(responseSize, &response, responseData, sizeof(responseData));

    if (err != 0) {
        return err;
    }

    return 0x00;
}

esp_bootloader_error_t EspBootloader::spiAttach(bool stubRunning) {
    EspBootloaderResponseHeader response{};
    uint32_t *data;
    size_t dataLen;

    if (stubRunning) {
        dataLen = 1*sizeof(uint32_t);
        data = static_cast<uint32_t *>(malloc(dataLen));

        data[0] = 0x00; // normal flash
    } else {
        dataLen = 2*sizeof(uint32_t);
        data = static_cast<uint32_t *>(malloc(dataLen));

        data[0] = 0x00; // normal flash
        data[1] = 0x00; // redundant word
    }

    uint8_t responseData[COMMAND_PACKAGE_LENGTH];

    size_t responseSize = sendCommand(0x0d, reinterpret_cast<uint8_t *>(data), dataLen, &response, responseData, sizeof(responseData));

    free(data);

    esp_bootloader_error_t err = checkResponse(responseSize, &response, responseData, sizeof(responseData));

    if (err != 0) {
        return err;
    }

    return 0x00;
}

esp_bootloader_error_t EspBootloader::spiFlashMd5(uint32_t addr, uint32_t size, esp_bootloader_md5_t *md5) {
    EspBootloaderResponseHeader response{};
    uint32_t data[4] = {
            addr, // address
            size, // size
            0x00,
            0x00,
    };
    uint8_t responseData[COMMAND_PACKAGE_LENGTH];

    auto timeout = (uint16_t)(((float)size / 1024 / 1024) * 8000);
    if (timeout < 3000) {
        timeout = 3000;
    }

    size_t responseSize = sendCommand(0x13, reinterpret_cast<uint8_t *>(&data), sizeof(data), &response, responseData, sizeof(responseData), 0, timeout);

    esp_bootloader_error_t err = checkResponse(responseSize, &response, responseData, sizeof(responseData));

    if (err != 0) {
        return err;
    }

    if (responseSize == 36 || responseSize == 34) {
        char byte[3];
        byte[2] = '\0';

        for (int i = 0; i < 16; i++) {
            memcpy(&byte, responseData + 2*i, 2);
            md5->x[i] = strtol(byte, nullptr, 16);
        }
    } else if (responseSize == 18 || responseSize == 20) {
        memcpy(md5->x, responseData, 16);
    }

    return 0x00;
}

esp_bootloader_error_t EspBootloader::memBegin(uint32_t size, uint32_t blocks, uint32_t blocksize, uint32_t offset) {
    EspBootloaderResponseHeader response{};
    uint32_t data[4] = {
            size,
            blocks,
            blocksize,
            offset
    };
    uint8_t responseData[COMMAND_PACKAGE_LENGTH];

    size_t responseSize = sendCommand(0x05, reinterpret_cast<uint8_t *>(&data), sizeof(data), &response, responseData, sizeof(responseData));

    esp_bootloader_error_t err = checkResponse(responseSize, &response, responseData, sizeof(responseData));

    if (err != 0) {
        return err;
    }

    return 0x00;
}

esp_bootloader_error_t EspBootloader::memData(uint32_t seq, uint8_t *uploadData, size_t actualLen) {
    EspBootloaderResponseHeader response{};
    uint32_t data[4] = {
            actualLen,
            seq,
            0,
            0
    };
    uint8_t responseData[COMMAND_PACKAGE_LENGTH];

    size_t fullDataLen = actualLen + sizeof(data);

    auto *fullData = static_cast<uint8_t*>(malloc(fullDataLen));
//    memset(fullData, 0xFF, fullDataLen);
    memcpy(fullData, data, sizeof(data));
    memcpy(fullData + sizeof(data), uploadData, actualLen);

    size_t responseSize = sendCommand(0x07, fullData, fullDataLen, &response, responseData, sizeof(responseData), checksum(uploadData, actualLen));

    free(fullData);

    esp_bootloader_error_t err = checkResponse(responseSize, &response, responseData, sizeof(responseData));

    if (err != 0) {
        return err;
    }

    return 0x00;
}

esp_bootloader_error_t EspBootloader::memEnd(uint32_t entrypoint) {
    EspBootloaderResponseHeader response{};
    uint32_t data[2] = {
            entrypoint == 0 ? (uint32_t)1 : (uint32_t)0,
            entrypoint
    };
    uint8_t responseData[COMMAND_PACKAGE_LENGTH];

    size_t responseSize = sendCommand(0x06, reinterpret_cast<uint8_t *>(&data), sizeof(data), &response, responseData, sizeof(responseData));

    esp_bootloader_error_t err = checkResponse(responseSize, &response, responseData, sizeof(responseData));

    if (err != 0) {
        return err;
    }

    return 0x00;
}

esp_bootloader_error_t EspBootloader::memBeginAndData(uint32_t addr, const uint8_t *data, size_t len) {
    esp_bootloader_error_t err;
    uint32_t blockSize = 0x1800;

    uint32_t numBlocks = (len + blockSize - 1) / blockSize;

    err = memBegin(len, numBlocks, blockSize, addr);

    if (err != 0x00) {
        return err;
    }

    for (unsigned int seq = 0; seq < numBlocks; seq++) {
        uint32_t fromOffs = seq * blockSize;
        size_t actualLen = len - fromOffs;
        if (actualLen > blockSize) {
            actualLen = blockSize;
        }

        err = memData(seq, const_cast<uint8_t *>(data + fromOffs), actualLen);

        if (err != 0x00) {
            return err;
        }
    }

    return 0x00;
}

esp_bootloader_error_t EspBootloader::uploadStub() {
    esp_bootloader_error_t err;

    err = memBeginAndData(stub_text_start, stub_text, sizeof(stub_text));

    if (err != 0x00) {
        return err;
    }

    err = memBeginAndData(stub_data_start, stub_data, sizeof(stub_data));

    if (err != 0x00) {
        return err;
    }

    err = memEnd(stub_entry);

    if (err != 0x00) {
        return err;
    }

/*    uint8_t expected[6] = {
            0xc0,
            'O',
            'H',
            'A',
            'I',
            0xc0
    };
    uint8_t ohai[6];

    bool read = uart_read_blocking_timeout(uart, ohai, sizeof(ohai), make_timeout_time_ms(3000));

    if (!read) {
        return 0x01;
    }

    return memcmp(ohai, expected, sizeof(ohai)) == 0 ? 0x00 : 0x01;*/

    return 0x00;
}

esp_bootloader_error_t EspBootloader::uploadFirmware(const std::function<void(uint16_t, uint16_t)>& progressCallback) {
    esp_bootloader_error_t err;

    err = flashBeginAndData(0x1000, bootloader_dio_40m_bin, sizeof(bootloader_dio_40m_bin), progressCallback);

    if (err != 0x00) {
        return err;
    }

    err = flashBeginAndData(0x8000, partitions_bin, sizeof(partitions_bin), progressCallback);

    if (err != 0x00) {
        return err;
    }

    err = flashBeginAndData(0xe000, boot_app0_bin, sizeof(boot_app0_bin), progressCallback);

    if (err != 0x00) {
        return err;
    }

    err = flashBeginAndData(0x10000, firmware_bin, sizeof(firmware_bin), progressCallback);

    if (err != 0x00) {
        return err;
    }

    err = flashBeginAndData(0x1E0000, reinterpret_cast<const uint8_t *>(&firmware_crc), sizeof(firmware_crc), progressCallback);

    if (err != 0x00) {
        return err;
    }

    err = flashEnd(false);

    return err;
}

esp_bootloader_error_t EspBootloader::flashBegin(uint32_t size, uint32_t blocks, uint32_t blocksize, uint32_t offset) {
    EspBootloaderResponseHeader response{};
    uint32_t data[4] = {
            size,
            blocks,
            blocksize,
            offset
    };
    uint8_t responseData[COMMAND_PACKAGE_LENGTH];

    size_t responseSize = sendCommand(0x02, reinterpret_cast<uint8_t *>(&data), sizeof(data), &response, responseData, sizeof(responseData));

    esp_bootloader_error_t err = checkResponse(responseSize, &response, responseData, sizeof(responseData));

    if (err != 0) {
        return err;
    }

    return 0x00;
}

esp_bootloader_error_t EspBootloader::flashData(uint32_t seq, uint8_t *uploadData, size_t actualLen, size_t padLen) {
    EspBootloaderResponseHeader response{};
    uint32_t data[4] = {
            padLen,
            seq,
            0,
            0
    };
    uint8_t responseData[COMMAND_PACKAGE_LENGTH];

    size_t fullDataLen = padLen + sizeof(data);

    auto *fullData = static_cast<uint8_t*>(malloc(fullDataLen));
    memset(fullData, 0xFF, fullDataLen);
    memcpy(fullData, data, sizeof(data));
    memcpy(fullData + sizeof(data), uploadData, actualLen);

    size_t responseSize = sendCommand(0x03, fullData, fullDataLen, &response, responseData, sizeof(responseData), checksum(uploadData, actualLen));

    free(fullData);

    esp_bootloader_error_t err = checkResponse(responseSize, &response, responseData, sizeof(responseData));

    if (err != 0) {
        return err;
    }

    return 0x00;
}

esp_bootloader_error_t EspBootloader::flashEnd(bool reboot) {
    EspBootloaderResponseHeader response{};
    uint32_t data[1] = {
            reboot ? (uint32_t)0 : (uint32_t)1,
    };
    uint8_t responseData[COMMAND_PACKAGE_LENGTH];

    size_t responseSize = sendCommand(0x04, reinterpret_cast<uint8_t *>(&data), sizeof(data), &response, responseData, sizeof(responseData));

    esp_bootloader_error_t err = checkResponse(responseSize, &response, responseData, sizeof(responseData));

    if (err != 0) {
        return err;
    }

    return 0x00;
}

esp_bootloader_error_t EspBootloader::flashBeginAndData(uint32_t addr, const uint8_t *data, size_t len, const std::function<void(uint16_t, uint16_t)>& progressCallback) {
    esp_bootloader_error_t err;
    uint32_t blockSize = 0x1800;

    uint32_t numBlocks = (len + blockSize - 1) / blockSize;

    err = flashBegin(len, numBlocks, blockSize, addr);

    if (err != 0x00) {
        return err;
    }

    for (unsigned int seq = 0; seq < numBlocks; seq++) {
        uint32_t fromOffs = seq * blockSize;
        size_t actualLen = len - fromOffs;
        if (actualLen > blockSize) {
            actualLen = blockSize;
        }

        progressCallback(seq, numBlocks);

        err = flashData(seq, const_cast<uint8_t *>(data + fromOffs), actualLen, blockSize);

        if (err != 0x00) {
            return err;
        }
    }

    return 0x00;
}

esp_bootloader_error_t EspBootloader::checkResponse(size_t responseSize, EspBootloaderResponseHeader *responseHeader,
                                                    uint8_t *responseData, size_t responseDataLen) {
    if (responseSize == 0) {
        return 0x01;
    }

    if (responseSize < 2) {
        return 0x02;
    }

    if (responseSize > responseDataLen) {
        return 0x03;
    }

    if (responseData[responseHeader->size - 2] == 0x01) {
        return responseData[responseHeader->size - 1];
    }

    if (responseData[responseHeader->size - 2] == 0x00) {
        return 0x00;
    }

    return 0x04;
}

size_t EspBootloader::sendCommand(uint8_t command, uint8_t *data, size_t dataLen, EspBootloaderResponseHeader *responseHeader, uint8_t *responseData, size_t responseDataLen, uint32_t checksum, uint16_t timeoutMs) {
    EspBootloaderCommandHeader header{
        .direction = 0x00,
        .command = command,
        .size = static_cast<uint16_t>(dataLen),
        .checksum = checksum,
    };

    size_t packetSize = sizeof(EspBootloaderCommandHeader) + dataLen;
    auto *pkt = static_cast<uint8_t *>(malloc(packetSize));

    memcpy(pkt, &header, sizeof(EspBootloaderCommandHeader));
    if (dataLen > 0) {
        memcpy(pkt + sizeof(EspBootloaderCommandHeader), data, dataLen);
    }

    size_t frameSize = SLIP::getFrameLength(pkt, packetSize);
    auto *frame = static_cast<uint8_t *>(malloc(frameSize));

    size_t retFrameSize = SLIP::encode(frame, pkt, packetSize);

    free(pkt);

    uart_clear_cruft(uart, false);
    uart_write_blocking(uart, frame, retFrameSize);

    free(frame);

    uint8_t chunk[COMMAND_PACKAGE_LENGTH];
    uint16_t i = 0;

    if (!uart_is_readable_within_us(uart, timeoutMs*1000)) {
        return 0;
    }

    chunk[i++] = uart_getc(uart);

    while(uart_is_readable_within_us(uart, timeoutMs*1000) && i < COMMAND_PACKAGE_LENGTH) {
        char c = uart_getc(uart);
        chunk[i++] = c;

        if (c == 0xc0) {
            break;
        }
    }

    uint8_t returnedFrame[COMMAND_PACKAGE_LENGTH];
    size_t retFrameLen = SLIP::getFrame(reinterpret_cast<unsigned char *>(&returnedFrame), reinterpret_cast<unsigned char *>(&chunk), i);

    if (retFrameLen == 0) {
        return 0;
    }

    uint8_t buf[COMMAND_PACKAGE_LENGTH];
    size_t bufLen = SLIP::decode(reinterpret_cast<unsigned char *>(&buf), reinterpret_cast<unsigned char *>(&returnedFrame), retFrameLen);

    if (bufLen < sizeof(EspBootloaderResponseHeader)) {
        return 0;
    }


    memcpy(responseHeader, &buf, sizeof(EspBootloaderResponseHeader));

    if (!validateHeader(responseHeader, command)) {
        return 0;
    }

    if (responseHeader->size <= responseDataLen && (responseHeader->size + sizeof(EspBootloaderResponseHeader)) == bufLen) {
        memcpy(responseData, buf + sizeof(EspBootloaderResponseHeader), responseHeader->size);

        return responseHeader->size;
    }

    return 0;
}

bool EspBootloader::validateHeader(EspBootloaderResponseHeader *responseHeader, uint8_t expectedCommand) {
    if (responseHeader->direction != 0x01) {
        return false;
    }

    if (responseHeader->command != expectedCommand) {
        return false;
    }

    return true;
}

EspBootloader::EspBootloader(uart_inst_t *uart) : uart(uart) {}

uint32_t EspBootloader::checksum(const uint8_t *data, size_t len) {
    uint32_t checksum = 0xEF;

    for (unsigned int i = 0; i < len; i++) {
        checksum ^= data[i];
    }

    return checksum;
}

