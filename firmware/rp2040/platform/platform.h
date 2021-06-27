//
// Created by Magnus Nordlander on 2021-06-27.
//

#ifndef LCC_RELAY_PLATFORM_H
#define LCC_RELAY_PLATFORM_H

#include "../lcc_protocol.h"
#include "../control_board_protocol.h"

void init_platform();
void loop_sleep();
void write_control_board_packet(LccRawPacket packet);
bool read_control_board_packet(ControlBoardRawPacket* packet);

#endif //LCC_RELAY_PLATFORM_H
