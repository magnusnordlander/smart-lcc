//
// Created by Magnus Nordlander on 2022-02-08.
//

#include <CRC32.h>
#include "FileIO.h"
#include <pico/multicore.h>

FileIO::FileIO(FS *fileSystem, PicoQueue<SystemControllerCommand> *queue) : _fileSystem(fileSystem), _queue(queue) {

}

bool FileIO::saveSystemSettings(SettingStruct systemSettings, const char *filename, uint8_t version) {
    File file = _fileSystem->open(filename, "w");

    if (file)
    {
        uint32_t calculatedChecksum = CRC32::calculate((uint8_t *) &systemSettings, sizeof(systemSettings));

        file.seek(0, SeekSet);
        file.write(version);
        file.write((uint8_t *) &systemSettings, sizeof(systemSettings));
        file.write((uint8_t *) &calculatedChecksum, sizeof(calculatedChecksum));
        file.close();

        return true;
    }
    else {
        return false;
    }
}

bool FileIO::saveWifiConfig(WiFiNINA_Configuration wifiConfig, const char *filename, uint8_t version) {
/*    SystemControllerCommand victimizeCommand = SystemControllerCommand{
        .type = COMMAND_VICTIMIZE
    };

    _queue->addBlocking(&victimizeCommand);

    multicore_lockout_start_blocking();
*/
    File file = _fileSystem->open(filename, "w");

    if (file)
    {
        uint32_t calculatedChecksum = CRC32::calculate((uint8_t *) &wifiConfig, sizeof(wifiConfig));

        DEBUGV("Calculated config checksum %d\n", calculatedChecksum);

        file.seek(0, SeekSet);
        file.write(version);
        file.write((uint8_t *) &wifiConfig, sizeof(wifiConfig));
        file.write((uint8_t *) &calculatedChecksum, sizeof(calculatedChecksum));
        file.close();

        DEBUGV("Writing network config\n");

        //multicore_lockout_end_blocking();

        return true;
    }
    else {
        DEBUGV("Couldn't network config file for writing\n");

        //multicore_lockout_end_blocking();

        return false;
    }
}

nonstd::optional<SettingStruct> FileIO::readSystemSettings(const char *filename, uint8_t version) {
    File file = _fileSystem->open(filename, "r");
    if (!file)
    {
        return nonstd::optional<SettingStruct>();
    }

    file.seek(0, SeekSet);

    uint8_t readVersion;
    file.read((uint8_t *) &readVersion, sizeof(readVersion));

    if (readVersion != version) {
        file.close();
        return nonstd::optional<SettingStruct>();
    }

    SettingStruct readSettings{};

    file.read((uint8_t *) &readSettings, sizeof(readSettings));

    uint32_t readChecksum;
    file.read((uint8_t *) &readChecksum, sizeof(readChecksum));

    file.close();

    uint32_t calculatedChecksum = CRC32::calculate((uint8_t *) &readSettings, sizeof(readSettings));

    if (calculatedChecksum != readChecksum) {
        return nonstd::optional<SettingStruct>();
    }


    return nonstd::optional<SettingStruct>(readSettings);
}

nonstd::optional<WiFiNINA_Configuration> FileIO::readWifiConfig(const char *filename, uint8_t version) {
    DEBUGV("Reading network config\n");
    WiFiNINA_Configuration readConfig;
    File file = _fileSystem->open(filename, "r");

    if (!file)
    {
        DEBUGV("No config file\n");
        return nonstd::optional<WiFiNINA_Configuration>();
    }

    file.seek(0, SeekSet);

    uint8_t readVersion;
    file.read((uint8_t *) &readVersion, sizeof(readVersion));

    if (readVersion != version) {
        file.close();
        DEBUGV("Wrong config version\n");
        return nonstd::optional<WiFiNINA_Configuration>();
    }

    file.read((uint8_t *) &readConfig, sizeof(readConfig));

    uint32_t readChecksum;
    file.read((uint8_t *) &readChecksum, sizeof(readChecksum));

    file.close();

    uint32_t calculatedChecksum = CRC32::calculate((uint8_t *) &readConfig, sizeof(readConfig));

    if (calculatedChecksum != readChecksum) {
        DEBUGV("Config checksum mismatch\n");
        DEBUGV("Calculated config checksum %d, read %d\n", calculatedChecksum, readChecksum);
        return nonstd::optional<WiFiNINA_Configuration>();
    }

    return nonstd::optional<WiFiNINA_Configuration>(readConfig);
}
