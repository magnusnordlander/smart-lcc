#include "mbed.h"
#include "SystemStatus.h"
#include <USB/PluggableUSBSerial.h>
#include <UI/UIController.h>
#include <SystemController/SystemController.h>
#include <Adafruit_SSD1306.h>
#include <Debug/AuxLccTransceiver.h>
#include <ExternalComms/WifiTransceiver.h>
#include <utils/SPIPreInit.h>
#include <Debug/LccMasterTransceiver.h>

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

REDIRECT_STDOUT_TO(SerialUSB);

using namespace std::chrono_literals;

SystemStatus* systemStatus = new SystemStatus;

rtos::Thread systemControllerThread(osPriorityRealtime);
SystemController systemController(CB_TX, CB_RX, systemStatus);

SPIPreInit gSpi(OLED_MOSI, OLED_MISO, OLED_SCK);
Adafruit_SSD1306_Spi gOled1(gSpi, OLED_DC, OLED_RST, OLED_CS,64);
rtos::Thread uiThread;
UIController uiController(systemStatus, &gOled1);

rtos::Thread wifiThread(osPriorityBelowNormal);
WifiTransceiver wifiTransceiver(systemStatus);

int main()
{
    PluggableUSBD().begin();
    _SerialUSB.begin(9600);

    mbed::Watchdog::get_instance().start(1000);
    systemControllerThread.start([] { systemController.run(); });

    uiThread.start([] { uiController.run(); });
    wifiThread.start([] { wifiTransceiver.run(); });

    while(true) {
        rtos::ThisThread::sleep_for(1000ms);
    }
}