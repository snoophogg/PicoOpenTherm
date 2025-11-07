# Getting kvstore-util

The `kvstore-util` tool is needed for provisioning configuration to your Pico W device. Due to incompatibility between the pico-kvstore submodule and Pico SDK 2.2.0, this tool cannot be built with the current SDK version.

## Option 1: Skip Provisioning (Easiest)

The firmware includes hardcoded default configuration values in `src/config.cpp`. You can:
1. Edit `src/config.cpp` directly with your WiFi/MQTT settings
2. Rebuild the firmware with `./build-all.sh`
3. Flash it to your Pico W

No provisioning needed!

## Option 2: Use an Older Pico SDK

If you need to use kvstore-util for provisioning:

1. Check out an older Pico SDK version that's compatible with pico-kvstore:
   ```bash
   cd pico-sdk
   git checkout 1.5.1  # Or another compatible version
   cd ..
   ```

2. Build kvstore-util:
   ```bash
   export PICO_SDK_PATH=$(pwd)/pico-sdk
   cd pico-kvstore/host
   mkdir -p build && cd build
   cmake ..
   make
   ```

3. After building kvstore-util, switch back to the newer SDK for firmware:
   ```bash
   cd ../../../pico-sdk
   git checkout 2.2.0
   cd ..
   ```

## Option 3: Runtime Configuration

The firmware supports runtime configuration through Home Assistant MQTT:
- WiFi SSID and Password can be configured via text entities
- MQTT settings can be configured via text entities
- Changes are saved to flash automatically

Just flash the firmware with default settings, then configure via Home Assistant.

## Why This Incompatibility Exists

The pico-kvstore host tool uses mbedtls APIs that changed between Pico SDK versions. Patching the submodule violates git submodule integrity, so we don't provide patched builds.
