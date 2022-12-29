//
// Created by Magnus Nordlander on 2022-01-31.
//

#include "SafePacketSender.h"

SafePacketSender::SafePacketSender(uart_inst_t *_uart): uart(_uart) {
    safeLccRawPacket = create_safe_packet();
    nextSend = make_timeout_time_ms(1000);
}

void SafePacketSender::loop() {
    if (absolute_time_diff_us(nextSend, get_absolute_time()) > 0) {
        uart_write_blocking(uart, (uint8_t *)&safeLccRawPacket, sizeof(safeLccRawPacket));
        nextSend = make_timeout_time_ms(1000);
    }
}
