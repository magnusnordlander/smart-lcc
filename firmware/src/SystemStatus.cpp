//
// Created by Magnus Nordlander on 2021-07-02.
//

#include "SystemStatus.h"

SystemStatus::SystemStatus():
    lccPacket(LccParsedPacket()),
    controlBoardPacket(ControlBoardParsedPacket()),
    has_bailed(false) {

}
