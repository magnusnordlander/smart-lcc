cmake_minimum_required(VERSION 3.12)


if(NOT TEST_RIG)
    # Pull in SDK (must be before project)
    include(pico_sdk_import.cmake)
endif()

project(lcc-relay C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if(NOT TEST_RIG)
set(PICO_EXAMPLES_PATH ${PROJECT_SOURCE_DIR})

# Initialize the SDK
pico_sdk_init()
endif()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

add_executable(main
        main.cpp
        utils/polymath.cpp utils/polymath.h
        utils/hex_format.cpp utils/hex_format.h
        control_board_protocol.cpp control_board_protocol.h
        utils/triplet.cpp utils/triplet.h
        utils/checksum.cpp utils/checksum.h
        lcc_protocol.cpp lcc_protocol.h
        PIDController.cpp PIDController.h
        SystemController.cpp SystemController.h
        platform/platform.h platform/rp2040/platform.cpp
        )

add_executable(testrig
        main.cpp
        utils/polymath.cpp utils/polymath.h
        utils/hex_format.cpp utils/hex_format.h
        control_board_protocol.cpp control_board_protocol.h
        utils/triplet.cpp utils/triplet.h
        utils/checksum.cpp utils/checksum.h
        lcc_protocol.cpp lcc_protocol.h
        PIDController.cpp PIDController.h
        SystemController.cpp SystemController.h
        platform/platform.h platform/debug/platform.cpp
        platform/debug/emulator/WaterBoiler.h platform/debug/emulator/WaterBoiler.cpp
        platform/debug/emulator/BiancaControlBoard.h platform/debug/emulator/BiancaControlBoard.cpp
        )

if(NOT TEST_RIG)

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(main pico_stdlib)

target_compile_options(main PUBLIC -Wall -Wextra -Wno-unused-function -Wno-unused-parameter)
target_compile_definitions(main PUBLIC DEBUG)

target_link_libraries(main
        hardware_clocks
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(main)
endif()