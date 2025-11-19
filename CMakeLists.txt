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
    include(ExternalProject)

    # Prefer system host compilers for the external host build
    find_program(HOST_GCC NAMES gcc)
    find_program(HOST_GXX NAMES g++)
    if(NOT HOST_GCC)
        set(HOST_GCC "")
    endif()
    if(NOT HOST_GXX)
        set(HOST_GXX "")
    endif()

    # Prepare optional host configure args if native compilers were found
    if(HOST_GCC AND HOST_GXX)
        set(HOST_CONFIGURE_ARGS -DCMAKE_C_COMPILER=${HOST_GCC} -DCMAKE_CXX_COMPILER=${HOST_GXX})
    else()
        set(HOST_CONFIGURE_ARGS)
    endif()

    ExternalProject_Add(host_simulator_ep
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/host
        BINARY_DIR ${CMAKE_BINARY_DIR}/host-external
        CONFIGURE_COMMAND ${CMAKE_COMMAND} -S ${CMAKE_SOURCE_DIR}/host -B <BINARY_DIR> -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} ${HOST_CONFIGURE_ARGS}
        BUILD_COMMAND ${CMAKE_COMMAND} --build <BINARY_DIR> --target host_simulator -- -j
        INSTALL_COMMAND ""
    )

    add_custom_target(host_simulator DEPENDS host_simulator_ep)
endif()

# Add OpenTherm Simulator executable (uses main.cpp with USE_SIMULATOR flag)
add_executable(picoopentherm_simulator
    src/main.cpp
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

# Define USE_SIMULATOR preprocessor flag for simulator build
target_compile_definitions(picoopentherm_simulator PRIVATE USE_SIMULATOR)

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
