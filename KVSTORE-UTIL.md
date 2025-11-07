# Getting kvstore-util

The `kvstore-util` tool is needed for provisioning configuration to your Pico W device. Due to incompatibility between the pico-kvstore submodule and Pico SDK 2.2.0, this tool cannot be built with the current SDK version.

## Option 1: Skip Provisioning (Easiest)

The firmware includes hardcoded default configuration values in `src/config.cpp`. You can:
1. Edit `src/config.cpp` directly with your WiFi/MQTT settings
2. Rebuild the firmware with `./build-all.sh`
3. Flash it to your Pico W

No provisioning needed!

## Option 2: Build kvstore-util with Compatible SDK (Advanced)

If you need kvstore-util for automated provisioning, you can build it with SDK 2.1.1. The easiest way is to manually fetch the SDK:

```bash
cd pico-kvstore/host
mkdir -p build/sdk
cd build/sdk

# Clone SDK 2.1.1
git clone --branch 2.1.1 https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk

# Initialize all submodules recursively
git submodule update --init --recursive
cd ../..

# Build kvstore-util
export PICO_SDK_PATH=$PWD/sdk/pico-sdk
cmake ..
make
```

The `kvstore-util` binary will be in `pico-kvstore/host/build/`

**How it works**: 
- Manually clones Pico SDK 2.1.1 to `pico-kvstore/host/build/sdk/pico-sdk/`
- Initializes all SDK submodules recursively
- Uses PICO_SDK_PATH to point to this SDK instead of FetchContent
- This is isolated from your main SDK 2.2.0 installation
- SDK 2.1.1 has compatible mbedtls API for kvstore's usage

## Option 3: Runtime Configuration

The firmware supports runtime configuration through Home Assistant MQTT:
- WiFi SSID and Password can be configured via text entities
- MQTT settings can be configured via text entities
- Changes are saved to flash automatically

Just flash the firmware with default settings, then configure via Home Assistant.

## Why This Incompatibility Exists

The pico-kvstore host tool uses mbedtls APIs that changed between Pico SDK versions. Patching the submodule violates git submodule integrity, so we don't provide patched builds.
