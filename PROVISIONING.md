# Configuration Provisioning

This directory contains tools to provision your PicoOpenTherm configuration to the device before flashing the firmware.

## Quick Start (Recommended)

**No kvstore-util needed!**

The easiest way to configure PicoOpenTherm is to edit the default values directly in the source code:

1. **Edit default configuration:**
   ```bash
   nano src/config.cpp
   # Update WiFi SSID, password, MQTT broker, etc.
   ```

2. **Build firmware:**
   ```bash
   ./build-all.sh
   ```

3. **Flash firmware:**
   ```bash
   # Put Pico in BOOTSEL mode, then:
   picotool load build/picoopentherm.uf2
   # Or drag-and-drop the UF2 file
   ```

4. **Optional: Runtime configuration via Home Assistant**
   - After first boot, configure settings via MQTT/Home Assistant entities
   - Changes are saved to flash automatically

## Advanced: Build from Source

If you want to build everything yourself:

1. **Build firmware:**
   ```bash
   ./build-all.sh
   ```

2. **Flash firmware:**
   ```bash
   picotool load build/picoopentherm.uf2
   ```

**Note:** kvstore-util provisioning is not supported with Pico SDK 2.2.0. See "Quick Start" above for the recommended configuration method.

## What do the provisioning scripts do?

### provision-simple.sh (Deprecated)
This script is deprecated due to kvstore-util incompatibility with Pico SDK 2.2.0.
Use the "Quick Start" method above instead (edit src/config.cpp directly).

### provision-config.sh (Deprecated)
This script is deprecated due to kvstore-util incompatibility with Pico SDK 2.2.0.
Use the "Quick Start" method above instead (edit src/config.cpp directly).

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

### Option 1: Edit Source Code (Easiest)

No special tools required:
- Text editor (nano, vim, VS Code, etc.)
- Pico SDK and build tools (for building firmware)
- picotool (for flashing) - optional, can use drag-and-drop

### Option 2: Use Older Pico SDK for kvstore-util

If you need kvstore-util for advanced provisioning, see [KVSTORE-UTIL.md](KVSTORE-UTIL.md) for instructions on using an older SDK version.

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

**kvstore-util build issues:**
- kvstore-util is incompatible with Pico SDK 2.2.0
- Use the "Quick Start" method (edit src/config.cpp directly)
- Or see [KVSTORE-UTIL.md](KVSTORE-UTIL.md) for advanced options

**Configuration not applied:**
- If you edited src/config.cpp, make sure you rebuilt the firmware
- Flash the new firmware to your Pico W
- Runtime configuration via Home Assistant also works

## Flash Layout

```
0x00000000 - Firmware (UF2)
    ...
0x001FC0000 - Configuration Storage (256KB)
0x00200000 - End of Flash (2MB)
```

The configuration is stored in the last 256KB of the 2MB flash, leaving plenty of space for firmware.
