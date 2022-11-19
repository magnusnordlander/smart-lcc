//
// Created by Magnus Nordlander on 2022-11-18.
//

#ifndef SMART_LCC_U8G2FUNCTIONS_H
#define SMART_LCC_U8G2FUNCTIONS_H

#include "pico/stdlib.h"
#include "u8g2.h"

#pragma once
#ifdef __cplusplus
extern "C"
{
#endif


uint8_t u8x8_byte_pico_hw_spi(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr);
uint8_t u8x8_gpio_and_delay_template(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, __attribute__((unused)) void *arg_ptr);

#ifdef __cplusplus
}
#endif

#endif //SMART_LCC_U8G2FUNCTIONS_H
