# PicoOpenTherm Simulator

The PicoOpenTherm Simulator is a special firmware build that simulates OpenTherm data without requiring physical OpenTherm hardware. This is ideal for:

- Testing Home Assistant integration and dashboards
- Developing automations and scripts
- Demonstrating the system without a boiler
- CI/CD testing and validation

## Features

The simulator generates realistic OpenTherm data with dynamic behavior:

- **Room Temperature**: 18-22°C with gradual approach to setpoint
- **Boiler Temperature**: 50-80°C with heating modulation effects
- **DHW Temperature**: 55-65°C when DHW is enabled
- **Pressure**: 1.3-1.7 bar with small variations
- **Modulation**: 0-100% calculated from temperature difference
- **Statistics**: Burner hours, starts, and pump hours increment over time
- **Fault Handling**: Simulates fault codes and OEM diagnostics

All values use sine wave patterns and realistic physics to create lifelike behavior.

## Quick Start

### 1. Flash the Simulator Firmware

#### Option A: Using Pre-built Release (Recommended)

1. Download the latest `picoopentherm_simulator-vX.X.X.uf2` from [Releases](https://github.com/snoophogg/PicoOpenTherm/releases)
2. Hold the **BOOTSEL** button on your Pico W
3. Connect the Pico W to your computer via USB (keep holding BOOTSEL)
4. Release BOOTSEL - the Pico W will appear as a USB drive
5. Copy the `.uf2` file to the Pico W drive
6. The Pico W will automatically reboot and start running the simulator

#### Option B: Build from Source

```bash
# Clone the repository
git clone --recursive https://github.com/snoophogg/PicoOpenTherm.git
cd PicoOpenTherm

# Configure WiFi and MQTT settings
cp secrets.cfg.example secrets.cfg
nano secrets.cfg  # Edit with your settings

# Provision the configuration
./provision-config.sh

# Build the simulator
./build.sh
cd build
make picoopentherm_simulator

# Flash the firmware (Pico W in BOOTSEL mode)
cp picoopentherm_simulator.uf2 /media/your-username/RPI-RP2/
```

### 2. Configure WiFi and MQTT

Before flashing, you need to configure your WiFi and MQTT broker settings:

1. Copy the example configuration:
   ```bash
   cp secrets.cfg.example secrets.cfg
   ```

2. Edit `secrets.cfg` with your settings:
   ```ini
   WIFI_SSID=YourWiFiNetwork
   WIFI_PASSWORD=YourWiFiPassword
   MQTT_BROKER=192.168.1.100
   MQTT_PORT=1883
   MQTT_USER=
   MQTT_PASSWORD=
   ```

3. Run the provisioning script:
   ```bash
   ./provision-config.sh
   ```

This will update the configuration in `src/config.cpp` which gets compiled into the firmware.

### 3. Start the Docker Development Environment

The Docker setup provides a complete Home Assistant and MQTT broker environment with pre-configured dashboards and automations.

```bash
# Navigate to the docker directory
cd docker

# Start the services
docker compose up -d

# Check the logs
docker compose logs -f
```

**Services:**
- **Home Assistant**: http://localhost:8123
- **MQTT Broker (Mosquitto)**: localhost:1883
- **MQTT WebSocket**: localhost:9001

**First-time setup:**
1. Open Home Assistant at http://localhost:8123
2. Complete the initial setup wizard (create user account)
3. Configure MQTT integration:
   - Go to Settings → Devices & Services
   - Click "Configure" on the MQTT card
   - Broker: `mosquitto` (container name)
   - Port: `1883`
   - Leave username/password empty (or configure if you set them up)

### 4. Monitor Simulator Output

Once the simulator is running, you can monitor it:

**Check MQTT topics:**
```bash
# Subscribe to all OpenTherm topics
docker exec -it picoopentherm-mosquitto mosquitto_sub -v -t 'opentherm/#'

# Or use a specific topic
docker exec -it picoopentherm-mosquitto mosquitto_sub -v -t 'opentherm/room_temp'
```

**View Pico W serial output:**
```bash
# On Linux
screen /dev/ttyACM0 115200

# On macOS
screen /dev/tty.usbmodem* 115200

# On Windows (use PuTTY or similar)
# Connect to the appropriate COM port at 115200 baud
```

**Home Assistant:**
- Open the "OpenTherm Gateway" dashboard
- All simulated sensors should appear and update every 30 seconds
- Room temperature will gradually approach the setpoint
- Modulation level will vary based on heating demand

## Simulated Data Details

### Temperature Behavior

- **Room Temperature**: Starts around 20°C and gradually approaches the setpoint
  - Uses sine wave with 600-second period
  - Adds weighted approach to setpoint (90% current + 10% target)
  - Realistic thermal inertia simulation

- **Boiler Temperature**: Base range 50-80°C with modulation heating
  - When modulation > 50%: adds up to +20°C based on modulation level
  - Creates realistic heating cycle behavior

- **DHW Temperature**: 55-65°C when DHW is enabled
  - Uses 400-second sine wave period
  - Independent of room heating

### Dynamic Modulation

Modulation level is calculated from temperature difference:
```
temp_diff = max(0, setpoint - room_temp)
modulation = min(100, temp_diff * 25)
```

This creates realistic heating behavior where:
- Large temperature difference = high modulation
- Small difference = low modulation  
- At setpoint = 0% modulation

### Statistics Tracking

The simulator increments operational statistics over time:
- **Burner Hours**: +0.01 hours every 30 seconds when flame is on
- **Burner Starts**: +1 when flame state changes from off to on
- **DHW Burner Hours**: +0.01 hours when DHW and flame are both on
- **CH Pump Hours**: +0.01 hours when CH is enabled

### Fault Simulation

The simulator supports fault code injection via MQTT:
```bash
# Set a fault code
mosquitto_pub -t 'opentherm/fault_code/set' -m '1'

# Clear the fault
mosquitto_pub -t 'opentherm/fault_code/set' -m '0'
```

## Testing Scenarios

### Basic Operation Test

1. Flash the simulator firmware
2. Start Docker environment
3. Open Home Assistant dashboard
4. Verify all sensors appear and update
5. Set a room temperature setpoint via MQTT or HA:
   ```bash
   mosquitto_pub -t 'opentherm/room_setpoint/set' -m '22.5'
   ```
6. Watch room temperature gradually approach setpoint
7. Observe modulation level decrease as target is reached

### Automation Testing

Test the included automations:

1. **Overheat Alert**: Set boiler temp > 85°C (requires MQTT injection)
2. **Low Pressure Alert**: Simulated pressure stays 1.3-1.7 bar (safe range)
3. **Night Setback**: Enable at 23:00, reduce to 18°C
4. **Morning Warmup**: Enable at 06:00, increase to 21°C

### Dashboard Validation

The pre-configured dashboards include:

1. **Main Dashboard**: Quick overview with key entities
2. **OpenTherm Gateway Dashboard**: Comprehensive monitoring
   - Status section (flame, CH, DHW, fault)
   - Temperatures (room, boiler, DHW, outside)
   - Pressure and modulation gauges
   - Historical graphs
   - Controls (setpoint, CH enable/disable)
   - Statistics (burner hours, starts, etc.)

### Script Testing

Test the included scripts:

```bash
# Boost heating
mosquitto_pub -t 'script/opentherm_boost_heating' -m 'on'

# Eco mode
mosquitto_pub -t 'script/opentherm_eco_mode' -m 'on'

# Comfort mode
mosquitto_pub -t 'script/opentherm_comfort_mode' -m 'on'
```

## Differences from Real Hardware

The simulator differs from real hardware in these ways:

| Feature | Real Hardware | Simulator |
|---------|--------------|-----------|
| OpenTherm Protocol | Full v2.2 implementation via PIO | Not implemented |
| Response Time | Based on boiler (~1s) | Immediate |
| Physical Limits | Real thermal constraints | Simulated limits |
| Fault Detection | Real boiler faults | Simulated via MQTT |
| Statistics | Actual runtime hours | Simulated increments |
| Outside Temp | Real sensor data | Fixed at 10°C |

## Troubleshooting

### Simulator Not Connecting

1. **Check WiFi credentials** in `secrets.cfg`
2. **Verify MQTT broker** is running:
   ```bash
   docker ps | grep mosquitto
   ```
3. **Check Pico W LED**:
   - Slow blink: WiFi connecting
   - Fast blink: MQTT connecting
   - Solid on: Connected successfully
   - Rapid blink: Error state

### No Data in Home Assistant

1. **Verify MQTT integration** is configured
2. **Check MQTT topics**:
   ```bash
   docker exec -it picoopentherm-mosquitto mosquitto_sub -v -t 'opentherm/#'
   ```
3. **Restart Home Assistant**:
   ```bash
   docker compose restart homeassistant
   ```
4. **Check broker IP** matches your `secrets.cfg`

### Data Not Updating

1. **Monitor serial output** for errors
2. **Check MQTT connection**:
   ```bash
   docker exec -it picoopentherm-mosquitto mosquitto_sub -v -t 'opentherm/status'
   ```
3. **Verify publish interval** (default: 30 seconds)
4. **Check for duplicate device IDs** if running multiple Picos

### Build Errors

1. **Update submodules**:
   ```bash
   git submodule update --init --recursive
   ```
2. **Check toolchain**:
   ```bash
   arm-none-eabi-gcc --version
   ```
3. **Clean build**:
   ```bash
   rm -rf build
   ./build.sh
   cd build
   make picoopentherm_simulator
   ```

## CI/CD Integration

The simulator is automatically built in CI/CD pipelines:

- **GitHub Actions**: Builds on every push and PR
- **Artifacts**: Available in Actions tab for 30 days
- **Releases**: Included in all tagged releases

You can download pre-built simulator firmware from:
- [Latest Release](https://github.com/snoophogg/PicoOpenTherm/releases/latest)
- [GitHub Actions](https://github.com/snoophogg/PicoOpenTherm/actions) (for development builds)

## Architecture

The simulator shares ~90% of its codebase with the main firmware:

```
Common Code (mqtt_common.cpp):
├── WiFi connection handling
├── MQTT client management
├── Connection retry logic
├── LED status indicators
└── Message queue handling

Simulator-Specific (simulator.cpp):
├── SimulatedOpenThermInterface
├── Sine wave data generation
├── Temperature approach simulation
├── Modulation calculation
└── Statistics tracking
```

This architecture ensures:
- Consistent MQTT behavior between simulator and real device
- Easy maintenance and updates
- Reliable testing of integration code
- Minimal code duplication (~600 lines eliminated)

## Development

To modify simulator behavior:

1. Edit `src/simulator.cpp`
2. Focus on `SimulatedOpenThermInterface` class
3. Adjust sine wave parameters in `read()` method:
   ```cpp
   // Room temperature sine wave (600s period)
   float room_base = 20.0f + 2.0f * sin(elapsed_time * M_PI / 300.0f);
   
   // Boiler temperature sine wave (500s period)
   float boiler_base = 65.0f + 15.0f * sin(elapsed_time * M_PI / 250.0f);
   ```
4. Rebuild and reflash:
   ```bash
   cd build
   make picoopentherm_simulator
   cp picoopentherm_simulator.uf2 /media/your-username/RPI-RP2/
   ```

## See Also

- [Main README](../README.md) - Project overview and hardware setup
- [Home Assistant Integration](HOME_ASSISTANT.md) - HA integration details
- [Home Assistant Examples](HOME_ASSISTANT_EXAMPLES.md) - Dashboard and automation examples
- [Docker README](../docker/README.md) - Docker environment documentation
- [Provisioning Guide](PROVISIONING.md) - Configuration management

## Support

For issues, questions, or contributions:
- [GitHub Issues](https://github.com/snoophogg/PicoOpenTherm/issues)
- [GitHub Discussions](https://github.com/snoophogg/PicoOpenTherm/discussions)
