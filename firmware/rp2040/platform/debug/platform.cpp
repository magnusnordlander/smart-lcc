//
// Created by Magnus Nordlander on 2021-06-27.
//

#include <unistd.h>
#include <cstring>
#include "../platform.h"
#include "../../lcc_protocol.h"
#include "emulator/BiancaControlBoard.h"
#include <ctime>
#include <cmath>
#include <cstdio>

BiancaControlBoard controlBoard;
uint64_t ltime = 0;

void init_platform()
{
    controlBoard = BiancaControlBoard();
}

void loop_sleep()
{
    usleep(500);
}

void write_control_board_packet(LccRawPacket packet) {
    LccParsedPacket parsed = convert_lcc_raw_to_parsed(packet);

    ltime += 50;
    printf("Updating CB. Time passed: %.02f s Pump: %s Solenoid: %s BrewSSR: %s ServiceSSR: %s\n",
           (float)ltime/1000.0f,
           parsed.pump_on ? "Y" : "N",
           parsed.service_boiler_solenoid_open ? "Y" : "N",
           parsed.brew_boiler_ssr_on ? "Y" : "N",
           parsed.service_boiler_ssr_on ? "Y" : "N"
           );

    controlBoard.update(ltime, parsed.pump_on, parsed.service_boiler_solenoid_open, parsed.brew_boiler_ssr_on, parsed.service_boiler_ssr_on);
}

bool read_control_board_packet(ControlBoardRawPacket* packet) {
    controlBoard.copyPacket((uint8_t*)packet);
/*    uint8_t p[18] = {0x81, 0x00, 0x00, 0x5D, 0x7F, 0x00, 0x79, 0x7F, 0x02, 0x5D, 0x7F, 0x03, 0x2B, 0x00, 0x02, 0x05, 0x7F, 0x67};
    memcpy(packet, p, sizeof(p));*/

    return true;
}