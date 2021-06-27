//
// Created by Magnus Nordlander on 2021-06-27.
//

#ifndef LCC_RELAY_UART_EXTRAS_H
#define LCC_RELAY_UART_EXTRAS_H

#include <pico.h>
#include <hardware/uart.h>

int uart_read_blocking_timeout(uart_inst_t *uart, uint8_t *dst, size_t len, absolute_time_t until);

#endif //LCC_RELAY_UART_EXTRAS_H
