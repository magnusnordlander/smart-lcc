//
// Created by Magnus Nordlander on 2022-12-24.
//

#ifndef SMART_LCC_UARTREADBLOCKINGTIMEOUT_H
#define SMART_LCC_UARTREADBLOCKINGTIMEOUT_H

#include <hardware/uart.h>
#include <hardware/gpio.h>
#include <pico/timeout_helper.h>

static inline bool uart_read_blocking_timeout(uart_inst_t *uart, uint8_t *dst, size_t len, absolute_time_t timeout_time) {
    timeout_state_t ts;
    check_timeout_fn timeout_check = init_single_timeout_until(&ts, timeout_time);

    for (size_t i = 0; i < len; ++i) {
        while (!uart_is_readable(uart)) {
            if (timeout_check(&ts)) {
                return false;
            }

            tight_loop_contents();
        }
        *dst++ = uart_get_hw(uart)->dr;
    }

    return true;
}

#endif //SMART_LCC_UARTREADBLOCKINGTIMEOUT_H
