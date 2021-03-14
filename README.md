# Smart LCC

A smarter LCC for the Lelit Bianca, with Wi-Fi and stuff.

Based on the protocol dissection I'm doing in [magnusnordlander/lelit-bianca-protocol](https://github.com/magnusnordlander/lelit-bianca-protocol).

## Project goals

* Stage 1: Create a piece of hardware that can sniff the bus between the LCC and the Gicar Control Board.
* Stage 2: Instead of sniffing the bus, act as a transceiver, i.e. receive on one UART, and relay on another.
* Stage 3: Create a replacement for the LCC.

## Architecture

This project will use dual MCUs, the Seeeduino Xiao and the Arduino Nano 33 IoT, both SAMD21 based. The UART protocol is timing sensitive, and as such, it needs to operate without lots of interrupt masking. Using the Wi-Fi on the Arduino Nano 33 IoT masks interrupts a lot.

## Status

Pre stage 1. Currently, this only uses one MCU, and as such is kind of unreliable even for bus snooping. Don't use this.