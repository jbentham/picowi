# PicoWi project, see http://iosoft.blog/picowi for details

# Set debug or release version
#set (RELEASE 1)

cmake_minimum_required(VERSION 3.12)
include($ENV{PICO_SDK_PATH}/pico_sdk_init.cmake)
pico_sdk_init()
include($ENV{PICO_SDK_PATH}/tools/CMakeLists.txt)

# Set name of project (as PROJECT_NAME) and C/C++ standards
project(picowi C CXX ASM)
include(PicoSDKConfig.cmake)
set(CMAKE_C_STANDARD 11)

# Set to 0 for Pico-W CYW43439, 1 for Murata 1DX (CYW4343W)
# It may be necessary to reduce PIO_SPI_FREQ when using an external device
#set (CHIP_4343W 1)

# Picowi library souce files
set (PICOWI_SRCE       lib/picowi_pico.c  lib/picowi_init.c
    lib/picowi_wifi.c  lib/picowi_ioctl.c lib/picowi_scan.c 
    lib/picowi_event.c lib/picowi_join.c  lib/picowi_pio.c
    lib/picowi_ip.c    lib/picowi_udp.c   lib/picowi_dhcp.c
    lib/picowi_dns.c   lib/picowi_net.c   lib/picowi_tcp.c
    lib/picowi_web.c)

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

# Use optimisation for release version
if (${RELEASE})
    add_compile_options(-O3 -DNDEBUG)
endif ()

# Create picowi library
add_library(picowi ${PICOWI_SRCE} ${FW_FILE})
pico_generate_pio_header(${PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/lib/picowi_pio.pio)
target_link_libraries(picowi pico_stdlib hardware_pio hardware_dma)

# Create 'blink' executable
add_executable(blink blink.c)
target_link_libraries(blink picowi pico_stdlib hardware_pio hardware_dma)
pico_add_extra_outputs(blink)

# Create 'scan' executable
add_executable(scan scan.c)
target_link_libraries(scan picowi pico_stdlib hardware_pio hardware_dma)
pico_add_extra_outputs(scan)

# Create 'join' executable
add_executable(join join.c)
target_link_libraries(join picowi pico_stdlib hardware_pio hardware_dma)
pico_add_extra_outputs(join)

# Create 'ping' executable
add_executable(ping ping.c)
target_link_libraries(ping picowi pico_stdlib hardware_pio hardware_dma)
pico_add_extra_outputs(ping)

# Create 'dhcp' executable
add_executable(dhcp dhcp.c)
target_link_libraries(dhcp picowi pico_stdlib hardware_pio hardware_dma)
pico_add_extra_outputs(dhcp)

# Create 'dns' executable
add_executable(dns dns.c)
target_link_libraries(dns picowi pico_stdlib hardware_pio hardware_dma)
pico_add_extra_outputs(dns)

# Create 'udp_socket_server' executable
add_executable(udp_socket_server udp_socket_server.c)
target_link_libraries(udp_socket_server picowi pico_stdlib hardware_pio hardware_dma)
pico_add_extra_outputs(udp_socket_server)

# Create 'web_server' executable
add_executable(web_server web_server.c)
target_link_libraries(web_server picowi pico_stdlib hardware_pio hardware_dma)
pico_add_extra_outputs(web_server)

# Create 'web_cam' executable
#add_executable(web_cam web_cam.c camera/cam_5642.c)
add_executable(web_cam web_cam.c camera/cam_2640.c)
target_link_libraries(web_cam picowi pico_stdlib hardware_i2c hardware_spi hardware_dma)
pico_add_extra_outputs(web_cam)

# EOF
