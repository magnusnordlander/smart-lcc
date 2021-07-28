# Smart LCC

A smarter LCC for the Lelit Bianca, with Wi-Fi and stuff.

Based on the protocol dissection I'm doing in [magnusnordlander/lelit-bianca-protocol](https://github.com/magnusnordlander/lelit-bianca-protocol).

## Status

Consider this project pre-alpha. There is a revision A PCB, of which I plan to make a revision B. The firmware has been function tested with an actual machine, but it still needs parameter tweaking to actually be able to control water temperatures in a meaningful way. The UI has a long way to go. An alpha release, which is functional, but not particularly polished will probably happen during the summer of 2021. Expect a rev B board and a beta release some time during fall/winter.

Also, considering this plugs in to an expensive machine it bears to mention: Anything you do with this, you do at your own risk. Components have been fried already during the course of this project. This is not the stage to get on board with this project if you aren't willing to deal with possibly having to buy spare parts for and repair your machine.

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
* Random facts about the stock implementation:
  * Minimum on/off-time seems to be around 100-120 ms
  * When in eco mode, a warmup consists of running the coffee boiler full blast, holding the temperature around 130°C for 4 minutes, then dropping to the set point
  * When in regular mode, there is a kind of time slot system for which boiler is used. The time slots are 1 second wide.

## Licensing

The firmware is MIT licensed (excepting dependencies, which have their own, compatible licenses), and the hardware is CERN-OHL-P.
