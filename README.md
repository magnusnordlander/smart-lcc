# Smart LCC

A smarter LCC for the Lelit Bianca, with Wi-Fi and stuff.

Based on the protocol dissection I'm doing in [magnusnordlander/lelit-bianca-protocol](https://github.com/magnusnordlander/lelit-bianca-protocol).

## Project goals

Create a smart replacement for the "LCC" PID module used in the Lelit Bianca with open source hardware and software.

## Architecture

### Hardware
This project is using the Arduino RP2040 Connect, a dual core ARM Cortex-M0+ board with a Ublox Nina W102 module.

A custom PCB has been designed (see `pcb/`) that is designed to fit in the original LCC enclosure. A BOM for components is also available. Aside from the Arduino RP2040 Connect, the PCB has push buttons for + and -, a resistor divider for the 5V Control Board UART signal, and a flat flex connector for a SSD1309 based OLED. The PCB has been designed with surface mount components, but uses relatively large 1206 caps and resistors, which are somewhat simple to hand solder.

A first manufacturing run of the rev A PCB has been ordered, and preliminary testing shows that it kinda works. That being said, I have already fried two sets of GPIOs on an Arduino by futzing around with the chip, so I am planning a rev B with some protection for the GPIOs. As such, manufacturing the rev A board is not recommended (I do have some spares though, so contact me if you're interested in helping with the project, and I'll send you one).

### Firmware
The project is using PlatformIO, with ArduinoCore-mbed. Ideally, very little code will use the Arduino library, and most of it will instead use mbed or the rp2040 library. Preliminarily, this project will use the RTOS capabilities of Mbed OS, and run the processes as different threads. Currently, only one of the cores is utilized, since mbed doesn't use multiple cores.

#### Threads on RP2040
* Control Board communication
  * Safety critical, runs at `osPriorityRealtime`, defensively coded
  * Performs a safety check, ensuring that temperatures in the boilers never exceed safe limits, and that both boilers are never running simultaneously.
  * Responsible for kicking the watchdog
* System controller
  * Runs at `osPriorityAboveNormal`
  * Responsible for PID, keeping water in the boiler, running pumps etc.
    * Includes set points for boilers, PID parameters etc
    * Also includes Wifi authentication settings
  * Basically bakes the LCC Packets for the control board
* UI controller
* External communication
  * Runs at `osPriorityBelowNormal`

#### Reference material
(In no particular order)

* https://github.com/improv-wifi/sdk-cpp

## Status

This is not yet ready. At all.
