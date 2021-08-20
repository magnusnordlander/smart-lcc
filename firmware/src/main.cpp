#include "mbed.h"
#include "SystemStatus.h"
#include <USB/PluggableUSBSerial.h>
#include <UI/UIController.h>
#include <SystemController/SystemController.h>
#include <Adafruit_SSD1306.h>
#include <ExternalComms/WifiTransceiver.h>
#include <utils/SPIPreInit.h>
#include "pico/multicore.h"
#include "utils/PicoQueue.h"
#include "types.h"

#define OLED_MOSI digitalPinToPinName(PIN_SPI_MOSI)
#define OLED_MISO digitalPinToPinName(PIN_SPI_MISO)
#define OLED_SCK digitalPinToPinName(PIN_SPI_SCK)
#define OLED_DC digitalPinToPinName(2)
#define OLED_RST digitalPinToPinName(3)
#define OLED_CS digitalPinToPinName(PIN_SPI_SS)

#define CB_TX SERIAL1_TX
#define CB_RX SERIAL1_RX

#define AUX_TX digitalPinToPinName(8)
#define AUX_RX digitalPinToPinName(9)

UART SerialAux(AUX_TX, AUX_RX);

REDIRECT_STDOUT_TO(SerialAux);

using namespace std::chrono_literals;

PicoQueue<SystemControllerCommand> queue0(100, 0xCA);
PicoQueue<SystemControllerStatusMessage> queue1(100, 0xCB);

SystemStatus* systemStatus0 = new SystemStatus;
SystemStatus* systemStatus1 = new SystemStatus;

SystemController systemController(uart0, CB_TX, CB_RX, &queue1, &queue0);

SPIPreInit gSpi(OLED_MOSI, OLED_MISO, OLED_SCK);
Adafruit_SSD1306_Spi gOled1(gSpi, OLED_DC, OLED_RST, OLED_CS,64);
rtos::Thread uiThread;
UIController uiController(systemStatus0, &gOled1);

rtos::Thread wifiThread(osPriorityBelowNormal);
WifiTransceiver wifiTransceiver(systemStatus0);

[[noreturn]] void launchCore1() {
    mbed::Watchdog::get_instance().start(3000);
    systemController.run();
}

int main()
{
#ifdef USB_DEBUG
    PluggableUSBD().begin();
    _SerialUSB.begin(9600);

    // No idea why this is needed, but without it core1 stalls
    rtos::ThisThread::sleep_for(3000ms);
#else
    SerialAux.begin(9600);
#endif
    queue1.getLevelUnsafe();

    //rtos::ThisThread::sleep_for(5000ms);
    //systemStatus0->readSettingsFromKV();

    //mbed::Watchdog::get_instance().start(3000);
    //systemControllerThread.start([] { systemController.run(); });
    multicore_launch_core1(launchCore1);

    //uiThread.start([] {  });
    //wifiThread.start([] { wifiTransceiver.run(); });

    //launchCore1();

    uiController.run();

    while(true) {
        rtos::ThisThread::sleep_for(1000ms);
    }
}