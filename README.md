# Smart LCC

A smarter LCC for the Lelit Bianca, with Wi-Fi and stuff.

Based on the protocol dissection I'm doing in [magnusnordlander/lelit-bianca-protocol](https://github.com/magnusnordlander/lelit-bianca-protocol).

## Status

Consider this project beta quality.

The revision B PCB works well. Manufacturing files are available in the `pcb/` directory. Unless you're *very* good at soldering fine pitch by hand, I suggest manufacturing with SMT assembly. I've used JLCPCB for my manufacturing runs, and the files have worked well there. I do have some spares of both the Rev A PCB (just the PCB) and the Rev B PCB (the difficult components already soldered, but does need a connector, buttons, a display and an Arduino). If you have experience of embedded programming, want to help with the project and you want one of the PCBs, reach out.

Considering this plugs in to an expensive machine it bears to mention: Anything you do with this, you do at your own risk. Components have been fried already during the course of this project. Your machine uses both line voltage power, high pressured hot water, steam and other dangerous components. There is a risk of both damaging the machine, personal injury and property damage, the liability for which you assume yourself. This is not the stage to get on board with this project if you aren't willing to deal with those risks. 

## Project goals

Create a smart replacement for the "LCC" PID module used in the Lelit Bianca with open source hardware and software.

## Architecture

### Hardware
This project is using the Arduino RP2040 Connect, a dual core ARM Cortex-M0+ board with a Ublox Nina W102 module (running an ESP32).

A custom PCB has been designed (see `pcb/`) that is designed to fit in the original LCC enclosure. A BOM for components is also available. Aside from the Arduino RP2040 Connect, the PCB has push buttons for + and -, a resistor divider for the 5V Control Board UART signal, and a flat flex connector for a SSD1309 based OLED. The PCB has been designed with surface mount components, but uses relatively large 1206 caps and resistors, which are somewhat simple to hand solder.

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
    * Idle
    * Heatup
    * Warm
    * Sleeping
    * Bailed
    * First run (fill all the boilers before heating them)
  * Bail reason
  * Brewing
  * Filling service boiler
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
  * Trigger first run
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

### Firmware R1B, normal mode
The project is using arduino-cli to build, and uses the earlephilhower/arduino-pico core. Anything system critical occurs on core0, which is responsible for communication with the Control Board. The Nina W102 module uses the Arduino WiFiNina firmware.

#### RP2040 Core 0
* System controller
  * Safety critical, after boot it uses the entire core for itself
  * Communicates with the Control Board
  * Performs a safety check, ensuring that temperatures in the boilers never exceed safe limits, and that both boilers are never running simultaneously.
  * Responsible for kicking the watchdog
    * Reboots if Core 1 isn't handling its messages (i.e. has crashed or is frozen)
  * Responsible for PID, keeping water in the boiler, running pumps etc.

#### RP2040 Core 1
* UI controller
* Network communication
  * MQTT

### Firmware R1B, OTA mode or configuration mode

In OTA mode or configuration mode, core 1 is not used. A safe packet is sent every second to the control board, though the timing of this may vary.

### Firmware R2A (Planned for 22Q2)
The project will start using custom code for the Nina W102. 

#### RP2040 Core 0
* System controller
  * Safety critical, after boot it uses the entire core for itself
  * Communicates with the Control Board
  * Performs a safety check, ensuring that temperatures in the boilers never exceed safe limits, and that both boilers are never running simultaneously.
  * Responsible for kicking the watchdog
    * Reboots if Core 1 isn't handling its messages (i.e. has crashed or is frozen)
  * Responsible for PID, keeping water in the boiler, running pumps etc.

#### RP2040 Core 1
* UI controller

#### W102 Core 1
* Communication with the RP2040

#### W102 Core 2
* Network communication
  * MQTT
  * HTTP

### Reference material
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
