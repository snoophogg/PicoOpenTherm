#!/bin/bash

# Build script for PicoOpenTherm

# Check if PICO_SDK_PATH is set
if [ -z "$PICO_SDK_PATH" ]; then
    echo "Error: PICO_SDK_PATH environment variable is not set"
    echo "Please set it to your Pico SDK installation path"
    echo "Example: export PICO_SDK_PATH=/path/to/pico-sdk"
    exit 1
fi

# Create build directory if it doesn't exist
mkdir -p build
cd build

# Run CMake configuration
cmake -DPICO_BOARD=pico_w ..

# Build the project
make -j$(nproc)

echo ""
echo "Build complete!"
echo "Flash blink_pio.uf2 to your Pico W"
