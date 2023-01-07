cmake_minimum_required(VERSION 3.19)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

#set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g2 -O0 -ggdb2")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

#set(CMAKE_C_FLAGS "-Wall -Wextra")
set(CMAKE_C_FLAGS_DEBUG "-g2 -O0 -ggdb2")
set(CMAKE_C_FLAGS_RELEASE "-O3")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# initialize pico-sdk from GIT
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_FETCH_FROM_GIT on)

# pico_sdk_import.cmake is a single file copied from this SDK
# note: this must happen before project()
include(pico_sdk_import.cmake)

set(PICO_BOARD "arduino_nano_rp2040_connect")
set(PICO_CXX_ENABLE_EXCEPTIONS 1)

project(smart_lcc)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

set(U8G2_ADD_INCLUDEDIRS lib/u8g2/csrc)
set(U8G2_SRCS
        "lib/u8g2/csrc/u8x8_setup.c"
        "lib/u8g2/csrc/u8x8_u8toa.c"
        "lib/u8g2/csrc/u8x8_d_ssd1306_96x16.c"
        "lib/u8g2/csrc/u8g2_d_memory.c"
        "lib/u8g2/csrc/u8g2_selection_list.c"
        "lib/u8g2/csrc/u8g2_hvline.c"
        "lib/u8g2/csrc/u8x8_d_ssd1306_128x64_noname.c"
        "lib/u8g2/csrc/u8x8_message.c"
        "lib/u8g2/csrc/u8x8_d_ssd1306_48x64.c"
        "lib/u8g2/csrc/u8x8_d_ssd1305.c"
        "lib/u8g2/csrc/u8g2_kerning.c"
        "lib/u8g2/csrc/u8x8_input_value.c"
        "lib/u8g2/csrc/u8x8_d_st7586s_s028hn118a.c"
        "lib/u8g2/csrc/u8g2_polygon.c"
        "lib/u8g2/csrc/u8x8_d_uc1604.c"
        "lib/u8g2/csrc/u8x8_d_ssd1306_128x32.c"
        "lib/u8g2/csrc/u8x8_d_ssd1322.c"
        "lib/u8g2/csrc/u8x8_d_sbn1661.c"
        "lib/u8g2/csrc/u8g2_input_value.c"
        "lib/u8g2/csrc/u8x8_8x8.c"
        "lib/u8g2/csrc/u8x8_debounce.c"
        "lib/u8g2/csrc/u8x8_d_uc1638.c"
        "lib/u8g2/csrc/u8g2_setup.c"
        "lib/u8g2/csrc/u8x8_d_ist3020.c"
        "lib/u8g2/csrc/u8x8_display.c"
        "lib/u8g2/csrc/u8x8_d_ssd1317.c"
        "lib/u8g2/csrc/u8x8_d_ks0108.c"
        "lib/u8g2/csrc/u8g2_box.c"
        "lib/u8g2/csrc/u8x8_selection_list.c"
        "lib/u8g2/csrc/u8x8_d_ssd1326.c"
        "lib/u8g2/csrc/u8x8_d_st75256.c"
        "lib/u8g2/csrc/u8x8_d_ssd1309.c"
        "lib/u8g2/csrc/u8x8_byte.c"
        "lib/u8g2/csrc/u8x8_d_ssd1607_200x200.c"
        "lib/u8g2/csrc/u8x8_d_pcd8544_84x48.c"
        "lib/u8g2/csrc/u8log.c"
        "lib/u8g2/csrc/u8x8_d_sh1107.c"
        "lib/u8g2/csrc/u8log_u8x8.c"
        "lib/u8g2/csrc/u8g2_message.c"
        "lib/u8g2/csrc/u8x8_d_uc1610.c"
        "lib/u8g2/csrc/u8x8_d_sh1106_72x40.c"
        "lib/u8g2/csrc/u8g2_line.c"
        "lib/u8g2/csrc/u8g2_intersection.c"
        "lib/u8g2/csrc/u8g2_fonts.c"
        "lib/u8g2/csrc/u8x8_d_sh1106_64x32.c"
        "lib/u8g2/csrc/u8x8_d_sed1330.c"
        "lib/u8g2/csrc/u8x8_d_pcf8814_hx1230.c"
        "lib/u8g2/csrc/u8x8_d_il3820_296x128.c"
        "lib/u8g2/csrc/u8x8_d_uc1701_dogs102.c"
        "lib/u8g2/csrc/u8g2_cleardisplay.c"
        "lib/u8g2/csrc/u8x8_d_ssd1325.c"
        "lib/u8g2/csrc/u8g2_font.c"
        "lib/u8g2/csrc/u8x8_d_uc1601.c"
        "lib/u8g2/csrc/u8x8_d_pcf8812.c"
        "lib/u8g2/csrc/u8x8_cad.c"
        "lib/u8g2/csrc/u8x8_d_uc1611.c"
        "lib/u8g2/csrc/u8x8_string.c"
        "lib/u8g2/csrc/u8g2_ll_hvline.c"
        "lib/u8g2/csrc/u8x8_fonts.c"
        "lib/u8g2/csrc/u8x8_d_ld7032_60x32.c"
        "lib/u8g2/csrc/u8x8_d_max7219.c"
        "lib/u8g2/csrc/u8x8_d_st7588.c"
        "lib/u8g2/csrc/u8x8_d_ssd1606_172x72.c"
        "lib/u8g2/csrc/u8x8_d_st7586s_erc240160.c"
        "lib/u8g2/csrc/u8x8_d_stdio.c"
        "lib/u8g2/csrc/u8x8_d_sh1122.c"
        "lib/u8g2/csrc/u8log_u8g2.c"
        "lib/u8g2/csrc/u8g2_circle.c"
        "lib/u8g2/csrc/u8x8_d_ssd1306_64x48.c"
        "lib/u8g2/csrc/u8g2_buffer.c"
        "lib/u8g2/csrc/u8g2_bitmap.c"
        "lib/u8g2/csrc/u8x8_d_lc7981.c"
        "lib/u8g2/csrc/u8x8_d_st7920.c"
        "lib/u8g2/csrc/u8x8_d_t6963.c"
        "lib/u8g2/csrc/u8x8_d_st7567.c"
        "lib/u8g2/csrc/u8x8_d_ls013b7dh03.c"
        "lib/u8g2/csrc/u8x8_d_ssd1327.c"
        "lib/u8g2/csrc/u8x8_d_sh1108.c"
        "lib/u8g2/csrc/u8x8_gpio.c"
        "lib/u8g2/csrc/u8x8_d_uc1608.c"
        "lib/u8g2/csrc/u8x8_d_uc1617.c"
        "lib/u8g2/csrc/u8x8_d_ssd1329.c"
        "lib/u8g2/csrc/u8x8_d_ssd1306_64x32.c"
        "lib/u8g2/csrc/u8g2_d_setup.c"
        "lib/u8g2/csrc/u8x8_d_uc1701_mini12864.c"
        "lib/u8g2/csrc/u8x8_u16toa.c"
        "lib/u8g2/csrc/u8x8_d_a2printer.c"
        "lib/u8g2/csrc/u8x8_d_st7565.c"
        "lib/u8g2/csrc/u8x8_capture.c")

add_executable(smart_lcc
        ${U8G2_SRCS}
        src/smart_lcc.cpp
        src/double_tap.cpp
        src/u8g2functions.c
        src/SystemStatus.cpp
        src/SystemSettings.cpp
        src/utils/checksum.cpp
        src/utils/hex_format.cpp
        src/utils/polymath.cpp
        src/utils/triplet.cpp
        src/MulticoreSupport.cpp src/MulticoreSupport.h
        src/Controller/Core0/control_board_protocol.cpp
        src/Controller/Core0/HybridController.cpp
        src/Controller/Core0/HysteresisController.cpp
        src/Controller/Core0/lcc_protocol.cpp
        src/Controller/Core0/PIDController.cpp
        src/Controller/Core0/SafePacketSender.cpp
        src/Controller/Core0/SystemController.cpp
        src/Controller/Core0/TimedLatch.cpp
        src/Controller/Core0/Utils/UartReadBlockingTimeout.h
        src/Controller/Core1/UIController.cpp
        lib/slip/slip.cpp lib/slip/slip.h
        src/Controller/SingleCore/EspBootloader.cpp src/Controller/SingleCore/EspBootloader.h
        src/Controller/SingleCore/EspFirmware.cpp src/Controller/SingleCore/EspFirmware.h
        src/Controller/Core0/Utils/ClearUartCruft.h
        stub/stub.h
        esphome-output/boot_app0.bin.h
        esphome-output/bootloader.bin.h
        esphome-output/firmware.bin.h
        esphome-output/partitions.bin.h
        esphome/esp-protocol.h
        src/utils/crc32.cpp src/utils/crc32.h)

target_include_directories(smart_lcc PUBLIC lib/u8g2/csrc lib/slip lib/Ring-Buffer src stub esphome esphome-output xbm)

# pull in common dependencies
target_link_libraries(smart_lcc pico_stdlib)

target_link_libraries(smart_lcc
        hardware_spi
        hardware_i2c
        hardware_dma
        pico_multicore
        hardware_clocks
        )

# enable usb output, disable uart output
pico_enable_stdio_usb(smart_lcc 1)
pico_enable_stdio_uart(smart_lcc 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(smart_lcc)

pico_set_linker_script(smart_lcc ${CMAKE_SOURCE_DIR}/memmap.ld)

add_custom_target(Upload-JLink COMMAND /Applications/SEGGER/JLink/JLinkExe -device RP2040_M0_0 -CommandFile ../upload.jlink DEPENDS smart_lcc)

add_custom_target(__DUMMY COMMAND echo foo)