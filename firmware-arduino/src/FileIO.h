//
// Created by Magnus Nordlander on 2022-02-08.
//

#ifndef FIRMWARE_ARDUINO_FILEIO_H
#define FIRMWARE_ARDUINO_FILEIO_H

#include <FS.h>
#include <LittleFS.h>
#include "types.h"
#include "optional.hpp"
#include "utils/PicoQueue.h"

class FileIO {
public:
    explicit FileIO(FS *fileSystem, PicoQueue<SystemControllerCommand>* queue);

    bool saveSystemSettings(SettingStruct systemSettings, const char * filename, uint8_t version);
    bool saveWifiConfig(WiFiNINA_Configuration wifiConfig, const char * filename, uint8_t version);

    nonstd::optional<SettingStruct> readSystemSettings(const char * filename, uint8_t version);
    nonstd::optional<WiFiNINA_Configuration> readWifiConfig(const char * filename, uint8_t version);
private:
    FS* _fileSystem;
    PicoQueue<SystemControllerCommand>* _queue;
};


#endif //FIRMWARE_ARDUINO_FILEIO_H
