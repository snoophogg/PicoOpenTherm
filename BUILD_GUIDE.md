# PicoOpenTherm Build Guide

## Quick Start

The easiest way to build firmware is using the build script:

```bash
./build.sh                 # Build both hardware and simulator
```

## Build Script Usage

### Basic Commands

```bash
./build.sh                 # Build both targets (default)
./build.sh hardware        # Build hardware firmware only
./build.sh simulator       # Build simulator firmware only
./build.sh --help          # Show help message
```

### Build Options

```bash
./build.sh clean           # Clean build before building
./build.sh reconfigure     # Re-run CMake configuration
./build.sh -v              # Verbose build output
```

### Combined Examples

```bash
./build.sh hardware clean  # Clean and build hardware
./build.sh sim -v          # Build simulator with verbose output
./build.sh both clean -v   # Clean, build both, verbose
```

### Short Aliases

The script supports convenient short aliases:
- `hw` = `hardware`
- `sim` = `simulator`
- `all` = `both`

```bash
./build.sh hw              # Build hardware
./build.sh sim clean       # Clean and build simulator
```

## Build Outputs

After successful build, firmware files are located in:
- `build/picoopentherm.uf2` - Hardware firmware (for real OpenTherm hardware)
- `build/picoopentherm_simulator.uf2` - Simulator firmware (simulates OpenTherm without hardware)

## Flashing Firmware

### Method 1: BOOTSEL Mode (Recommended)

1. Hold the **BOOTSEL button** on Pico W
2. While holding, connect USB cable to computer
3. Pico W appears as **RPI-RP2** USB drive
4. Copy `.uf2` file to RPI-RP2 drive
5. Pico W automatically reboots with new firmware

### Method 2: Using Picotool

```bash
# Install picotool (first time only)
sudo apt install picotool  # On Raspberry Pi
# or build from source: https://github.com/raspberrypi/picotool

# Flash firmware
sudo picotool load build/picoopentherm.uf2 -f
sudo picotool reboot
```

## Build Targets Explained

### Hardware Firmware (`picoopentherm.uf2`)

**Use this for**: Production deployment with real OpenTherm hardware
- Communicates with actual boiler via OpenTherm protocol
- Uses PIO state machines for timing-critical communication
- Requires OpenTherm adapter hardware connected to GPIO pins

### Simulator Firmware (`picoopentherm_simulator.uf2`)

**Use this for**: Testing and development without hardware
- Simulates realistic OpenTherm data (temperatures, pressures, etc.)
- No OpenTherm adapter needed
- Perfect for testing Home Assistant integration
- Ideal for software development and debugging

## Manual Build (Advanced)

If you need more control or want to use different build tools:

### Initial Configuration

```bash
mkdir -p build
cd build
cmake -DPICO_BOARD=pico_w ..
cd ..
```

### Build Specific Target

```bash
# Hardware firmware
cmake --build build --target picoopentherm

# Simulator firmware
cmake --build build --target picoopentherm_simulator

# Both targets
cmake --build build
```

### Parallel Build (Faster)

```bash
cmake --build build --target picoopentherm -- -j$(nproc)
```

### Clean Build

```bash
rm -rf build
mkdir -p build
cd build
cmake -DPICO_BOARD=pico_w ..
make -j$(nproc)
```

## Troubleshooting

### "pico-sdk not found"

Initialize submodules:
```bash
git submodule update --init --recursive
```

### Build fails with linker errors

Try a clean rebuild:
```bash
./build.sh clean
```

### CMake configuration errors

Reconfigure from scratch:
```bash
rm -rf build
./build.sh
```

### Permission denied when running build.sh

Make script executable:
```bash
chmod +x build.sh
```

## Development Workflow

### Typical Development Cycle

```bash
# 1. Make code changes
vim src/mqtt_common.cpp

# 2. Build and test simulator first (faster iteration)
./build.sh sim

# 3. Flash simulator firmware and test
# (copy build/picoopentherm_simulator.uf2 to Pico W)

# 4. Once working, build hardware version
./build.sh hardware

# 5. Deploy to production
# (copy build/picoopentherm.uf2 to Pico W)
```

### Quick Iteration

For rapid testing during development:
```bash
# Terminal 1: Watch for changes and rebuild
watch -n 1 './build.sh sim'

# Terminal 2: Monitor serial output
minicom -D /dev/ttyACM0 -b 115200
```

## Build Performance

### Typical Build Times

On Raspberry Pi 4 (4 cores):
- **First build**: ~5-8 minutes (compiles all dependencies)
- **Incremental build**: ~5-15 seconds (only changed files)
- **Clean build**: ~3-5 minutes (reuses some dependencies)

### Optimization Tips

1. **Use parallel builds**: Script uses `-j$(nproc)` automatically
2. **Incremental builds**: Don't use `clean` unless necessary
3. **Build on SSD**: Faster I/O than SD card
4. **Use ccache**: Cache compilation results
   ```bash
   sudo apt install ccache
   export CMAKE_C_COMPILER_LAUNCHER=ccache
   export CMAKE_CXX_COMPILER_LAUNCHER=ccache
   ```

## CI/CD Integration

### GitHub Actions Example

```yaml
name: Build Firmware
on: [push, pull_request]

jobs:
  build:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
        with:
          submodules: recursive

      - name: Install ARM toolchain
        run: sudo apt install gcc-arm-none-eabi

      - name: Build firmware
        run: ./build.sh both

      - name: Upload artifacts
        uses: actions/upload-artifact@v3
        with:
          name: firmware
          path: build/*.uf2
```

## Build Configuration

### CMake Options

```bash
# Custom board
cmake -DPICO_BOARD=pico_w ..

# Enable simulator by default
cmake -DUSE_SIMULATOR=ON ..

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build (optimized)
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### Compiler Flags

Edit `CMakeLists.txt` to add custom flags:
```cmake
target_compile_options(picoopentherm PRIVATE
    -Wall -Wextra -Werror  # Strict warnings
    -O2                     # Optimization level
)
```

## Size Optimization

### Current Sizes

- Hardware firmware: ~1.3MB
- Simulator firmware: ~1.3MB
- Pico W flash: 2MB total (plenty of space)

### If Size Becomes an Issue

```cmake
# In CMakeLists.txt
target_compile_options(picoopentherm PRIVATE -Os)  # Optimize for size
target_link_options(picoopentherm PRIVATE -Os)
```

## Related Documentation

- [DUAL_CORE_ARCHITECTURE.md](DUAL_CORE_ARCHITECTURE.md) - Dual-core implementation details
- [MQTT_RELIABILITY_IMPROVEMENTS.md](MQTT_RELIABILITY_IMPROVEMENTS.md) - Network optimizations
- [README.md](README.md) - Project overview and setup
- [Pico SDK Documentation](https://raspberrypi.github.io/pico-sdk-doxygen/)
