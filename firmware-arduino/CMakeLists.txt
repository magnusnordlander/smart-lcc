cmake_minimum_required(VERSION 3.19)
project(firmware_arduino)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(z_dummy
        firmware-arduino.cpp double_tap.cpp src/NetworkController.cpp src/NetworkController.h)
set_target_properties(z_dummy PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(z_dummy PRIVATE
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/cores/rp2040
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/libraries/LittleFS/src
        arduino/user/libraries/WiFiManager_NINA_Lite/src
        arduino/user/libraries/WiFiNINA_Generic/src
        arduino/user/libraries/Adafruit_MQTT_Library
        arduino/user/libraries/ArduinoOTA/src
        )
target_compile_definitions(z_dummy PUBLIC ARDUINO_NANO_RP2040_CONNECT)

add_custom_target(install-libraries COMMAND
        arduino-cli lib install Functional-Vlpp@1.0.2 --config-file ../arduino-config.yaml &&
        arduino-cli lib install WiFiNINA_Generic@1.8.14-3 --config-file ../arduino-config.yaml &&
        arduino-cli lib install WiFiWebServer@1.5.5-1 --config-file ../arduino-config.yaml &&
        arduino-cli lib install DoubleResetDetector_Generic@1.8.0 --config-file ../arduino-config.yaml &&
        arduino-cli lib install FlashStorage_SAMD@1.3.2 --config-file ../arduino-config.yaml &&
        arduino-cli lib install FlashStorage_STM32@1.2.0 --config-file ../arduino-config.yaml &&
        arduino-cli lib install WiFiManager_NINA_Lite@1.6.1 --no-deps --config-file ../arduino-config.yaml &&
        arduino-cli lib install "Adafruit MQTT Library@2.4.2" --config-file ../arduino-config.yaml &&
        rm -rf ../arduino/user/libraries/ArduinoOTA &&
        mkdir ../arduino/user/libraries/ArduinoOTA &&
        git clone https://github.com/jandrassy/ArduinoOTA.git ../arduino/user/libraries/ArduinoOTA/
        )
add_custom_target(install-core COMMAND arduino-cli core install rp2040:rp2040@1.9.14 --config-file ../arduino-config.yaml)
add_custom_target(compile COMMAND
        arduino-cli compile
        -v
        -b rp2040:rp2040:arduino_nano_connect
        --build-property \"upload.maximum_size=12578816\"
        --build-property \"build.flash_length=12578816\"
        --build-property \"build.eeprom_start=285208576\"
        --build-property \"build.fs_start=281014272\"
        --build-property \"build.fs_end=285208576\"
        ${CMAKE_SOURCE_DIR}
        -e --output-dir ${CMAKE_BINARY_DIR}/build
        --config-file ../arduino-config.yaml
        )
#         --build-property \"build.debug_port=-DDEBUG_RP2040_PORT=Serial\"
# build.debug_level=-DDEBUG_RP2040_WIRE -DDEBUG_RP2040_SPI -DDEBUG_RP2040_CORE
add_custom_target(compile-and-copy DEPENDS compile COMMAND bash -c \"if [ -d /Volumes/RPI-RP2 ]\; then cp ${CMAKE_BINARY_DIR}/build/firmware-arduino.ino.uf2 /Volumes/RPI-RP2/ \; else echo Pico not mounted\; fi\")