//
// Created by Magnus Nordlander on 2021-07-06.
//

#ifndef FIRMWARE_SYSTEMCONTROLLER_H
#define FIRMWARE_SYSTEMCONTROLLER_H


#include <SystemStatus.h>

class SystemController {
public:
    explicit SystemController(SystemStatus *status);

    [[noreturn]] void run();

private:
    LccParsedPacket handleControlBoardPacket(ControlBoardParsedPacket packet);

    SystemStatus* status;
};


#endif //FIRMWARE_SYSTEMCONTROLLER_H
