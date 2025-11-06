# Configuration Provisioning

This directory contains tools to provision your PicoOpenTherm configuration to the device before flashing the firmware.

## Quick Start

1. **Create your configuration file:**
   ```bash
   cp secrets.cfg.example secrets.cfg
   nano secrets.cfg
   ```

2. **Edit `secrets.cfg` with your settings:**
   - WiFi SSID and password
   - MQTT broker IP and port
   - Device name and ID
   - OpenTherm GPIO pins

3. **Run the provisioning script:**
   ```bash
   ./provision-config.sh
   ```

4. **Flash your firmware:**
   ```bash
   picotool load build/picoopentherm.uf2
   ```

## What does provision-config.sh do?

1. Reads your configuration from `secrets.cfg`
2. Uses `kvstore-util` to create a `settings.bin` file
3. Flashes the configuration to the Pico W at the correct flash offset (last 256KB)
4. The configuration persists in flash memory

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

### kvstore-util

The `kvstore-util` tool is part of the pico-kvstore submodule. If not built, the script will attempt to build it automatically.

To manually build:
```bash
cd pico-kvstore/tools
make kvstore-util
```

### picotool

Install picotool to flash the configuration:

```bash
cd picotool
mkdir -p build && cd build
cmake ..
make
sudo make install
```

## Workflow

### First Time Setup
```bash
# 1. Configure your settings
cp secrets.cfg.example secrets.cfg
nano secrets.cfg

# 2. Build the firmware
./build.sh

# 3. Connect Pico in BOOTSEL mode
# (hold BOOTSEL button, connect USB, release BOOTSEL)

# 4. Provision configuration
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
./provision-config.sh

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

**kvstore-util not found:**
```bash
cd pico-kvstore/tools
make kvstore-util
```

**picotool not found:**
```bash
sudo apt install picotool
# or build from source (see above)
```

**Pico not detected:**
- Make sure Pico is in BOOTSEL mode (hold button while connecting USB)
- Check USB connection
- Try `lsusb` to see if device is recognized

**Configuration not applied:**
- Verify settings.bin was created
- Check flash offset is correct (0x1FC0000 for 2MB flash)
- Ensure you flash firmware after provisioning config

## Flash Layout

```
0x00000000 - Firmware (UF2)
    ...
0x001FC0000 - Configuration Storage (256KB)
0x00200000 - End of Flash (2MB)
```

The configuration is stored in the last 256KB of the 2MB flash, leaving plenty of space for firmware.
