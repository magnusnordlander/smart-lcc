/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef NANO33IOT_BIANCAUART_H
#define NANO33IOT_BIANCAUART_H

#include "api/HardwareSerial.h"
#include "SERCOM.h"
#include "SafeRingBuffer.h"
#include "Arduino.h"
#include "wiring_private.h"

#define NO_RTS_PIN 255
#define NO_CTS_PIN 255
#define RTS_RX_THRESHOLD 10

template <int N>
class BiancaUart : public HardwareSerial
{
public:
    BiancaUart(SERCOM *_s, uint8_t _pinRX, uint8_t _pinTX, SercomRXPad _padRX, SercomUartTXPad _padTX);
    BiancaUart(SERCOM *_s, uint8_t _pinRX, uint8_t _pinTX, SercomRXPad _padRX, SercomUartTXPad _padTX, uint8_t _pinRTS, uint8_t _pinCTS);
    void begin(unsigned long baudRate) override;
    void begin(unsigned long baudrate, uint16_t config) override;
    void end() override;
    int available() override;
    int availableForWrite() override;
    int peek() override;
    int read() override;
    void flush() override;
    size_t write(const uint8_t data) override;
    using Print::write; // pull in write(str) and write(buf, size) from Print

    void IrqHandler();

    size_t readDatagram(uint8_t *buffer);
    bool goodTimeForWifi();

    explicit operator bool() override { return true; }

private:
    SERCOM *sercom{};
    SafeRingBuffer rxBuffer;
    SafeRingBuffer txBuffer;

    bool _goodTimeForWifi = false;
    unsigned long previousByteReceivedAt = 0;
    uint8_t datagramTail = 0;
    unsigned long timeDiff = 0;

    uint8_t currentDatagram[N] = {0};
    uint8_t previousDatagram[N] = {0};

    uint8_t uc_pinRX{};
    uint8_t uc_pinTX{};
    SercomRXPad uc_padRX;
    SercomUartTXPad uc_padTX;
    uint8_t uc_pinRTS{};
    volatile uint32_t* pul_outsetRTS{};
    volatile uint32_t* pul_outclrRTS{};
    uint32_t ul_pinMaskRTS{};
    uint8_t uc_pinCTS{};

    SercomNumberStopBit extractNbStopBit(uint16_t config);
    SercomUartCharSize extractCharSize(uint16_t config);
    SercomParityMode extractParity(uint16_t config);
};

template <int N>
BiancaUart<N>::BiancaUart(SERCOM *_s, uint8_t _pinRX, uint8_t _pinTX, SercomRXPad _padRX, SercomUartTXPad _padTX) :
        BiancaUart(_s, _pinRX, _pinTX, _padRX, _padTX, NO_RTS_PIN, NO_CTS_PIN)
{
}

template <int N>
BiancaUart<N>::BiancaUart(SERCOM *_s, uint8_t _pinRX, uint8_t _pinTX, SercomRXPad _padRX, SercomUartTXPad _padTX, uint8_t _pinRTS, uint8_t _pinCTS)
{
    sercom = _s;
    uc_pinRX = _pinRX;
    uc_pinTX = _pinTX;
    uc_padRX = _padRX ;
    uc_padTX = _padTX;
    uc_pinRTS = _pinRTS;
    uc_pinCTS = _pinCTS;
}

template <int N>
void BiancaUart<N>::begin(unsigned long baudrate)
{
    begin(baudrate, SERIAL_8N1);
}

template <int N>
void BiancaUart<N>::begin(unsigned long baudrate, uint16_t config)
{
    pinPeripheral(uc_pinRX, g_APinDescription[uc_pinRX].ulPinType);
    pinPeripheral(uc_pinTX, g_APinDescription[uc_pinTX].ulPinType);

    if (uc_padTX == UART_TX_RTS_CTS_PAD_0_2_3) {
        if (uc_pinCTS != NO_CTS_PIN) {
            pinPeripheral(uc_pinCTS, g_APinDescription[uc_pinCTS].ulPinType);
        }
    }

    if (uc_pinRTS != NO_RTS_PIN) {
        pinMode(uc_pinRTS, OUTPUT);

        EPortType rtsPort = g_APinDescription[uc_pinRTS].ulPort;
        pul_outsetRTS = &PORT->Group[rtsPort].OUTSET.reg;
        pul_outclrRTS = &PORT->Group[rtsPort].OUTCLR.reg;
        ul_pinMaskRTS = (1ul << g_APinDescription[uc_pinRTS].ulPin);

        *pul_outclrRTS = ul_pinMaskRTS;
    }

    sercom->initUART(UART_INT_CLOCK, SAMPLE_RATE_x16, baudrate);
    sercom->initFrame(extractCharSize(config), LSB_FIRST, extractParity(config), extractNbStopBit(config));
    sercom->initPads(uc_padTX, uc_padRX);

    sercom->enableUART();
}

template <int N>
void BiancaUart<N>::end()
{
    sercom->resetUART();
    rxBuffer.clear();
    txBuffer.clear();
}

template <int N>
void BiancaUart<N>::flush()
{
    while(txBuffer.available()); // wait until TX buffer is empty

    sercom->flushUART();
}

template <int N>
void BiancaUart<N>::IrqHandler()
{
    if (sercom->isFrameErrorUART()) {
        // frame error, next byte is invalid so read and discard it
        sercom->readDataUART();

        sercom->clearFrameErrorUART();
    }

    if (sercom->availableDataUART()) {
        unsigned long currentTime = micros();
        timeDiff = (currentTime - previousByteReceivedAt);
        _goodTimeForWifi = false;

        if (timeDiff > 70*1000) {
            memset(currentDatagram, 0, sizeof(currentDatagram));
            datagramTail = 0;
        }

        previousByteReceivedAt = currentTime;

        uint8_t data = sercom->readDataUART();
        rxBuffer.store_char(data);

        if (uc_pinRTS != NO_RTS_PIN) {
            // RX buffer space is below the threshold, de-assert RTS
            if (rxBuffer.availableForStore() < RTS_RX_THRESHOLD) {
                *pul_outsetRTS = ul_pinMaskRTS;
            }
        }

        if (datagramTail <= N) {
            currentDatagram[datagramTail] = data;

            if (datagramTail >= N) {
                //memset(previousDatagram, 1, N);
                memcpy(previousDatagram, currentDatagram, N);
                _goodTimeForWifi = true;
            }

            datagramTail++;
        }
    }

    if (sercom->isDataRegisterEmptyUART()) {
        if (txBuffer.available()) {
            uint8_t data = txBuffer.read_char();

            sercom->writeDataUART(data);
        } else {
            sercom->disableDataRegisterEmptyInterruptUART();
        }
    }

    if (sercom->isUARTError()) {
        sercom->acknowledgeUARTError();
        // TODO: if (sercom->isBufferOverflowErrorUART()) ....
        // TODO: if (sercom->isParityErrorUART()) ....
        sercom->clearStatusUART();
    }
}

template <int N>
int BiancaUart<N>::available()
{
    return rxBuffer.available();
}

template <int N>
int BiancaUart<N>::availableForWrite()
{
    return txBuffer.availableForStore();
}

template <int N>
int BiancaUart<N>::peek()
{
    return rxBuffer.peek();
}

template <int N>
int BiancaUart<N>::read()
{
    int c = rxBuffer.read_char();

    if (uc_pinRTS != NO_RTS_PIN) {
        // if there is enough space in the RX buffer, assert RTS
        if (rxBuffer.availableForStore() > RTS_RX_THRESHOLD) {
            *pul_outclrRTS = ul_pinMaskRTS;
        }
    }

    return c;
}

template <int N>
size_t BiancaUart<N>::write(const uint8_t data)
{
    if (sercom->isDataRegisterEmptyUART() && txBuffer.available() == 0) {
        sercom->writeDataUART(data);
    } else {
        // spin lock until a spot opens up in the buffer
        while(txBuffer.isFull()) {
            uint8_t interruptsEnabled = ((__get_PRIMASK() & 0x1) == 0);

            if (interruptsEnabled) {
                uint32_t exceptionNumber = (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk);

                if (exceptionNumber == 0 ||
                    NVIC_GetPriority((IRQn_Type)(exceptionNumber - 16)) > SERCOM_NVIC_PRIORITY) {
                    // no exception or called from an ISR with lower priority,
                    // wait for free buffer spot via IRQ
                    continue;
                }
            }

            // interrupts are disabled or called from ISR with higher or equal priority than the SERCOM IRQ
            // manually call the UART IRQ handler when the data register is empty
            if (sercom->isDataRegisterEmptyUART()) {
                IrqHandler();
            }
        }

        txBuffer.store_char(data);

        sercom->enableDataRegisterEmptyInterruptUART();
    }

    return 1;
}

template <int N>
SercomNumberStopBit BiancaUart<N>::extractNbStopBit(uint16_t config)
{
    switch(config & SERIAL_STOP_BIT_MASK)
    {
        case SERIAL_STOP_BIT_1:
        default:
            return SERCOM_STOP_BIT_1;

        case SERIAL_STOP_BIT_2:
            return SERCOM_STOP_BITS_2;
    }
}

template <int N>
SercomUartCharSize BiancaUart<N>::extractCharSize(uint16_t config)
{
    switch(config & SERIAL_DATA_MASK)
    {
        case SERIAL_DATA_5:
            return UART_CHAR_SIZE_5_BITS;

        case SERIAL_DATA_6:
            return UART_CHAR_SIZE_6_BITS;

        case SERIAL_DATA_7:
            return UART_CHAR_SIZE_7_BITS;

        case SERIAL_DATA_8:
        default:
            return UART_CHAR_SIZE_8_BITS;

    }
}

template <int N>
SercomParityMode BiancaUart<N>::extractParity(uint16_t config)
{
    switch(config & SERIAL_PARITY_MASK)
    {
        case SERIAL_PARITY_NONE:
        default:
            return SERCOM_NO_PARITY;

        case SERIAL_PARITY_EVEN:
            return SERCOM_EVEN_PARITY;

        case SERIAL_PARITY_ODD:
            return SERCOM_ODD_PARITY;
    }
}

template<int N>
size_t BiancaUart<N>::readDatagram(uint8_t *buffer) {
    synchronized {
        memcpy(buffer, previousDatagram, N);
    }

    return N;
}

template<int N>
bool BiancaUart<N>::goodTimeForWifi() {
    synchronized {
        return _goodTimeForWifi;
    }
}

#endif //NANO33IOT_BIANCAUART_H
