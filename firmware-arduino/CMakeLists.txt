cmake_minimum_required(VERSION 3.19)
project(firmware_arduino)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(z_dummy
        firmware-arduino.cpp)
set_target_properties(z_dummy PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(z_dummy PRIVATE
        arduino/data/packages/rp2040/hardware/rp2040/1.9.14/cores/rp2040
        arduino/user/libraries/WiFiManager_NINA_Lite/src
        arduino/user/libraries/WiFiNINA_Generic/src
        arduino/user/libraries/Adafruit_MQTT_Library
        arduino/user/libraries/ArduinoOTA/src
        )

add_custom_target(install-libraries COMMAND
        arduino-cli lib install Functional-Vlpp@1.0.2 --config-file ../arduino-config.yaml &&
        arduino-cli lib install WiFiNINA_Generic@1.8.14-3 --config-file ../arduino-config.yaml &&
        arduino-cli lib install WiFiWebServer@1.5.5-1 --config-file ../arduino-config.yaml &&
        arduino-cli lib install DoubleResetDetector_Generic@1.8.0 --config-file ../arduino-config.yaml &&
        arduino-cli lib install FlashStorage_SAMD@1.3.2 --config-file ../arduino-config.yaml &&
        arduino-cli lib install FlashStorage_STM32@1.2.0 --config-file ../arduino-config.yaml &&
        arduino-cli lib install WiFiManager_NINA_Lite@1.6.1 --no-deps --config-file ../arduino-config.yaml &&
        arduino-cli lib install "Adafruit MQTT Library@2.4.2" --config-file ../arduino-config.yaml &&
        arduino-cli lib install ArduinoOTA@1.0.7 --config-file ../arduino-config.yaml
        )
add_custom_target(install-core COMMAND arduino-cli core install rp2040:rp2040@1.9.14 --config-file ../arduino-config.yaml)
add_custom_target(compile COMMAND
        arduino-cli compile -v -b rp2040:rp2040:arduino_nano_connect ${CMAKE_SOURCE_DIR} -e --output-dir ${CMAKE_BINARY_DIR}/build  --config-file ../arduino-config.yaml
        )