#!/bin/bash

# Build script for PicoOpenTherm
# Uses pico-sdk submodule

# Ensure submodules are initialized
if [ ! -f "pico-sdk/CMakeLists.txt" ]; then
    echo "Initializing pico-sdk submodule..."
    git submodule update --init --recursive
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
echo "Flash opentherm_test.uf2 to your Pico W"
