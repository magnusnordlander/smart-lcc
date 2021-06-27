//
// Created by Magnus Nordlander on 2021-06-27.
//

#include "../platform.h"
#include "pico/stdlib.h" // NOLINT(modernize-deprecated-headers)
#include "hardware/gpio.h"
#include <pico/timeout_helper.h>
#include <cstring>

#define CB_UART uart0
#define CB_BAUD_RATE 9600
#define CB_TX_PIN 12
#define CB_RX_PIN 13

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

void init_platform() {
    stdio_init_all();

    // Initialize the UARTs
    uart_init(CB_UART, CB_BAUD_RATE);

    gpio_set_function(CB_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(CB_RX_PIN, GPIO_FUNC_UART);

    // The Bianca uses inverted UART
    gpio_set_outover(CB_TX_PIN, GPIO_OVERRIDE_INVERT);
    gpio_set_inover(CB_RX_PIN, GPIO_OVERRIDE_INVERT);
}

void loop_sleep() {
    sleep_ms(50);
}

void write_control_board_packet(LccRawPacket packet) {
    uart_write_blocking(CB_UART, (uint8_t *)&packet, sizeof(packet));
}

bool read_control_board_packet(ControlBoardRawPacket* packet) {
    uint8_t cb[18];

    if (!uart_read_blocking_timeout(CB_UART, cb, sizeof(cb), make_timeout_time_ms(200))) {
        return false;
    }

    memcpy(packet, cb, sizeof(cb));

    return true;
}