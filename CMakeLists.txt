# PicoWi project, see http://iosoft.blog/picowi for details

cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)
pico_sdk_init()
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(picowi C CXX ASM)
set(CMAKE_C_STANDARD 11)

# Set to 0 for Pico-W CYW43439, 1 for Murata 1DX (CYW4343W)
set (CHIP_4343W 0)

# Picowi library souce files
set (PICOWI_SRCE       lib/picowi_pico.c  lib/picowi_init.c
    lib/picowi_spi.c   lib/picowi_ioctl.c lib/picowi_scan.c 
    lib/picowi_event.c lib/picowi_join.c  lib/picowi_pio.c
    lib/picowi_ip.c)

# Firmware file for CYW43439 or CYW4343W
if (${CHIP_4343W})
    message (STATUS "Building CYW4343W version")
    set (FW_FILE firmware/fw_4343w.c)
    add_compile_options(-DCHIP_4343W)
else ()
    message (STATUS "Building Pico-W CYW43439 version")
    set (FW_FILE firmware/fw_43439.c)
endif ()

# Enable compiler warnings
add_compile_options(-Wall)

# Create picowi library
add_library(picowi ${PICOWI_SRCE} ${FW_FILE})
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/lib/picowi_pio.pio)
target_link_libraries(picowi pico_stdlib hardware_pio)

# Create 'blink' executable
add_executable(blink blink.c)
target_link_libraries(blink picowi pico_stdlib hardware_pio)
pico_add_extra_outputs(blink)

# Create 'scan' executable
add_executable(scan scan.c)
target_link_libraries(scan picowi pico_stdlib hardware_pio)
pico_add_extra_outputs(scan)

# Create 'join' executable
add_executable(join join.c)
target_link_libraries(join picowi pico_stdlib hardware_pio)
pico_add_extra_outputs(join)

# Create 'ping' executable
add_executable(ping ping.c)
target_link_libraries(ping picowi pico_stdlib hardware_pio)
pico_add_extra_outputs(ping)

# EOF
