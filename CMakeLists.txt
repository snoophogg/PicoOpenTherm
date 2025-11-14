cmake_minimum_required(VERSION 3.13)

# Set board type explicitly
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Set the path to the Pico SDK and Picotool (using submodules)
set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/pico-sdk)
set(PICOTOOL_FETCH_FROM_GIT_PATH ${CMAKE_CURRENT_LIST_DIR}/picotool)

# Include the Pico SDK import from the SDK submodule
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(PicoOpenTherm C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Add pico-kvstore library path
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/pico-kvstore)

# Add OpenTherm test executable
add_executable(picoopentherm
    src/main.cpp
    src/opentherm.cpp
    src/opentherm_protocol.cpp
    src/opentherm_ha.cpp
    src/config.cpp
    src/mqtt_common.cpp
    src/mqtt_discovery.cpp
    src/led_blink.cpp
    src/mqtt_publish.cpp
    src/kvs_init_custom.c
)

# Generate the PIO header files for OpenTherm
pico_generate_pio_header(picoopentherm ${CMAKE_CURRENT_LIST_DIR}/src/opentherm_write.pio)
pico_generate_pio_header(picoopentherm ${CMAKE_CURRENT_LIST_DIR}/src/opentherm_read.pio)

# Add include directory for lwipopts.h
target_include_directories(picoopentherm PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/pico-kvstore/include
    ${CMAKE_CURRENT_LIST_DIR}/pico-kvstore/lib/pico-vfs/include
)

# Link the necessary libraries
target_link_libraries(picoopentherm
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_pio
    hardware_watchdog
    hardware_timer
    pico_lwip_mqtt
    kvstore
    kvstore_logkvs
    blockdevice_flash
    hardware_flash
    hardware_sync
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(picoopentherm 1)
pico_enable_stdio_uart(picoopentherm 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(picoopentherm)

# Optional host-native simulator target
option(BUILD_HOST_SIMULATOR "Build host-native simulator (requires libmosquitto)" OFF)
if(BUILD_HOST_SIMULATOR)
    add_executable(host_simulator
        src/host_simulator.cpp
        src/simulated_opentherm.cpp
        src/opentherm_protocol.cpp
    )

    target_include_directories(host_simulator PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src
    )

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(MOSQUITTO REQUIRED libmosquitto)
    target_include_directories(host_simulator PRIVATE ${MOSQUITTO_INCLUDE_DIRS})
    target_link_libraries(host_simulator ${MOSQUITTO_LIBRARIES} pthread)
endif()

# Add OpenTherm Simulator executable
add_executable(picoopentherm_simulator
    src/simulator.cpp
    src/simulated_opentherm.cpp
    src/opentherm_protocol.cpp
    src/opentherm_ha.cpp
    src/config.cpp
    src/mqtt_common.cpp
    src/mqtt_discovery.cpp
    src/led_blink.cpp
    src/mqtt_publish.cpp
    src/kvs_init_custom.c
)

# Add include directory for simulator
target_include_directories(picoopentherm_simulator PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/pico-kvstore/include
    ${CMAKE_CURRENT_LIST_DIR}/pico-kvstore/lib/pico-vfs/include
)

# Link the necessary libraries for simulator
target_link_libraries(picoopentherm_simulator
    pico_stdlib
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_watchdog
    hardware_timer
    pico_lwip_mqtt
    kvstore
    kvstore_logkvs
    blockdevice_flash
    hardware_flash
    hardware_sync
)

# Enable USB output for simulator
pico_enable_stdio_usb(picoopentherm_simulator 1)
pico_enable_stdio_uart(picoopentherm_simulator 0)

# Create map/bin/hex/uf2 files for simulator
pico_add_extra_outputs(picoopentherm_simulator)
