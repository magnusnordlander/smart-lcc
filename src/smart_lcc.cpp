#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pins.h"
#include "u8g2.h"
#include "u8g2functions.h"
#include "Controller/Core0/SystemController.h"
#include "utils/PicoQueue.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "MulticoreSupport.h"
#include "Controller/SingleCore/EspBootloader.h"
#include "Controller/Core0/Utils/ClearUartCruft.h"

repeating_timer_t safePacketBootupTimer;
u8g2_t u8g2;
SystemController* systemController;
PicoQueue<SystemControllerStatusMessage>* statusQueue;
PicoQueue<SystemControllerCommand>* commandQueue;
MulticoreSupport support;
extern "C" {
volatile bool __otherCoreIdled = false;
};

[[noreturn]] void main1();

void initGpio() {
    // LED_BUILTIN uses the same pin as OLED SCK, so we can't use that
    // gpio_init(LED_BUILTIN);
    // gpio_set_dir(LED_BUILTIN, true);

    gpio_init(NINA_GPIO0);
    gpio_init(NINA_RESETN);
    gpio_init(PLUS_BUTTON);
    gpio_init(MINUS_BUTTON);

//    gpio_set_function(AUX_RX, GPIO_FUNC_UART);
//    gpio_set_function(AUX_TX, GPIO_FUNC_UART);

    gpio_set_function(NINA_UART_TX_SPI_MISO, GPIO_FUNC_UART);
    gpio_set_function(NINA_UART_RX_SPI_CS, GPIO_FUNC_UART);

    gpio_set_dir(NINA_GPIO0, true);
    gpio_set_dir(NINA_RESETN, true);

    gpio_set_dir(PLUS_BUTTON, false);
    gpio_pull_down(PLUS_BUTTON);
    gpio_set_dir(MINUS_BUTTON, false);
    gpio_pull_down(MINUS_BUTTON);

    gpio_set_function(CB_RX, GPIO_FUNC_UART);
    gpio_set_inover(CB_RX, GPIO_OVERRIDE_INVERT);
    gpio_set_function(CB_TX, GPIO_FUNC_UART);
    gpio_set_outover(CB_TX, GPIO_OVERRIDE_INVERT);

    uart_init(uart0, 9600);
    uart_init(uart1, 115200);
}

void initEsp() {
    gpio_put(NINA_GPIO0, true);
    gpio_put(NINA_RESETN, true);

    bool correctVersion = false;

    if (!correctVersion) {
        gpio_put(NINA_RESETN, false);
        gpio_put(NINA_GPIO0, false);

        sleep_ms(50);

        u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);

        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawStr(&u8g2, 16, 16, "Incorrect ESP32");
        u8g2_DrawStr(&u8g2, 16, 24, "firmware version");
        u8g2_DrawStr(&u8g2, 16, 32, "Press + to flash");
        u8g2_SendBuffer(&u8g2);

/*        while (gpio_get(PLUS_BUTTON) != 1) {
            sleep_ms(10);
        }*/

        absolute_time_t time0 = get_absolute_time();

        //sleep_ms(1000);

/*        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawStr(&u8g2, 16, 16, "Detecting");
        u8g2_SendBuffer(&u8g2);*/

        gpio_put(NINA_RESETN, true);

        sleep_ms(100);

        uart_clear_cruft(uart1);

        EspBootloader bootloader(uart1);
        esp_bootloader_error_t err;
        uint32_t magicAddr = 0x40001000;

        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawStr(&u8g2, 16, 16, "Syncing");
        u8g2_SendBuffer(&u8g2);

        while (!bootloader.sync()) {}

        uint32_t regVal;

        err = bootloader.readReg(magicAddr, &regVal);

        if (err != 0x00) {
            u8g2_ClearBuffer(&u8g2);
            u8g2_DrawStr(&u8g2, 16, 16, "Bootloader error");
            u8g2_SendBuffer(&u8g2);

            while (gpio_get(PLUS_BUTTON) != 1) {
                sleep_ms(10);
            }
        } else if (regVal == 0x00F01D83) {
            u8g2_ClearBuffer(&u8g2);
            u8g2_DrawStr(&u8g2, 16, 16, "Magic correct");
            u8g2_SendBuffer(&u8g2);
        } else {
            u8g2_ClearBuffer(&u8g2);
            u8g2_DrawStr(&u8g2, 16, 16, "Magic wrong");
            u8g2_SendBuffer(&u8g2);

            while (gpio_get(PLUS_BUTTON) != 1) {
                sleep_ms(10);
            }
        }
/*
        err = bootloader.spiSetParamsNinaW102();

        if (err == 0x00) {
            u8g2_ClearBuffer(&u8g2);
            u8g2_DrawStr(&u8g2, 16, 16, "SPI params");
            u8g2_SendBuffer(&u8g2);
        } else {
            u8g2_ClearBuffer(&u8g2);
            u8g2_DrawStr(&u8g2, 16, 16, "Err SPI params");
            u8g2_SendBuffer(&u8g2);

            while (gpio_get(PLUS_BUTTON) != 1) {
                sleep_ms(10);
            }
        }

        err = bootloader.spiAttach();

        if (err == 0x00) {
            u8g2_ClearBuffer(&u8g2);
            u8g2_DrawStr(&u8g2, 16, 16, "SPI attach");
            u8g2_SendBuffer(&u8g2);
        } else {
            u8g2_ClearBuffer(&u8g2);
            u8g2_DrawStr(&u8g2, 16, 16, "Err SPI attach");
            u8g2_SendBuffer(&u8g2);

            while (gpio_get(PLUS_BUTTON) != 1) {
                sleep_ms(10);
            }
        }

        esp_bootloader_md5_t md5{};
        err = bootloader.spiFlashMd5(0x10000, 1024, &md5);

        absolute_time_t time1 = get_absolute_time();

        if (err == 0x00) {
            u8g2_ClearBuffer(&u8g2);
            u8g2_DrawStr(&u8g2, 16, 16, "MD5");
            char timeStr[10];

            snprintf(timeStr, sizeof(timeStr), "%lli", absolute_time_diff_us(time0, time1)/1000);
            u8g2_DrawStr(&u8g2, 0, 24, timeStr);

            u8g2_SendBuffer(&u8g2);
        } else {
            u8g2_ClearBuffer(&u8g2);
            u8g2_DrawStr(&u8g2, 16, 16, "Err MD5");
            char errStr[10];

            snprintf(errStr, sizeof(errStr), "%i", err);
            u8g2_DrawStr(&u8g2, 0, 24, errStr);
            u8g2_SendBuffer(&u8g2);

            while (gpio_get(PLUS_BUTTON) != 1) {
                sleep_ms(10);
            }
        }
*/
        err = bootloader.uploadStub();

        if (err == 0x00) {
            u8g2_ClearBuffer(&u8g2);
            u8g2_DrawStr(&u8g2, 16, 16, "Stub");
            u8g2_SendBuffer(&u8g2);
        } else {
            u8g2_ClearBuffer(&u8g2);
            u8g2_DrawStr(&u8g2, 16, 16, "Err Stub");
            u8g2_SendBuffer(&u8g2);

            while (gpio_get(PLUS_BUTTON) != 1) {
                sleep_ms(10);
            }
        }

        while (gpio_get(PLUS_BUTTON) != 1) {
            sleep_ms(10);
        }
    }
}

void initU8g2() {
    u8g2_Setup_ssd1306_128x64_noname_f(&u8g2, U8G2_R2, u8x8_byte_pico_hw_spi,
                                       u8x8_gpio_and_delay_template);
    u8g2_InitDisplay(&u8g2);
    u8g2_ClearDisplay(&u8g2);
    u8g2_SetPowerSave(&u8g2, 0);

    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
}

void u8g2Char(char c) {
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawGlyph(&u8g2, 16, 16, c);
    u8g2_SendBuffer(&u8g2);
}

void u8g2Int(uint8_t i) {
    u8g2_SetFont(&u8g2, u8g2_font_ncenB14_tr);
    char str[5];

    sprintf(str, "%d", i);

    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, 16, 16, str);
    u8g2_SendBuffer(&u8g2);
}

[[noreturn]] void main0() {
    cancel_repeating_timer(&safePacketBootupTimer);
    support.registerCore();

    // Core 0 - System controller (incl. safe packet sender), Settings controller
    nonstd::optional<absolute_time_t> core1RebootTimer{};

    while(true) {
        if (watchdog_get_count() > 0) {
            watchdog_update();
        }

        if (core1RebootTimer.has_value() && absolute_time_diff_us(core1RebootTimer.value(), get_absolute_time()) > 0) {
            multicore_reset_core1();
            multicore_launch_core1(main1);
            core1RebootTimer = make_timeout_time_ms(5000);
        }

        systemController->loop();

        if (statusQueue->isFull())  {
            if (!core1RebootTimer.has_value()) {
                core1RebootTimer = make_timeout_time_ms(2000);
            }
        }
    }
}

[[noreturn]] void main1() {
    u8g2Char('X');
    support.registerCore();
    SystemControllerStatusMessage sm;

    u8g2Char('Y');

    SystemControllerCommand beginCommand;
    beginCommand.type = COMMAND_BEGIN;
    commandQueue->tryAdd(&beginCommand);

    u8g2Char('Z');

    while (true) {
        u8g2Int(statusQueue->getLevelUnsafe());

        while (!statusQueue->isEmpty()) {
            statusQueue->removeBlocking(&sm);
        }

        uart_write_blocking(uart1, reinterpret_cast<const uint8_t *>(&sm), sizeof(SystemControllerStatusMessage));
    }

    // Core 1 - UI Controller, ESP UART controller, USB-ESP bridge

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

bool repeating_timer_callback(repeating_timer_t *t) {
    systemController->sendSafePacketNoWait();
    return true;
}

int main() {
    initGpio();
    initU8g2();
    u8g2Char('A');

    support.begin(2);

    u8g2Char('B');

    statusQueue = new PicoQueue<SystemControllerStatusMessage>(100);
    commandQueue = new PicoQueue<SystemControllerCommand>(100);

    SystemSettings settings(commandQueue, &support);
    settings.initialize();

    u8g2Char('C');

    systemController = new SystemController(uart0, statusQueue, commandQueue, &settings);
    add_repeating_timer_ms(1000, repeating_timer_callback, NULL, &safePacketBootupTimer);

    u8g2Char('D');

    initEsp();

    u8g2Char('E');

    multicore_reset_core1();
    multicore_launch_core1(main1);

    main0();
}
