cmake_minimum_required(VERSION 3.13)

# Set board type explicitly
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Set the path to the Pico SDK and Picotool (using submodules)
set(PICO_SDK_PATH ${CMAKE_CURRENT_LIST_DIR}/pico-sdk)
set(PICOTOOL_FETCH_FROM_GIT_PATH ${CMAKE_CURRENT_LIST_DIR}/picotool)

# Include the Pico SDK
include(pico_sdk_import.cmake)

project(PicoOpenTherm C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

# Add the main executable
add_executable(blink_pio
    src/blink.c
)

# Generate the PIO header files
pico_generate_pio_header(blink_pio ${CMAKE_CURRENT_LIST_DIR}/src/blink.pio)
pico_generate_pio_header(blink_pio ${CMAKE_CURRENT_LIST_DIR}/src/opentherm_write.pio)
pico_generate_pio_header(blink_pio ${CMAKE_CURRENT_LIST_DIR}/src/opentherm_read.pio)

# Link the necessary libraries
target_link_libraries(blink_pio
    pico_stdlib
    pico_cyw43_arch_none
    hardware_pio
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(blink_pio 1)
pico_enable_stdio_uart(blink_pio 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(blink_pio)

# Add OpenTherm test executable
add_executable(opentherm_test
    src/opentherm_test.c
)

# Generate the PIO header files for OpenTherm
pico_generate_pio_header(opentherm_test ${CMAKE_CURRENT_LIST_DIR}/src/opentherm_write.pio)
pico_generate_pio_header(opentherm_test ${CMAKE_CURRENT_LIST_DIR}/src/opentherm_read.pio)

# Link the necessary libraries
target_link_libraries(opentherm_test
    pico_stdlib
    hardware_pio
)

# Enable USB output, disable UART output
pico_enable_stdio_usb(opentherm_test 1)
pico_enable_stdio_uart(opentherm_test 0)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(opentherm_test)
