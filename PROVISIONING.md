# Configuration Provisioning

This directory contains tools to provision your PicoOpenTherm configuration to the device before flashing the firmware.

## Quick Start (Pre-built Tools)

**Easiest method - no compilation required!**

1. **Download pre-built tools from [GitHub Releases](https://github.com/snoophogg/PicoOpenTherm/releases/latest):**
   - `picotool-linux-amd64-vX.X.X`
   - `kvstore-util-linux-amd64-vX.X.X`

2. **Place them in the bin/ folder:**
   ```bash
   mkdir -p bin
   mv ~/Downloads/picotool-linux-amd64-* bin/picotool
   mv ~/Downloads/kvstore-util-linux-amd64-* bin/kvstore-util
   chmod +x bin/picotool bin/kvstore-util
   ```

3. **Create your configuration file:**
   ```bash
   cp secrets.cfg.example secrets.cfg
   nano secrets.cfg
   ```

4. **Run the simple provisioning script:**
   ```bash
   ./provision-simple.sh
   ```

5. **Flash your firmware** (download from releases or use drag-and-drop)

## Advanced: Build from Source

If you prefer to build the tools yourself:

1. **Create your configuration file:**
   ```bash
   cp secrets.cfg.example secrets.cfg
   nano secrets.cfg
   ```

2. **Run the full provisioning script:**
   ```bash
   ./provision-config.sh
   ```
   This will build kvstore-util and picotool if needed.

3. **Flash your firmware:**
   ```bash
   picotool load build/picoopentherm.uf2
   ```

## What do the provisioning scripts do?

### provision-simple.sh (Recommended)
- Uses pre-built tools from the `bin/` folder
- No compilation required
- Reads your configuration from `secrets.cfg`
- Creates a `settings.bin` file using `kvstore-util`
- Flashes the configuration to the Pico W at the correct flash offset (last 256KB)
- Configuration persists in flash memory

### provision-config.sh (Advanced)
- Same functionality as provision-simple.sh
- Additionally builds kvstore-util and picotool from source if not found
- Useful for developers or if pre-built tools aren't available

## Configuration Keys

The following keys are supported in `secrets.cfg`:

| Key | Description | Example |
|-----|-------------|---------|
| `wifi.ssid` | WiFi network name | `MyNetwork` |
| `wifi.password` | WiFi password | `MyPassword123` |
| `mqtt.server_ip` | MQTT broker IP address | `192.168.1.100` |
| `mqtt.server_port` | MQTT broker port | `1883` |
| `mqtt.client_id` | MQTT client identifier | `picoopentherm` |
| `device.name` | Device display name | `OpenTherm Gateway` |
| `device.id` | Device unique ID | `opentherm_gw` |
| `opentherm.tx_pin` | OpenTherm TX GPIO pin | `16` |
| `opentherm.rx_pin` | OpenTherm RX GPIO pin | `17` |

## Requirements

### Option 1: Pre-built Tools (Easiest)

Download from [GitHub Releases](https://github.com/snoophogg/PicoOpenTherm/releases/latest):
- `picotool-linux-amd64` - Tool for flashing Pico devices
- `kvstore-util-linux-amd64` - Configuration management tool

Place them in the `bin/` folder and make them executable.

### Option 2: Build from Source

#### kvstore-util

The `kvstore-util` tool is part of the pico-kvstore submodule. If not built, the script will attempt to build it automatically.

To manually build:
```bash
export PICO_SDK_PATH=$(pwd)/pico-sdk
cd pico-kvstore/host
mkdir -p build && cd build
cmake ..
make
```

#### picotool

Install picotool to flash the configuration. Picotool requires the Pico SDK to build:

```bash
# Set PICO_SDK_PATH to the pico-sdk submodule in this project
export PICO_SDK_PATH=$(pwd)/pico-sdk

# Build picotool
cd picotool
mkdir -p build && cd build
cmake ..
make

# Optional: install system-wide
sudo make install
```

**Note:** The `PICO_SDK_PATH` must point to a valid Pico SDK directory. This project includes the SDK as a submodule at `pico-sdk/`.

## Workflow

### First Time Setup (Using Pre-built Tools)
```bash
# 1. Download pre-built tools from GitHub releases
mkdir -p bin
mv ~/Downloads/picotool-linux-amd64-* bin/picotool
mv ~/Downloads/kvstore-util-linux-amd64-* bin/kvstore-util
chmod +x bin/picotool bin/kvstore-util

# 2. Configure your settings
cp secrets.cfg.example secrets.cfg
nano secrets.cfg

# 3. Connect Pico in BOOTSEL mode
# (hold BOOTSEL button, connect USB, release BOOTSEL)

# 4. Provision configuration
./provision-simple.sh

# 5. Download and flash firmware from GitHub releases
bin/picotool load picoopentherm-vX.X.X.uf2
```

### First Time Setup (Building from Source)
```bash
# 1. Configure your settings
cp secrets.cfg.example secrets.cfg
nano secrets.cfg

# 2. Build the firmware
./build.sh

# 3. Connect Pico in BOOTSEL mode
# (hold BOOTSEL button, connect USB, release BOOTSEL)

# 4. Provision configuration (builds tools if needed)
./provision-config.sh

# 5. Flash firmware
picotool load build/picoopentherm.uf2
```

### Updating Configuration Only
```bash
# 1. Edit secrets.cfg
nano secrets.cfg

# 2. Connect Pico in BOOTSEL mode

# 3. Re-provision
./provision-simple.sh  # or ./provision-config.sh

# 4. Reboot (or re-flash firmware)
picotool reboot
```

## Runtime Configuration

You can also change configuration via Home Assistant after the device is running:
- Device Name (text entity)
- Device ID (text entity)
- OpenTherm TX Pin (number entity)
- OpenTherm RX Pin (number entity)

Changes made via Home Assistant are saved to flash and trigger an automatic restart.

## Troubleshooting

**Tools not found:**

Using pre-built tools:
```bash
# Download from releases and place in bin/
mkdir -p bin
# Download picotool-linux-amd64-* and kvstore-util-linux-amd64-*
mv ~/Downloads/picotool-linux-amd64-* bin/picotool
mv ~/Downloads/kvstore-util-linux-amd64-* bin/kvstore-util
chmod +x bin/*
```

Building from source:
```bash
# kvstore-util (requires PICO_SDK_PATH)
export PICO_SDK_PATH=$(pwd)/pico-sdk
cd pico-kvstore/host
mkdir -p build && cd build
cmake ..
make

# picotool (requires PICO_SDK_PATH)
export PICO_SDK_PATH=$(pwd)/pico-sdk
cd picotool
mkdir -p build && cd build
cmake ..
make
sudo make install
```

**"PICO_SDK_PATH is not defined" error when building picotool:**

Picotool requires the Pico SDK to build. Set the environment variable to point to the SDK submodule:

```bash
# From the project root directory
export PICO_SDK_PATH=$(pwd)/pico-sdk

# Then build picotool
cd picotool
mkdir -p build && cd build
cmake ..
make
```

Or set it permanently in your shell profile:
```bash
echo 'export PICO_SDK_PATH=/path/to/PicoOpenTherm/pico-sdk' >> ~/.bashrc
source ~/.bashrc
```

**Pico not detected:**
- Make sure Pico is in BOOTSEL mode (hold button while connecting USB)
- Check USB connection
- Try `lsusb` to see if device is recognized
- On Linux, you may need udev rules for non-root access

**Configuration not applied:**
- Verify settings.bin was created
- Check flash offset is correct (0x1FC0000 for 2MB flash)
- Ensure you flash firmware after provisioning config

**Permission denied when running tools:**
```bash
chmod +x bin/picotool bin/kvstore-util
```

## Flash Layout

```
0x00000000 - Firmware (UF2)
    ...
0x001FC0000 - Configuration Storage (256KB)
0x00200000 - End of Flash (2MB)
```

The configuration is stored in the last 256KB of the 2MB flash, leaving plenty of space for firmware.
