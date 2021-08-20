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
#include "SystemSettings.h"

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

PicoQueue<SystemControllerCommand> *queue0 = new PicoQueue<SystemControllerCommand>(100);
PicoQueue<SystemControllerStatusMessage> *queue1 = new PicoQueue<SystemControllerStatusMessage>(100);

SystemStatus* systemStatus = new SystemStatus;
SystemSettings* systemSettings = new SystemSettings(queue0);

SystemController systemController(uart0, CB_TX, CB_RX, queue1, queue0);

SPIPreInit gSpi(OLED_MOSI, OLED_MISO, OLED_SCK);
Adafruit_SSD1306_Spi gOled1(gSpi, OLED_DC, OLED_RST, OLED_CS,64);
rtos::Thread uiThread;
UIController uiController(systemStatus, &gOled1);

rtos::Thread wifiThread(osPriorityBelowNormal);
WifiTransceiver wifiTransceiver(systemStatus);

[[noreturn]] void launchCore1() {
    //mbed::Watchdog::get_instance().start(3000);
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
    SerialAux.begin(115200);
#endif
    systemSettings->initialize();

    //rtos::ThisThread::sleep_for(5000ms);
    //systemStatus->readSettingsFromKV();

    //mbed::Watchdog::get_instance().start(3000);
    //systemControllerThread.start([] { systemController.run(); });
    multicore_launch_core1(launchCore1);

    uiThread.start([] { uiController.run(); });
    //wifiThread.start([] { wifiTransceiver.run(); });

    //launchCore1();

    SystemControllerStatusMessage message;

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
    while(true) {
        while (!queue1->isEmpty()) {
            queue1->removeBlocking(&message);

            systemStatus->hasReceivedControlBoardPacket = true;
            systemStatus->hasSentLccPacket = true;
            systemStatus->brewBoilerTemperature = message.brewTemperature;
            systemStatus->serviceBoilerTemperature = message.serviceTemperature;
            systemStatus->brewing = message.currentlyBrewing;
            systemStatus->filling = message.currentlyFillingServiceBoiler;
            systemStatus->waterTankEmpty = message.waterTankLow;
            systemStatus->brewSsr = message.brewSSRActive;
            systemStatus->serviceSsr = message.serviceSSRActive;

            printf("BT: %.1f SSR: %u P: %.2f BI: %.2f BD: %.2f BInt: %.2f\n",
               message.brewTemperature,
               message.brewSSRActive,
               message.brewPidParameters.p,
               message.brewPidParameters.i,
               message.brewPidParameters.d,
               message.brewPidParameters.integral);
        }

        rtos::ThisThread::sleep_for(10ms);
    }
#pragma clang diagnostic pop
}