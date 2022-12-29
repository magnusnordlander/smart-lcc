#!/bin/bash

/Applications/SEGGER/JLink/JLinkGDBServerCLExe -select USB -device RP2040_M0_1 -endian little -if SWD -speed 4000 -noir -noLocalhostOnly -nologtofile -port 2331 -SWOPort 2332 -TelnetPort 2333 -singlerun -x /Users/magnus/Developer/lcc-pico-sdk/.gdbinit