#include <cstdio>
#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "pins.h"
#include <u8g2.h>
#include "u8g2functions.h"
#include "Controller/Core0/SystemController.h"
#include "utils/PicoQueue.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "MulticoreSupport.h"
#include "Controller/SingleCore/EspBootloader.h"
#include "Controller/Core0/Utils/ClearUartCruft.h"
#include "Controller/Core0/Utils/UartReadBlockingTimeout.h"
#include "Controller/Core1/UIController.h"
#include "Controller/SingleCore/EspFirmware.h"


#define U8G2_DISP_STR(__STR__) \
u8g2_ClearBuffer(&u8g2); \
u8g2_DrawStr(&u8g2, 16, 16, __STR__); \
u8g2_SendBuffer(&u8g2);

#define WAIT_FOR_PLUS() \
while (gpio_get(PLUS_BUTTON) != 1) { \
sleep_ms(10); \
}

#define ESP_ERR_HANDLE(__err__, __name__) \
if (__err__ == 0x00) { \
U8G2_DISP_STR(__name__); \
} else { \
U8G2_DISP_STR("Err " __name__); \
WAIT_FOR_PLUS(); \
}

repeating_timer_t safePacketBootupTimer;
u8g2_t u8g2;
SystemController* systemController;
SystemStatus* status;
UIController* uiController;
PicoQueue<SystemControllerStatusMessage>* statusQueue;
PicoQueue<SystemControllerCommand>* commandQueue;
MulticoreSupport support;
EspFirmware *espFirmware;
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

    gpio_init(AUX_RX);
    gpio_set_dir(AUX_RX, true);
    gpio_put(AUX_RX, false);

    gpio_init(AUX_TX);
    gpio_set_dir(AUX_TX, true);
    gpio_put(AUX_TX, false);

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
    gpio_put(NINA_RESETN, false);

//    sleep_ms(20000);

//    bool success = espFirmware.pingBlocking();

    bool success = false;

    u8g2_SetFont(&u8g2, u8g2_font_5x7_tr);

    u8g2_ClearBuffer(&u8g2);
    u8g2_DrawStr(&u8g2, 16, 16, "ESP32 Boot mode");
    u8g2_DrawStr(&u8g2, 16, 24, "Press - to boot");
    u8g2_DrawStr(&u8g2, 16, 32, "Press + to flash");
    if (success) {
        u8g2_DrawStr(&u8g2, 16, 48, "Comms successful");
    } else {
        u8g2_DrawStr(&u8g2, 16, 48, "Comms not successful");
    }
    u8g2_SendBuffer(&u8g2);



    bool correctVersion = true;

    while (gpio_get(PLUS_BUTTON) != 1 && gpio_get(MINUS_BUTTON) != 1) {
        sleep_ms(10);
    }

    if (gpio_get(PLUS_BUTTON)) {
        correctVersion = false;
    } else if (gpio_get(MINUS_BUTTON)) {
        correctVersion = true;
    }

    if (!correctVersion) {
        gpio_put(NINA_RESETN, false);
        gpio_put(NINA_GPIO0, false);

        sleep_ms(50);

        u8g2_ClearBuffer(&u8g2);
        u8g2_DrawStr(&u8g2, 16, 16, "Incorrect ESP32");
        u8g2_DrawStr(&u8g2, 16, 24, "firmware version");
        u8g2_DrawStr(&u8g2, 16, 32, "Press + to flash");
        u8g2_SendBuffer(&u8g2);

/*        while (gpio_get(PLUS_BUTTON) != 1) {
            sleep_ms(10);
        }*/

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

        U8G2_DISP_STR("Syncing");

        while (!bootloader.sync()) {}

        sleep_ms(100);

        uart_clear_cruft(uart1, true);

        uint32_t regVal;

        err = bootloader.readReg(magicAddr, &regVal);

        if (err != 0x00) {
            U8G2_DISP_STR("Bootloader error");
            WAIT_FOR_PLUS()
        } else if (regVal == 0x00F01D83) {
            U8G2_DISP_STR("Magic correct");
        } else {
            U8G2_DISP_STR("Magic wrong");
            WAIT_FOR_PLUS()
        }

        err = bootloader.uploadStub();
        ESP_ERR_HANDLE(err, "Stub");

        err = bootloader.spiSetParamsNinaW102();
        ESP_ERR_HANDLE(err, "SPI params");

        err = bootloader.spiAttach(true);
        ESP_ERR_HANDLE(err, "SPI attach");

/*        esp_bootloader_md5_t md5{};
        err = bootloader.spiFlashMd5(0x10000, 1024, &md5);
        ESP_ERR_HANDLE(err, "MD5"); */

        err = bootloader.uploadFirmware();
        ESP_ERR_HANDLE(err, "Firmware");

        WAIT_FOR_PLUS();
    } else {
        sleep_ms(1000);

        gpio_put(NINA_RESETN, true);
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
//        espFirmware->loop();

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

    uiController = new UIController(status, commandQueue, &u8g2, MINUS_BUTTON, PLUS_BUTTON);

    u8g2Char('Z');

    SystemControllerCommand beginCommand;
    beginCommand.type = COMMAND_BEGIN;
    commandQueue->tryAdd(&beginCommand);

    absolute_time_t nextSend = make_timeout_time_ms(1000);

    espFirmware = new EspFirmware(uart1, commandQueue);
    EspFirmware::initInterrupts(uart1);

    //gpio_put(AUX_TX, true);

    while (true) {
        while (!statusQueue->isEmpty()) {
            statusQueue->removeBlocking(&sm);
        }

        status->updateStatusMessage(sm);
        uiController->loop();
        espFirmware->loop();

        if (time_reached(nextSend)) {
            espFirmware->sendStatus(&sm);
            nextSend = make_timeout_time_ms(1000);
        }
    }

    // Core 1 - UI Controller, ESP UART controller

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

    // Move inside SystemController
    SystemSettings settings(commandQueue, &support);
    settings.initialize();
    // End move

    u8g2Char('C');

    systemController = new SystemController(uart0, statusQueue, commandQueue, &settings);
    add_repeating_timer_ms(1000, repeating_timer_callback, NULL, &safePacketBootupTimer);

    u8g2Char('D');

    initEsp();

    u8g2Char('E');

    status = new SystemStatus();

    multicore_reset_core1();
    multicore_launch_core1(main1);

    main0();
}
