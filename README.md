# Smart LCC

A smarter LCC for the Lelit Bianca, with Wi-Fi and stuff.

Based on the protocol dissection I'm doing in [magnusnordlander/lelit-bianca-protocol](https://github.com/magnusnordlander/lelit-bianca-protocol).

## Status

Consider this project early alpha. Expect a beta release some time during fall/winter.

The revision B PCB works well. Manufacturing files are available in the `pcb/` directory. Unless you're *very* good at soldering fine pitch by hand, I suggest manufacturing with SMT assembly. I've used JLCPCB for my manufacturing runs, and the files have worked well there. I do have some spares of both the Rev A PCB (just the PCB) and the Rev B PCB (most components already soldered, but does need buttons, a display and an Arduino). If you have experience of embedded programming, want to help with the project and you want one of the PCBs, reach out.

Considering this plugs in to an expensive machine it bears to mention: Anything you do with this, you do at your own risk. Components have been fried already during the course of this project. This is not the stage to get on board with this project if you aren't willing to deal with possibly having to buy spare parts for and repair your machine.

## Project goals

Create a smart replacement for the "LCC" PID module used in the Lelit Bianca with open source hardware and software.

## Architecture

### Hardware
This project is using the Arduino RP2040 Connect, a dual core ARM Cortex-M0+ board with a Ublox Nina W102 module.

A custom PCB has been designed (see `pcb/`) that is designed to fit in the original LCC enclosure. A BOM for components is also available. Aside from the Arduino RP2040 Connect, the PCB has push buttons for + and -, a resistor divider for the 5V Control Board UART signal, and a flat flex connector for a SSD1309 based OLED. The PCB has been designed with surface mount components, but uses relatively large 1206 caps and resistors, which are somewhat simple to hand solder.

### Firmware
The project is using PlatformIO, with ArduinoCore-mbed. Ideally, very little code will use the Arduino library, and most of it will instead use mbed or the rp2040 library. Since Mbed OS doesn't support running the RTOS Kernel (at all) on Core 1, this is where the system controller will reside (since that doesn't require anything that requires Mbed RTOS). Core 0 will run UI and external communications, and will communicate with core 1 through `pico_util/queue`.

#### Things to avoid in Core 1
* The Mbed RTOS Kernel really doesn't like it if Core 1 enters an *mbed* critical section, so that needs to be avoided as far as possible. This pretty much means that Core 1 needs to be pure pico-sdk.

#### (Planned) Threads on RP2040 Core 0
* UI controller
* External communication

#### (Planned) RP2040 Core 1
* System controller
  * Safety critical, uses the entire core for itself
  * Communicates with the Control Board
  * Performs a safety check, ensuring that temperatures in the boilers never exceed safe limits, and that both boilers are never running simultaneously.
  * Responsible for kicking the watchdog
  * Responsible for PID, keeping water in the boiler, running pumps etc.

#### Core communication protocol

##### Core1 to Core0

* One message type
  * Timestamp (absolute_time_t) 
  * Brew temperature
  * Brew set point
  * Brew PID settings
  * Brew PID parameters
  * Service temperature
  * Service set point
  * Service PID settings
  * Service PID parameters
  * Eco mode
  * System state
    * Heatup
    * Warm
    * Sleeping
    * Bailed
  * Bail reason
  * Brewing
  * Water low

##### Core0 to Core1

* Multiple message types
  * Set Brew Set Point (Non compensated)
  * Set Brew PID Parameters
  * Set Service Set Point
  * Set Service PID Parameters
  * Set Eco Mode (on/off)
  * Set Sleep mode (on/off)
  * Unbail
* Payloads
  * Float 1
    * Brew set point
    * Service boiler set point
    * Kp
  * Float 2
    * Ki
  * Float 3
    * Kd
  * Bool
    * Eco mode
    * Sleep mode

#### Reference material
(In no particular order)

* https://github.com/improv-wifi/sdk-cpp
* Random facts about the stock implementation:
  * Some implementation details are available in https://www.1st-line.com/wp-content/uploads/2018/12/Parameter-settings-technical-menu-Bianca-PL162T.pdf
    * Original parameters (not necessarily applicable since PID parameters are hightly implementation dependent):
      * Brew boiler
        * Kp: 0.8
        * Ki: 0.04
        * Kd: 12
        * On/off delta: 15°C
      * Service boiler
        * Kp: 20
        * Ki: 0
        * Kd: 20
        * On/off delta: 0
      * Brew boiler offset: 10°C
    * A "full" heatup cycle goes to 130°C first, then drops to set temperature
      * Triggered when the temperature on power on is less than 70°C
    * Service boiler is not PID controlled (even though it can be). Even if it *were*, having a Ki of 0, means it's a PD controller. It's a simple on/off controller.
  * Minimum on/off-time seems to be around 100-120 ms
  * When in eco mode, a warmup consists of running the coffee boiler full blast, holding the temperature around 130°C for 4 minutes, then dropping to the set point
  * When in regular mode, there is a kind of time slot system for which boiler is used. The time slots are 1 second wide.

## Licensing

The firmware is MIT licensed (excepting dependencies, which have their own, compatible licenses), and the hardware is CERN-OHL-P.
