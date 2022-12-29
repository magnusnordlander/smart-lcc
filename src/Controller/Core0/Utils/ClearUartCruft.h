//
// Created by Magnus Nordlander on 2022-12-28.
//

#ifndef SMART_LCC_CLEARUARTCRUFT_H
#define SMART_LCC_CLEARUARTCRUFT_H

#include <hardware/uart.h>
#include <hardware/timer.h>

inline void uart_clear_cruft(uart_inst_t *uart, bool wait = true) {
    if(uart_is_readable(uart)) {
        // There's cruft inside the UART. That's weird. Wait a little. Clear that out, and wait a little.
        if (wait) {
            busy_wait_ms(50);
        }

        while (uart_is_readable(uart)) {
            uart_get_hw(uart)->dr;
        }

        if (wait) {
            busy_wait_ms(50);
        }
    }
}

#endif //SMART_LCC_CLEARUARTCRUFT_H
