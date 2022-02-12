#include <Arduino.h>
#include <pins_arduino.h>
#include <hardware/uart.h>
#include <hardware/irq.h>
#include <hardware/regs/intctrl.h>
#include <U8g2lib.h>
#include <hardware/watchdog.h>
#include "src/NetworkController.h"
#include "src/SystemController/SystemController.h"
#include "src/SystemSettings.h"
#include "src/SystemStatus.h"
#include "src/UIController.h"
#include "src/SafePacketSender.h"
#include "src/MemoryFree.h"
#include "src/FileIO.h"

#define OLED_MOSI PIN_SPI0_MOSI
#define OLED_MISO PIN_SPI0_MISO
#define OLED_SCK PIN_SPI0_SCK
#define OLED_DC D2
#define OLED_RST D3
#define OLED_CS PIN_SPI0_SS

#define PLUS_BUTTON D5
#define MINUS_BUTTON D4

#define CB_TX PIN_SERIAL1_TX
#define CB_RX PIN_SERIAL1_RX

#define AUX_TX D8
#define AUX_RX D9

PicoQueue<SystemControllerCommand> *queue0 = new PicoQueue<SystemControllerCommand>(100);
PicoQueue<SystemControllerStatusMessage> *queue1 = new PicoQueue<SystemControllerStatusMessage>(100);

FS* fileSystem = &LittleFS;
FileIO* fileIO = new FileIO(fileSystem, queue0);

SystemController systemController(uart0, queue1, queue0);
SafePacketSender safePacketSender(uart0);
SystemSettings settings(queue0, fileIO);
SystemStatus status(&settings);
NetworkController networkController(fileIO, &status, &settings);

U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R2, /* cs=*/ OLED_CS, /* dc=*/ OLED_DC, /* reset=*/ OLED_RST);
UIController uiController(&status, &settings, &u8g2, MINUS_BUTTON, PLUS_BUTTON);

void setup()
{
    u8g2.begin();

    u8g2.clearBuffer();
    u8g2.drawDisc(32, 32, 20);
    u8g2.sendBuffer();

#ifdef DEBUG_RP2040_CORE
    Serial.begin(115200);
    while(!Serial) { sleep_ms(1); }
#endif

    fileSystem->begin();

    u8g2.clearBuffer();
    u8g2.drawDisc(64, 32, 20);
    u8g2.sendBuffer();


    gpio_set_dir(PLUS_BUTTON, false);
    gpio_pull_down(PLUS_BUTTON);
    gpio_set_dir(MINUS_BUTTON, false);
    gpio_pull_down(MINUS_BUTTON);
    gpio_set_dir(PIN_LED, true);
    gpio_put(PIN_LED, true);

    gpio_set_function(CB_RX, GPIO_FUNC_UART);
    gpio_set_inover(CB_RX, GPIO_OVERRIDE_INVERT);
    gpio_set_function(CB_TX, GPIO_FUNC_UART);
    gpio_set_outover(CB_TX, GPIO_OVERRIDE_INVERT);

    uart_init(uart0, 9600);

    if (gpio_get(PLUS_BUTTON)) {
        networkController.init(NETWORK_CONTROLLER_MODE_OTA);
    } else if (gpio_get(MINUS_BUTTON)) {
        networkController.init(NETWORK_CONTROLLER_MODE_CONFIG);
    } else {
        networkController.init(NETWORK_CONTROLLER_MODE_NORMAL);
        settings.initialize();

        watchdog_enable(3000, false);

        SystemControllerCommand beginCmd = SystemControllerCommand{.type = COMMAND_BEGIN};
        queue0->addBlocking(&beginCmd);
    }
}

void loop()
{
    //DEBUGV("Loop. Memfree: %u\n", freeMemory());

    if (networkController.getMode() != NETWORK_CONTROLLER_MODE_NORMAL) {
        safePacketSender.loop();
    }

    SystemControllerStatusMessage message;

    while (!queue1->isEmpty()) {
        queue1->removeBlocking(&message);
        status.updateStatusMessage(message);
        status.hasReceivedControlBoardPacket = true;
        status.hasSentLccPacket = true;
    }

    networkController.loop();

    status.mode = networkController.getMode();
    status.wifiConnected = networkController.isConnectedToWifi();
    status.mqttConnected = networkController.isConnectedToMqtt();

    uiController.loop();
}

void loop1() {
    systemController.loop();
}