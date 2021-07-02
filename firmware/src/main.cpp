#include <Arduino.h>
#include <cstdio>
#include "platform/platform.h"
#include "control_board_protocol.h"
#include "utils/hex_format.h"
#include "SystemController.h"

#define LINE "\n"
// #define LINE "\x1b[1000D"

SystemController controller = SystemController();

void loop() {
    LccRawPacket lccPacket = convert_lcc_parsed_to_raw(controller.createLccPacket());
    write_control_board_packet(lccPacket);

    ControlBoardRawPacket control_board_packet;
    if (!read_control_board_packet(&control_board_packet)) {
        printf("Can't read Control Board \n");

        return;
    }

    if (uint16_t validation = validate_raw_packet(control_board_packet)) {
        printf("Control Board packet invalid: %hx \n", validation);
        printhex((uint8_t*)&control_board_packet, sizeof(control_board_packet));
        printf("\n");
    }

    ControlBoardParsedPacket parsed_packet = convert_raw_packet(control_board_packet);
    controller.updateWithControlBoardPacket(parsed_packet);

    printf(
            "Brew temp: %.02f Service temp: %.02f Brew switch: %s Service boiler low: %s Water tank low %s" LINE,
            parsed_packet.brew_boiler_temperature,
            parsed_packet.service_boiler_temperature,
            parsed_packet.brew_switch ? "Y" : "N",
            parsed_packet.service_boiler_low ? "Y" : "N",
            parsed_packet.water_tank_empty ? "Y" : "N"
    );

#ifdef TEST_RIG
    if (parsed_packet.service_boiler_temperature >= 125) {
        exit(0);
    }
#endif

    loop_sleep();
}

void setup() {
    init_platform();
}