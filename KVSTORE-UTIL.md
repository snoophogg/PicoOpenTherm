# Getting kvstore-util

The `kvstore-util` tool is needed for provisioning configuration to your Pico W device. Due to incompatibility between the pico-kvstore submodule and Pico SDK 2.2.0, this tool cannot be built with the current SDK version.

## Option 1: Skip Provisioning (Easiest)

The firmware includes hardcoded default configuration values in `src/config.cpp`. You can:
1. Edit `src/config.cpp` directly with your WiFi/MQTT settings
2. Rebuild the firmware with `./build-all.sh`
3. Flash it to your Pico W

No provisioning needed!

## Option 2: Build kvstore-util with Compatible SDK (Advanced)

If you need kvstore-util for automated provisioning, you can build it with an older SDK version that's compatible. The build system can automatically fetch the correct SDK version:

```bash
cd pico-kvstore/host
mkdir -p build && cd build

# Let CMake fetch SDK 2.1.1 (compatible with kvstore-util)
cmake -DPICO_SDK_FETCH_FROM_GIT=ON \
      -DPICO_SDK_FETCH_FROM_GIT_TAG=2.1.1 \
      -DPICO_SDK_FETCH_FROM_GIT_PATH=./sdk \
      .. || true

# Initialize SDK submodules (required after fetch)
if [ -d sdk/pico_sdk-src ]; then
  cd sdk/pico_sdk-src
  git submodule update --init --recursive
  cd ../..
fi

# Reconfigure now that submodules are initialized
cmake -DPICO_SDK_FETCH_FROM_GIT=ON \
      -DPICO_SDK_FETCH_FROM_GIT_TAG=2.1.1 \
      -DPICO_SDK_FETCH_FROM_GIT_PATH=./sdk \
      ..

make
```

The `kvstore-util` binary will be in `pico-kvstore/host/build/`

**How it works**: 
- CMake will download Pico SDK 2.1.1 to `pico-kvstore/host/build/sdk/`
- We then initialize the SDK's submodules (like mbedtls)
- A second cmake run completes the configuration
- This is isolated from your main SDK 2.2.0 installation
- No need to manually manage multiple SDK versions
- SDK 2.1.1 has compatible mbedtls API for kvstore's usage

## Option 3: Runtime Configuration

The firmware supports runtime configuration through Home Assistant MQTT:
- WiFi SSID and Password can be configured via text entities
- MQTT settings can be configured via text entities
- Changes are saved to flash automatically

Just flash the firmware with default settings, then configure via Home Assistant.

## Why This Incompatibility Exists

The pico-kvstore host tool uses mbedtls APIs that changed between Pico SDK versions. Patching the submodule violates git submodule integrity, so we don't provide patched builds.
