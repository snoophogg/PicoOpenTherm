cmake_minimum_required(VERSION 3.13)

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

# Generate the PIO header file
pico_generate_pio_header(blink_pio ${CMAKE_CURRENT_LIST_DIR}/src/blink.pio)

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
