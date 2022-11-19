#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pins.h"
#include "u8g2.h"
#include "u8g2functions.h"

u8g2_t u8g2;

void initGpio() {
    gpio_init(LED_BUILTIN);
    gpio_init(NINA_GPIO0);
    gpio_init(NINA_RESETN);
    gpio_init(PLUS_BUTTON);
    gpio_init(MINUS_BUTTON);

    gpio_set_dir(LED_BUILTIN, true);

    gpio_set_function(NINA_UART_TX_SPI_MISO, GPIO_FUNC_UART);
    gpio_set_function(NINA_UART_RX_SPI_CS, GPIO_FUNC_UART);

    gpio_set_dir(NINA_GPIO0, true);
    gpio_set_dir(NINA_RESETN, true);

    gpio_set_dir(PLUS_BUTTON, false);
    gpio_pull_down(PLUS_BUTTON);
    gpio_set_dir(MINUS_BUTTON, false);
    gpio_pull_down(MINUS_BUTTON);

}

int main() {
    stdio_init_all();
    initGpio();

    u8g2_Setup_ssd1306_128x64_noname_f(&u8g2, U8G2_R2, u8x8_byte_pico_hw_spi,
                                  u8x8_gpio_and_delay_template);
    u8g2_InitDisplay(&u8g2);
    u8g2_ClearDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    while (true) {
        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawDisc(&u8g2, 32, 32, 20, U8G2_DRAW_ALL);
        u8g2_SendBuffer(&u8g2);

    }

    /*uart_init(uart1, 115200);

    while (true) {
        gpio_put(NINA_RESETN, gpio_get(PLUS_BUTTON));
        gpio_put(NINA_GPIO0, gpio_get(MINUS_BUTTON));

        gpio_put(LED_BUILTIN, gpio_get(PLUS_BUTTON));

        while (uart_is_readable(uart1)) {
            printf("%c", uart_getc(uart1));
        }
    }*/
}