//
// Created by Magnus Nordlander on 2022-11-18.
//

#include "u8g2functions.h"

#include "pins.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "u8g2.h"

#define SPI_PORT spi0
#define PIN_MISO OLED_MISO
#define PIN_CS OLED_CS
#define PIN_SCK OLED_SCK
#define PIN_MOSI OLED_MOSI
#define PIN_DC OLED_DC
#define PIN_RST OLED_RST
#define SPI_SPEED 8000000UL

uint8_t u8x8_byte_pico_hw_spi(u8x8_t *u8x8, uint8_t msg,
                              uint8_t arg_int, void *arg_ptr) {
    uint8_t *data;
    switch (msg) {
        case U8X8_MSG_BYTE_SEND:
            data = (uint8_t *)arg_ptr;
            // while( arg_int > 0 ) {
            // printf("U8X8_MSG_BYTE_SEND %d \n", (int)data[0]);
            spi_write_blocking(SPI_PORT, data, arg_int);
            // data++;
            // arg_int--;
            // }
            break;
        case U8X8_MSG_BYTE_INIT:
            // printf("U8X8_MSG_BYTE_INIT \n");
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
            // if(spiInit){
            //   spi_deinit(SPI_PORT);
            // }
            // spi_init(SPI_PORT, SPI_SPEED);
            // spiInit = true;
            break;
        case U8X8_MSG_BYTE_SET_DC:
            // printf("U8X8_MSG_BYTE_SET_DC %d \n", arg_int);
            u8x8_gpio_SetDC(u8x8, arg_int);
            break;
        case U8X8_MSG_BYTE_START_TRANSFER:
            // printf("U8X8_MSG_BYTE_START_TRANSFER %d \n", arg_int);

            // if(spiInit){
            //   spi_deinit(SPI_PORT);
            // }
            // spi_init(SPI_PORT, SPI_SPEED);
            // spiInit = true;

            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO,
                                    u8x8->display_info->post_chip_enable_wait_ns, NULL);
            break;
        case U8X8_MSG_BYTE_END_TRANSFER:
            // printf("U8X8_MSG_BYTE_END_TRANSFER %d \n", arg_int);
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO,
                                    u8x8->display_info->pre_chip_disable_wait_ns, NULL);
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
            // spi_deinit(SPI_PORT);
            // spiInit = false;
            break;
        default:
            return 0;
    }
    return 1;
}

uint8_t u8x8_gpio_and_delay_template(u8x8_t *u8x8, uint8_t msg,
                                     uint8_t arg_int,
                                     __attribute__((unused)) void *arg_ptr) {
    switch (msg) {
        case U8X8_MSG_GPIO_AND_DELAY_INIT: // called once during init phase of
            // u8g2/u8x8
            // printf("U8X8_MSG_GPIO_AND_DELAY_INIT\n");
            spi_init(SPI_PORT, SPI_SPEED);
            gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
            gpio_set_function(PIN_CS, GPIO_FUNC_SIO);
            gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
            gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
            // spiInit = true;
            // Chip select is active-low, so we'll initialise it to a driven-high state
            gpio_init(PIN_DC);
            gpio_init(PIN_CS);
            gpio_init(PIN_RST);
            gpio_set_dir(PIN_DC, GPIO_OUT);
            gpio_set_dir(PIN_CS, GPIO_OUT);
            gpio_set_dir(PIN_RST, GPIO_OUT);
            gpio_put(PIN_CS, 1);
            gpio_put(PIN_DC, 0);
            gpio_put(PIN_RST, 0);
            break;                  // can be used to setup pins
        case U8X8_MSG_DELAY_NANO: // delay arg_int * 1 nano second
            // printf("U8X8_MSG_DELAY_NANO %d\n", arg_int);
            sleep_us(1000 * arg_int);
            break;
        case U8X8_MSG_DELAY_100NANO: // delay arg_int * 100 nano seconds
            // printf("U8X8_MSG_DELAY_100NANO %d\n", arg_int);
            sleep_us(1000 * 100 * arg_int);
            break;
        case U8X8_MSG_DELAY_10MICRO: // delay arg_int * 10 micro seconds
            // printf("U8X8_MSG_DELAY_10MICRO %d\n", arg_int);
            sleep_us(arg_int * 10);
            break;
        case U8X8_MSG_DELAY_MILLI: // delay arg_int * 1 milli second
            // printf("U8X8_MSG_DELAY_MILLI %d\n", arg_int);
            sleep_ms(arg_int);
            break;
        case U8X8_MSG_DELAY_I2C: // arg_int is the I2C speed in 100KHz, e.g. 4 = 400
            // KHz
            break; // arg_int=1: delay by 5us, arg_int = 4: delay by 1.25us
        case U8X8_MSG_GPIO_D0: // D0 or SPI clock pin: Output level in arg_int
            // case U8X8_MSG_GPIO_SPI_CLOCK:
            break;
        case U8X8_MSG_GPIO_D1: // D1 or SPI data pin: Output level in arg_int
            // case U8X8_MSG_GPIO_SPI_DATA:
            break;
        case U8X8_MSG_GPIO_D2: // D2 pin: Output level in arg_int
            break;
        case U8X8_MSG_GPIO_D3: // D3 pin: Output level in arg_int
            break;
        case U8X8_MSG_GPIO_D4: // D4 pin: Output level in arg_int
            break;
        case U8X8_MSG_GPIO_D5: // D5 pin: Output level in arg_int
            break;
        case U8X8_MSG_GPIO_D6: // D6 pin: Output level in arg_int
            break;
        case U8X8_MSG_GPIO_D7: // D7 pin: Output level in arg_int
            break;
        case U8X8_MSG_GPIO_E: // E/WR pin: Output level in arg_int
            break;
        case U8X8_MSG_GPIO_CS: // CS (chip select) pin: Output level in arg_int
            // printf("U8X8_MSG_GPIO_CS %d\n", arg_int);
            gpio_put(PIN_CS, arg_int);
            break;
        case U8X8_MSG_GPIO_DC: // DC (data/cmd, A0, register select) pin: Output level
            // in arg_int
            // printf("U8X8_MSG_GPIO_DC %d\n", arg_int);
            gpio_put(PIN_DC, arg_int);
            break;
        case U8X8_MSG_GPIO_RESET: // Reset pin: Output level in arg_int
            // printf("U8X8_MSG_GPIO_RESET %d\n", arg_int);
            gpio_put(PIN_RST, arg_int);
            break;
        case U8X8_MSG_GPIO_CS1: // CS1 (chip select) pin: Output level in arg_int
            break;
        case U8X8_MSG_GPIO_CS2: // CS2 (chip select) pin: Output level in arg_int
            break;
        case U8X8_MSG_GPIO_I2C_CLOCK: // arg_int=0: Output low at I2C clock pin
            break; // arg_int=1: Input dir with pullup high for I2C clock pin
        case U8X8_MSG_GPIO_I2C_DATA: // arg_int=0: Output low at I2C data pin
            break; // arg_int=1: Input dir with pullup high for I2C data pin
        case U8X8_MSG_GPIO_MENU_SELECT:
            u8x8_SetGPIOResult(u8x8, /* get menu select pin state */ 0);
            break;
        case U8X8_MSG_GPIO_MENU_NEXT:
            u8x8_SetGPIOResult(u8x8, /* get menu next pin state */ 0);
            break;
        case U8X8_MSG_GPIO_MENU_PREV:
            u8x8_SetGPIOResult(u8x8, /* get menu prev pin state */ 0);
            break;
        case U8X8_MSG_GPIO_MENU_HOME:
            u8x8_SetGPIOResult(u8x8, /* get menu home pin state */ 0);
            break;
        default:
            u8x8_SetGPIOResult(u8x8, 1); // default return value
            break;
    }
    return 1;
}