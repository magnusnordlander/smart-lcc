# Smart LCC

A smarter LCC for the Lelit Bianca, with Wi-Fi and stuff.

Based on the protocol dissection I'm doing in [magnusnordlander/lelit-bianca-protocol](https://github.com/magnusnordlander/lelit-bianca-protocol).

## Project goals

Create a smart replacement for the "LCC" PID module used in the Lelit Bianca with open source hardware and software.

## Architecture

### Hardware
This project is using the Arduino RP2040 Connect, a dual core ARM Cortex-M0+ board with a Ublox Nina W102 module.

A custom PCB has been designed (see `pcb/`) that is designed to fit in the original LCC enclosure. A BOM for components is also available. Aside from the Arduino RP2040 Connect, the PCB has push buttons for + and -, a resistor divider for the 5V Control Board UART signal, and a flat flex connector for a SSD1309 based OLED. The PCB has been designed with surface mount components, but uses relatively large 1206 caps and resistors, which are somewhat simple to hand solder.

A first manufacturing run of the rev A PCB has been ordered, but not yet received or tested. As such, it's *highly* unrecommended to not order any PCBs at this time.

### Firmware
The project is using PlatformIO, with ArduinoCore-mbed. Ideally, very little code will use the Arduino library, and most of it will instead use mbed or the rp2040 library. Preliminarily, this project will use the RTOS capabilities of Mbed OS, and run the processes as different threads.

Also preliminarily, the Nina W102 module will not be running the "regular" WifiNina firmware. It will be running a custom firmware which handles all external communication.

#### Threads on RP2040
* Control Board communication
  * Also performs a safety check, ensuring that temperatures in the boilers never exceed safe limits, and that both boilers are never running simultaneously.
* Machine controller
  * Responsible for PID, keeping water in the boiler, running pumps etc.
    * Includes set points for boilers, PID parameters etc
    * Also includes Wifi authentication settings
  * Basically bakes the LCC Packets for the control board
* UI controller
* Nina W102 communication
  * Allows the Nina W102 to query and change state in the machine controller.

#### Threads on Nina W102
* MQTT client

## Status

This is not yet ready. At all.
