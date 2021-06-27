//
// Created by Magnus Nordlander on 2021-06-27.
//

#include <pico/timeout_helper.h>
#include "uart_extras.h"

int uart_read_blocking_timeout(uart_inst_t *uart, uint8_t *dst, size_t len, absolute_time_t until) {
    timeout_state_t ts;
    check_timeout_fn timeout_check = init_single_timeout_until(&ts, until);
    bool timeout = false;

    for (size_t i = 0; i < len; ++i) {
        do {
            timeout = timeout_check(&ts);
            tight_loop_contents();
        } while (!timeout && !uart_is_readable(uart));

        if (timeout) {
            return 0;
        }

        *dst++ = (uint8_t) uart_get_hw(uart)->dr;
    }

    return 1;
}