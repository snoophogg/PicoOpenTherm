# PicoOpenTherm

A PIO-based blink routine for Raspberry Pi Pico W, built with CMake using the Pico SDK.

## Features

- Uses PIO (Programmable I/O) state machine for efficient GPIO control
- Blinks both the onboard LED (via CYW43) and an external LED on GPIO 25
- Built with CMake and the official Pico SDK
- Optimized for Pico W

## Prerequisites

1. **ARM GCC Toolchain**: Install `gcc-arm-none-eabi`
2. **CMake**: Version 3.13 or higher
3. **Build Tools**: Make or Ninja
4. **Git**: For cloning submodules

Note: The Pico SDK and Picotool are included as git submodules and will be automatically initialized.

## Setup

1. Clone the repository with submodules:
   ```bash
   git clone --recursive https://github.com/yourusername/PicoOpenTherm.git
   ```
   
   Or if already cloned, initialize submodules:
   ```bash
   git submodule update --init --recursive
   ```

## Building

### Using the build script:
```bash
chmod +x build.sh
./build.sh
```

### Manual build:
```bash
mkdir -p build
cd build
cmake -DPICO_BOARD=pico_w ..
make -j4
```

## Flashing

1. Connect your Pico W while holding the BOOTSEL button
2. Copy the generated `build/blink_pio.uf2` file to the Pico W drive
3. The Pico will automatically reboot and start running the blink program

## Project Structure

```
.
├── CMakeLists.txt           # Main CMake configuration
├── pico_sdk_import.cmake    # Pico SDK import helper
├── build.sh                 # Build automation script
├── pico-sdk/                # Pico SDK submodule
├── picotool/                # Picotool submodule
├── src/
│   ├── blink.c             # Main application code
│   └── blink.pio           # PIO assembly program for LED blinking
└── protocol/
    └── openthermProtocol.txt
```

## How It Works

The project demonstrates PIO usage on the Pico W:

1. **PIO State Machine**: The `blink.pio` file contains a PIO program that efficiently toggles a GPIO pin
2. **Dual Blinking**: 
   - GPIO 25: Controlled by PIO state machine (1 Hz)
   - Onboard LED: Controlled by CYW43 chip via main loop (2 Hz)

The PIO program is more efficient than traditional GPIO toggling as it runs independently on dedicated hardware.

## Customization

- **Blink Frequency**: Modify the blink frequency in `src/blink.c` by changing the last parameter in `blink_program_init()`
- **GPIO Pin**: Change `BLINK_PIN` to use a different GPIO pin
- **USB Output**: The project is configured for USB serial output. Connect via serial terminal at 115200 baud to see debug messages