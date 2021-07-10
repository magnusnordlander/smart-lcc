#include "mbed.h"
#include "SystemStatus.h"
#include "ControlBoardCommunication/ControlBoardTransceiver.h"
#include <USB/PluggableUSBSerial.h>
#include <UI/UIController.h>
#include <SystemController/SystemController.h>
#include <Adafruit_SSD1306.h>

REDIRECT_STDOUT_TO(SerialUSB);

using namespace std::chrono_literals;
mbed::DigitalOut led(LED1);

SystemStatus* systemStatus = new SystemStatus;

rtos::Thread controlBoardCommunicationThread;
ControlBoardTransceiver trx(digitalPinToPinName(16), digitalPinToPinName(17), systemStatus);

void mbed_error_hook(const mbed_error_ctx *error_context) {
    //led = true;
}

class SPIPreInit : public mbed::SPI
{
public:
    SPIPreInit(PinName mosi, PinName miso, PinName clk) : SPI(mosi,miso,clk)
    {
        format(8,3);
        frequency(2000000);
    };
};

SPIPreInit gSpi(
        digitalPinToPinName(PIN_SPI_MOSI),
        digitalPinToPinName(PIN_SPI_MISO),
        digitalPinToPinName(PIN_SPI_SCK)
        );

Adafruit_SSD1306_Spi gOled1(
        gSpi,
        digitalPinToPinName(2),
        digitalPinToPinName(3),
        digitalPinToPinName(PIN_SPI_SS),
        64
        );

rtos::Thread uiThread;
UIController uiController(systemStatus, &gOled1);

rtos::Thread systemControllerThread;
SystemController systemController(systemStatus);

int main()
{
    uiThread.start([] { uiController.run(); });

    controlBoardCommunicationThread.start([] { trx.run(); });
    systemControllerThread.start([] { systemController.run(); });
    while(true) {
        led = !led;
        rtos::ThisThread::sleep_for(2000ms);
    }
}