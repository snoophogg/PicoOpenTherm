# Getting kvstore-util

The `kvstore-util` tool is needed for provisioning configuration to your Pico W device. Due to compatibility issues between the pico-kvstore submodule and newer Pico SDK versions, pre-built binaries are not provided in releases.

## Building Locally (Recommended)

The easiest way is to use the provided build script:

```bash
./build-all.sh
```

This builds both the firmware and `kvstore-util`. The tool will be available at:
```
pico-kvstore/host/build/kvstore-util
```

The `provision-config.sh` script will automatically find and use this tool, or attempt to build it if not found.

## Manual Build

If you prefer to build manually:

```bash
export PICO_SDK_PATH=$(pwd)/pico-sdk
cd pico-kvstore/host

# Apply compatibility patches for Pico SDK 2.2.0+
sed -i.bak '36,39s/^add_library/#add_library/' CMakeLists.txt
sed -i.bak2 '/kvstore_securekvs/d' CMakeLists.txt
sed -i.bak3 '/mbedcrypto/d' CMakeLists.txt

mkdir -p build && cd build
cmake ..
make
```

## Why Not Pre-built?

The pico-kvstore host tool has dependencies on specific mbedtls API versions that vary between Pico SDK releases. Building locally ensures compatibility with your environment.

## Alternative: Use Docker

If you have build issues, you can use the included Docker environment which has all dependencies pre-configured. See `docker/README.md` for details.
