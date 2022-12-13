# Smart LCC

[![Join the chat at https://gitter.im/smart-lcc/community](https://badges.gitter.im/smart-lcc/community.svg)](https://gitter.im/smart-lcc/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

A smarter LCC for the Lelit Bianca V2, with Wi-Fi and stuff.

Based on the protocol dissection I'm doing in [magnusnordlander/lelit-bianca-protocol](https://github.com/magnusnordlander/lelit-bianca-protocol).

## Status

Consider this project beta quality.

The revision B PCB works well. Manufacturing files are available in the `pcb/` directory. Unless you're *very* good at soldering fine pitch by hand, I suggest manufacturing with SMT assembly. I've used JLCPCB for my manufacturing runs, and the files have worked well there. I do have some spares of both the Rev A PCB (just the PCB) and the Rev B PCB (the difficult components already soldered, but does need a connector, buttons, a display and an Arduino). If you have experience of embedded programming, want to help with the project and you want one of the PCBs, reach out.

I am considering making a Rev C PCB that doesn't use an Arduino, but rather RP2040 and ESP32 chips directly. This would allow for a relocated display connector (similar to the original LCC) and a better antenna, which is useful since the board is located inside a metal enclosure. We'll have to see though.

Considering this plugs in to an expensive machine it bears to mention: Anything you do with this, you do at your own risk. Components have been fried already during the course of this project. Your machine uses both line voltage power, high pressured hot water, steam and other dangerous components. There is a risk of both damaging the machine, personal injury and property damage, the liability for which you assume yourself. This is not the stage to get on board with this project if you aren't willing to deal with those risks. 

### A note on Bianca versions

This project has only been tested on a Bianca V2. There are (at the time of writing) three versions of the Bianca, V1, V2, and V3. To my understanding it should work on a Bianca V1, but it's untested. As for the Bianca V3, it features upgraded hardware (a different solenoid valve with high/low flow signals), which this project doesn't support. Since I don't have access to a Bianca V3 (for reverse engineering or otherwise), support is currently not on the roadmap.

The LCC-relevant hardware differences between a V2 and a V3 Bianca (to my understanding) are the following:

* A new solenoid to control full/low flow from the pump (upgradable via the V3 upgrade kit)
* The power light now an LED and software controlled (not a part of the V3 upgrade kit)

Most notably, the Gicar box is the same, which is very interesting.

Like I said, I do not have access to a Bianca V3, so reverse engineering the protocol changes for the V3 would be difficult, and validation would be impossible. That being said, if someone were to reverse engineer the protocol changes, a new version of this firmware could be made that would be backwards-compatible with the Bianca V2. That is absolutely an interesting proposition.

## Project goals

Create a smart replacement for the "LCC" PID module used in the Lelit Bianca with open source hardware and software.

## Architecture

### Hardware
This project is using the Arduino RP2040 Connect, a dual core ARM Cortex-M0+ board with a Ublox Nina W102 module.

A custom PCB has been designed (see `pcb/`) that is designed to fit in the original LCC enclosure. A BOM for components is also available. Aside from the Arduino RP2040 Connect, the PCB has push buttons for + and -, a resistor divider for the 5V Control Board UART signal, and a flat flex connector for a SSD1309 based OLED. The PCB has been designed with surface mount components, but uses relatively large 1206 caps and resistors, which are somewhat simple to hand solder.

#### Possible future revision
I have been considering a new hardware revision, which instead of using an Arduino RP2040 Connect, uses just a plain RP2040, and a plain ESP32 (of some kind, though if I know myself, it'll probably be an ESP32-S3).

The main motivations are the following: 

* Allowing a better WiFi antenna (the Bianca is basically a faraday cage, so a better antenna would be beneficial)
* Allowing for separate programming ports on the board for the ESP32 and the RP2040
* Possibly attaching the RP2040 QSPI Flash to the ESP32 as well, for easier OTA updates
* Allowing for a cutout for the display ribbon cable, similar to the cutout on the original LCC board

There's also a couple of more errata like things I would add in a new revision:

* Use just a Zener instead of resistor divider to shift the 5V UART to 3V3

This being said, I haven't made up my mind about whether to do a future revision at all.

### Firmware (1.0.0, current)
The project is using arduino-cli, with earlephilhower/arduino-pico. Core 0 runs UI and external communications, and will communicate with core 1 through `pico_util/queue`. Core1 does not *store* it's parameters. Core0 reads them from LittleFS, and passes them to Core1 through regular message passing.

#### Usage
Hold the plus button while booting to trigger configuration mode. In configuration mode, a safe packet is continually sent to the control board, and a WiFi access point is created, with the SSID `LCC-XXXXXX`, where `XXXXXX` is the last 6 hex digits of the unit's MAC address. It is password protected, using the same password as the SSID, (i.e. `LCC-XXXXXX`). Therein you can configure your WiFi network and MQTT settings.

Hold the minus button while booting to trigger OTA mode. As in configuration mode, a safe packet is continually sent to the control board. It connects to the WiFi configured in configuration mode. Use ArduinoOTA to send new firmware. The controller doesn't automatically reboot after OTA, so after the OTA is complete and the display goes blank, manually cycle power.

#### RP2040 Core 0
* UI controller
* External communication

#### RP2040 Core 1
* System controller
  * Safety critical, uses the entire core for itself
  * Communicates with the Control Board
  * Performs a safety check, ensuring that temperatures in the boilers never exceed safe limits, and that both boilers are never running simultaneously.
  * Responsible for kicking the watchdog
    * Reboots if Core 0 isn't handling its messages (i.e. has crashed or is frozen)
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

### Firmware 1.1.0
In this firmware, Core0 and Core1 will swap functions.

### Firmware 2.0.0
In this firmware, a custom firmware for the Ublox Nina W102 will be developed to handle network communication.

### Reference material
(In no particular order)

* https://github.com/improv-wifi/sdk-cpp
* https://www.youtube.com/watch?v=fAEDHOUJCEo
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
  * Shot saving seems to be implemented by simply blocking new brews from starting when the tank is empty. There's no fanciness with allowing it to run for a certain amount of time or anything.
  * It takes about 3 seconds for the LCC to pick up on an empty water tank (presumably to debounce the signal)

## Licensing

The firmware is MIT licensed (excepting dependencies, which have their own, compatible licenses), and the hardware is CERN-OHL-P.
