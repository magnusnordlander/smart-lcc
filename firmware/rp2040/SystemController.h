//
// Created by Magnus Nordlander on 2021-06-27.
//

#ifndef LCC_RELAY_SYSTEMCONTROLLER_H
#define LCC_RELAY_SYSTEMCONTROLLER_H


#include "PIDController.h"
#include "control_board_protocol.h"
#include "lcc_protocol.h"

class SystemController {
    void updateWithControlBoardPacket(ControlBoardParsedPacket packet);
    LccParsedPacket createLccPacket();

protected:
    PIDController* pidController;
};


#endif //LCC_RELAY_SYSTEMCONTROLLER_H
