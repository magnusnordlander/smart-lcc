#include <Arduino.h>
#include <pins_arduino.h>
#include <hardware/uart.h>
#include <hardware/irq.h>
#include <U8g2lib.h>
#include <hardware/watchdog.h>
#include <hardware/adc.h>
#include "src/AutomationController.h"
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
AutomationController automationController(&status, &settings);

U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R2, /* cs=*/ OLED_CS, /* dc=*/ OLED_DC, /* reset=*/ OLED_RST);
UIController uiController(&status, &settings, &u8g2, MINUS_BUTTON, PLUS_BUTTON);

volatile SystemMode systemMode = SYSTEM_MODE_UNDETERMINED;

void setup() {
    u8g2.begin();

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_9x15_tf);
    u8g2.drawDisc(32, 32, 20);
    u8g2.drawStr(32, 64, "AA");
    u8g2.sendBuffer();

#ifdef DEBUG_RP2040_CORE
    Serial.begin(115200);
    while(!Serial) { sleep_ms(1); }
    sleep_ms(500);
#endif

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_9x15_tf);
    u8g2.drawDisc(48, 32, 20);
    u8g2.drawStr(48, 64, "BB");
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

    adc_init();
    adc_set_temp_sensor_enabled(true);

    if (gpio_get(PLUS_BUTTON)) {
        systemMode = SYSTEM_MODE_OTA;
    } else if (gpio_get(MINUS_BUTTON)) {
        systemMode = SYSTEM_MODE_CONFIG;
    } else {
        systemMode = SYSTEM_MODE_NORMAL;
        watchdog_enable(3000, false);
    }
}


void setup1()
{
    while (systemMode == SYSTEM_MODE_UNDETERMINED) {
        tight_loop_contents();
    }

    DEBUGV("Setup1\n");

    fileSystem->begin();

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_9x15_tf);
    u8g2.drawDisc(64, 32, 20);
    u8g2.drawStr(64, 64, "CC");
    u8g2.sendBuffer();

    networkController.init(systemMode);

    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_9x15_tf);
    u8g2.drawDisc(72, 32, 20);
    u8g2.drawStr(72, 64, "DD");
    u8g2.sendBuffer();

    if (systemMode == SYSTEM_MODE_NORMAL) {
        /* @todo If current time is more than 60 seconds, set some flag, because that's weird */

        settings.initialize();
        automationController.init();

        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_9x15_tf);
        u8g2.drawDisc(80, 32, 20);
        u8g2.drawStr(80, 64, "EE");
        u8g2.sendBuffer();

        SystemControllerCommand beginCmd = SystemControllerCommand{.type = COMMAND_BEGIN};
        queue0->addBlocking(&beginCmd);
    }
}

float rp2040Temperature() {
    adc_select_input(4);

    auto raw = (float)adc_read();
    const float conversion_factor = 3.3f / (1<<12);
    float result = raw * conversion_factor;
    return 27.f - (result -0.706)/0.001721;
}

void loop1()
{
    /* @todo This should not be in a timer */
    if (systemMode != SYSTEM_MODE_NORMAL) {
        safePacketSender.loop();
    } else {
        automationController.loop();
    }

    /* @todo We can either use hardware_timer or pico_time/repeating_timer. Regardless, we'll have to create a core1 alarm pool */

    SystemControllerStatusMessage message;

    /* @todo Check if pico_queue is interrupt safe */
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
    status.ipAddress = networkController.getIPAddress();

    /* @todo This should also not be run in an interrupt */
    status.rp2040Temperature = rp2040Temperature();

    /*
     * @todo
     * This is *not* interrupt safe, because it can trigger LittleFS-writes. That'll have to be handled by writing
     * to some kind of shadow config and updating LittleFS from within loop1.
     */
    uiController.loop();
}

void loop() {
    systemController.loop();
}