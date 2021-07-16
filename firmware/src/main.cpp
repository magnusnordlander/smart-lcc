#include "mbed.h"
#include "SystemStatus.h"
#include "ControlBoardCommunication/ControlBoardTransceiver.h"
#include <USB/PluggableUSBSerial.h>
#include <UI/UIController.h>
#include <SystemController/SystemController.h>
#include <Adafruit_SSD1306.h>
#include <Debug/AuxLccTransceiver.h>

#define OLED_MOSI digitalPinToPinName(PIN_SPI_MOSI)
#define OLED_MISO digitalPinToPinName(PIN_SPI_MISO)
#define OLED_SCK digitalPinToPinName(PIN_SPI_SCK)
#define OLED_DC digitalPinToPinName(2)
#define OLED_RST digitalPinToPinName(3)
#define OLED_CS digitalPinToPinName(PIN_SPI_SS)

#define CB_TX digitalPinToPinName(16)
#define CB_RX digitalPinToPinName(17)

#define AUX_TX digitalPinToPinName(8)
#define AUX_RX digitalPinToPinName(9)

REDIRECT_STDOUT_TO(SerialUSB);

using namespace std::chrono_literals;
mbed::DigitalOut led(LED1);

SystemStatus* systemStatus = new SystemStatus;

rtos::Thread controlBoardCommunicationThread;
ControlBoardTransceiver trx(CB_TX, CB_RX, systemStatus);

rtos::Thread auxLccCommunicationThread;
AuxLccTransceiver auxTrx(AUX_TX, AUX_RX, systemStatus);

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

SPIPreInit gSpi(OLED_MOSI, OLED_MISO, OLED_SCK);
Adafruit_SSD1306_Spi gOled1(gSpi, OLED_DC, OLED_RST, OLED_CS,64);

rtos::Thread uiThread;
UIController uiController(systemStatus, &gOled1);

rtos::Thread systemControllerThread;
SystemController systemController(systemStatus);

int main()
{
    uiThread.start([] { uiController.run(); });

    controlBoardCommunicationThread.start([] { trx.run(); });
//    auxLccCommunicationThread.start([] { auxTrx.run(); });
//    auxLccCommunicationThread.set_priority(osPriorityRealtime);
    systemControllerThread.start([] { systemController.run(); });

    while(true) {
        led = !led;
        rtos::ThisThread::sleep_for(2000ms);
    }
}