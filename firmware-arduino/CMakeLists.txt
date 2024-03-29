cmake_minimum_required(VERSION 3.19)
project(firmware_arduino)

set(CMAKE_CXX_STANDARD 14)

include_directories(
        .
        src
)

add_executable(z_dummy
        firmware-arduino.cpp
        double_tap.cpp
        src/NetworkController.cpp
        src/SystemStatus.cpp
        src/SystemSettings.cpp
        src/utils/checksum.cpp
        src/utils/hex_format.cpp
        src/utils/polymath.cpp
        src/utils/triplet.cpp
        src/UIController.cpp
        src/SystemController/control_board_protocol.cpp
        src/SystemController/HybridController.cpp
        src/SystemController/HysteresisController.cpp
        src/SystemController/lcc_protocol.cpp
        src/SystemController/PIDController.cpp
        src/SystemController/SystemController.cpp
        src/SystemController/TimedLatch.cpp
        src/SafePacketSender.cpp src/SafePacketSender.h
        src/MemoryFree.cpp src/MemoryFree.h
        src/FileIO.cpp src/FileIO.h
        src/xbm/bssr_on.h src/xbm/eco_mode.h src/xbm/no_water.h src/xbm/sssr_on.h src/xbm/wifi_mqtt.h src/xbm/wifi_no_mqtt.h src/xbm/pump_on.h src/xbm/cup_no_smoke.h src/xbm/cup_smoke_1.h src/xbm/cup_smoke_2.h src/AutomationController.cpp src/AutomationController.h)
set_target_properties(z_dummy PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(z_dummy PRIVATE
        arduino/user/libraries/Functional-Vlpp/src
        arduino/user/libraries/WiFiWebServer/src
        arduino/user/libraries/WiFiNINA_Generic/src
        arduino/user/libraries/PubSubClient/src
        arduino/user/libraries/ArduinoOTA/src
        arduino/user/libraries/ArduinoJson/src
        arduino/user/libraries/CRC32/src
        arduino/user/libraries/U8g2/src
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/cores/rp2040
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/libraries/LittleFS/src
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/variants/arduino_nano_connect
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/common/pico_base/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/common/pico_time/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/common/pico_util/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/rp2_common/hardware_sync/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/rp2_common/hardware_watchdog/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/rp2_common/hardware_uart/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/rp2_common/hardware_gpio/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/rp2_common/hardware_base/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/rp2_common/hardware_timer/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/rp2_common/hardware_irq/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/rp2_common/hardware_adc/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/rp2040/hardware_regs/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/rp2040/hardware_structs/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/host/pico_platform/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/host/pico_multicore/include
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/lib/tinyusb/src
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/pico-sdk/src/common/pico_sync/include
        )
target_compile_definitions(z_dummy PUBLIC ARDUINO_NANO_RP2040_CONNECT U8X8_USE_PINS PUBSUB_USE_FUNCTIONAL)

add_custom_target(install-libraries COMMAND
        arduino-cli lib install WiFiNINA_Generic@1.8.14-3 --config-file ../arduino-config.yaml &&
        arduino-cli lib install WiFiWebServer@1.5.5-1 --config-file ../arduino-config.yaml &&
        arduino-cli lib install CRC32@2.0.0 --config-file ../arduino-config.yaml &&
        arduino-cli lib install ArduinoJson@6.19.1  --config-file ../arduino-config.yaml &&
        arduino-cli lib install U8g2@2.31.2  --config-file ../arduino-config.yaml &&
        arduino-cli lib install PubSubClient@2.8.0  --config-file ../arduino-config.yaml &&
        rm -rf ../arduino/user/libraries/PubSubClient &&
        mkdir ../arduino/user/libraries/PubSubClient &&
        git clone https://github.com/magnusnordlander/pubsubclient.git ../arduino/user/libraries/PubSubClient/ &&
        arduino-cli lib install ArduinoOTA@1.0.8  --config-file ../arduino-config.yaml
        )
add_custom_target(install-core COMMAND
        arduino-cli core install rp2040:rp2040@1.9.14 --config-file ../arduino-config.yaml &&
        patch -p0 -i main.patch &&
        arduino-cli core install arduino:avr --config-file ../arduino-config.yaml)
add_custom_target(compile COMMAND
        arduino-cli compile
        -v
        -b rp2040:rp2040:arduino_nano_connect
        --build-property \"upload.maximum_size=12578816\"
        --build-property \"build.flash_length=12578816\"
        --build-property \"build.eeprom_start=285208576\"
        --build-property \"build.fs_start=281014272\"
        --build-property \"build.fs_end=285208576\"
        --build-property \"compiler.cpp.extra_flags=-DPUBSUB_USE_FUNCTIONAL=\"
        ${CMAKE_SOURCE_DIR}
        -e --output-dir ${CMAKE_BINARY_DIR}/build
        --config-file ../arduino-config.yaml
        )

add_custom_target(compile-dev COMMAND
        arduino-cli compile
        -v
        --build-property \"upload.maximum_size=12578816\"
        --build-property \"build.flash_length=12578816\"
        --build-property \"build.eeprom_start=285208576\"
        --build-property \"build.fs_start=281014272\"
        --build-property \"build.fs_end=285208576\"
        --build-property \"compiler.cpp.extra_flags=-DPUBSUB_USE_FUNCTIONAL=\"
        ${CMAKE_SOURCE_DIR}
        -e --output-dir ${CMAKE_BINARY_DIR}/build
        --config-file ../arduino-config.yaml
        --profile arduino_nano_connect

        )

add_custom_target(compile-usb-debug COMMAND
        arduino-cli compile
        -v
        -b rp2040:rp2040:arduino_nano_connect
        --build-property \"upload.maximum_size=12578816\"
        --build-property \"build.flash_length=12578816\"
        --build-property \"build.eeprom_start=285208576\"
        --build-property \"build.fs_start=281014272\"
        --build-property \"build.fs_end=285208576\"
        --build-property \"build.debug_port=-DDEBUG_RP2040_PORT=Serial\"
        --build-property \"build.debug_level=-DDEBUG_RP2040_CORE\"
        --build-property \"compiler.cpp.extra_flags=-DPUBSUB_USE_FUNCTIONAL= -DPICO_USE_STACK_GUARDS= -DKH_WIFININA_SPI_DEBUG=2\"
        ${CMAKE_SOURCE_DIR}
        -e --output-dir ${CMAKE_BINARY_DIR}/build
        --config-file ../arduino-config.yaml
        )

add_custom_target(copy COMMAND bash -c \"if [ -d /Volumes/RPI-RP2 ]\; then cp ${CMAKE_BINARY_DIR}/build/firmware-arduino.ino.uf2 /Volumes/RPI-RP2/ \; else echo Pico not mounted\; fi\")

add_custom_target(compile-and-copy DEPENDS compile COMMAND bash -c \"if [ -d /Volumes/RPI-RP2 ]\; then cp ${CMAKE_BINARY_DIR}/build/firmware-arduino.ino.uf2 /Volumes/RPI-RP2/ \; else echo Pico not mounted\; fi\")
add_custom_target(compile-usb-debug-and-copy DEPENDS compile-usb-debug COMMAND bash -c \"if [ -d /Volumes/RPI-RP2 ]\; then cp ${CMAKE_BINARY_DIR}/build/firmware-arduino.ino.uf2 /Volumes/RPI-RP2/ \; else echo Pico not mounted\; fi\")

add_custom_target(upload-ota-prod COMMAND
        mkdir -p ../prod-ota-uploads && cp ${CMAKE_BINARY_DIR}/build/firmware-arduino.ino.uf2 ../prod-ota-uploads/`date +%Y%m%dT%H%M%S`.uf2 &&
        python3 ../uf2conv.py ${CMAKE_BINARY_DIR}/build/firmware-arduino.ino.uf2 -o ${CMAKE_BINARY_DIR}/build/firmware-arduino.ino.bin &&
        ../arduinoOTA/arduinoOTA -address 192.168.11.83 -port 65280 -username arduino -password password -sketch ${CMAKE_BINARY_DIR}/build/firmware-arduino.ino.bin -upload /sketch -b)
add_custom_target(upload-ota-dev-1 COMMAND
        python3 ../uf2conv.py ${CMAKE_BINARY_DIR}/build/firmware-arduino.ino.uf2 -o ${CMAKE_BINARY_DIR}/build/firmware-arduino.ino.bin &&
        ../arduinoOTA/arduinoOTA -address 192.168.20.86 -port 65280 -username arduino -password password -sketch ${CMAKE_BINARY_DIR}/build/firmware-arduino.ino.bin -upload /sketch -b)
