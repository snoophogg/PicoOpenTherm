# PicoOpenTherm

A complete OpenTherm v2.2 protocol implementation for Raspberry Pi Pico W with Home Assistant MQTT integration. Uses PIO (Programmable I/O) state machines for hardware-accelerated Manchester encoding/decoding.

## Features

### OpenTherm Protocol
- **Hardware Manchester Encoding**: PIO-based TX for 1000 bps Manchester encoding
- **Hardware Manchester Decoding**: PIO-based RX with automatic sampling
- **Complete OpenTherm Protocol**: Full implementation of OpenTherm v2.2 specification
- **High-Level API**: Easy-to-use C++ interface for reading sensors and controlling setpoints
- **Comprehensive Data IDs**: Support for 60+ standard OpenTherm data IDs with human-readable decoding
- **Type-Safe Conversions**: Automatic f8.8 to float, s16, and flag decoding
- **Configurable Timeout**: Adjustable timeout for all read/write operations

### Home Assistant Integration
- **MQTT Auto-Discovery**: Automatic entity registration with Home Assistant
- **60+ Entities**: Complete coverage of OpenTherm data as sensors, binary sensors, switches, and numbers
- **Two-Way Control**: Read sensor data and control boiler settings from Home Assistant
- **WiFi Connectivity**: Built-in WiFi support with automatic reconnection
- **Robust Connection Handling**: Infinite retry logic with LED error indication
- **Ready-to-Use Dashboards**: Included YAML examples for HA dashboards, cards, and automations

### System Reliability
- **Automatic Reconnection**: Intelligent WiFi and MQTT connection monitoring
- **LED Status Indication**: Visual feedback for connection state and errors
  - 1 blink: Normal operation
  - 2 blinks: WiFi connection error
  - 3 blinks: MQTT connection error
  - Rapid blinking: Fatal error (configuration or hardware initialization)
- **Persistent Configuration**: Flash-based storage for WiFi, MQTT, and device settings
- **Runtime Configuration**: Change device settings via Home Assistant (auto-restart on change)
- **Configurable Timeouts**: Adjustable retry delays and connection check intervals
- **Built with CMake**: Uses official Pico SDK with lwIP network stack
- **Optimized for Pico W**: Takes advantage of dual PIO blocks and CYW43 WiFi

## OpenTherm Overview

OpenTherm is a point-to-point communication protocol for HVAC systems, primarily used between room thermostats and boilers. This implementation provides:

- Bi-directional communication between master (thermostat) and slave (boiler)
- Manchester encoding (Bi-phase-L) at 1000 bits/sec
- 32-bit frame structure with parity checking
- Support for temperature, pressure, flow, modulation, and status monitoring

## Prerequisites

1. **ARM GCC Toolchain**: Install `gcc-arm-none-eabi`
2. **CMake**: Version 3.13 or higher
3. **Build Tools**: Make or Ninja
4. **Git**: For cloning submodules

Note: The Pico SDK and Picotool are included as git submodules and will be automatically initialized.

## Hardware Setup

### OpenTherm Connection

Connect your OpenTherm device to the Pico W:

- **TX Pin** (default GPIO 16): Connect to OpenTherm device RX
- **RX Pin** (default GPIO 17): Connect to OpenTherm device TX
- **Ground**: Common ground connection

**Important**: OpenTherm uses a voltage signaling standard. You may need appropriate level shifting or isolation circuitry depending on your specific hardware.

### WiFi & MQTT Configuration

Configuration is stored in flash memory using the pico-kvstore library. On first boot, default values are used. You can modify these defaults before building by editing the constants in `src/config.hpp`:

```cpp
// Default WiFi credentials
constexpr const char *DEFAULT_WIFI_SSID = "your_wifi_ssid";
constexpr const char *DEFAULT_WIFI_PASSWORD = "your_wifi_password";

// Default MQTT broker settings
constexpr const char *DEFAULT_MQTT_SERVER_IP = "192.168.1.100";
constexpr uint16_t DEFAULT_MQTT_SERVER_PORT = 1883;
constexpr const char *DEFAULT_MQTT_CLIENT_ID = "picoopentherm";

// Default device identification
constexpr const char *DEFAULT_DEVICE_NAME = "OpenTherm Gateway";
constexpr const char *DEFAULT_DEVICE_ID = "opentherm_gw";

// Default OpenTherm GPIO pins
constexpr uint8_t DEFAULT_OPENTHERM_TX_PIN = 16;
constexpr uint8_t DEFAULT_OPENTHERM_RX_PIN = 17;
```

Configuration is automatically saved to flash memory and persists across reboots. 

**Runtime Configuration**: Device name, device ID, and OpenTherm GPIO pins can be changed via Home Assistant MQTT entities. Changes are saved to flash and the device automatically restarts to apply new settings.

### LED Status Indicator

The onboard LED provides visual feedback:
- **1 blink** every ~1.4 seconds: Normal operation (connected and publishing)
- **2 blinks** repeating: WiFi connection error (retrying)
- **3 blinks** repeating: MQTT connection error (retrying)
- **Rapid blinking** (100ms on/off): Fatal error - configuration system or hardware initialization failed

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

This will generate the firmware file:
- `picoopentherm.uf2` - Complete OpenTherm gateway with Home Assistant MQTT integration

## Flashing

1. Connect your Pico W while holding the BOOTSEL button
2. Copy the generated `.uf2` file to the Pico W drive
3. The Pico will automatically reboot and start running the program

## Home Assistant Setup

After flashing the firmware and powering on your Pico W, Home Assistant will automatically discover the OpenTherm Gateway via MQTT.

### Quick Setup

1. **Configure defaults** in `src/config.hpp` (WiFi, MQTT broker, device name)
2. **Flash the firmware** to your Pico W
3. **Power on** - the device will use the configured defaults on first boot
4. **Check LED status**:
   - Single blinks = Connected and working
   - Double blinks = WiFi issue
   - Triple blinks = MQTT issue
   - Rapid blinks = Fatal error (check configuration or hardware)
5. **Open Home Assistant** - All OpenTherm entities will appear automatically

Configuration is saved to flash memory and persists across reboots.

### Available Entities

The gateway exposes 60+ entities organized by type:

#### Sensors
- Temperatures: Boiler, DHW, Return, Outside, Room, Exhaust
- Modulation: Current level, Max level
- Pressure & Flow: CH water pressure, DHW flow rate
- Counters: Burner starts/hours, CH pump starts/hours, DHW pump starts/hours
- Status: Fault code, OpenTherm version

#### Binary Sensors
- Status Flags: Fault, CH mode, DHW mode, Flame status, Cooling, Diagnostic
- Configuration: DHW present, Cooling supported, CH2 present

#### Switches
- Central Heating Enable
- Hot Water Enable

#### Numbers (Setpoints)
- Control Setpoint (0-100°C)
- Room Setpoint (5-30°C)
- DHW Setpoint (30-90°C)
- Max CH Setpoint (30-90°C)

#### Configuration Entities (Runtime Changeable)
- **Device Name** (text) - Change device display name
- **Device ID** (text) - Change device identifier  
- **OpenTherm TX Pin** (number 0-28) - Configure TX GPIO pin
- **OpenTherm RX Pin** (number 0-28) - Configure RX GPIO pin

**Note**: Configuration changes are saved to flash and trigger an automatic restart after 2 seconds to apply the new settings.

### Dashboard Examples

The `home_assistant/` folder includes ready-to-use dashboard examples:

- **`dashboard_overview.yaml`** - Complete system overview with all key metrics
- **`dashboard_graphs.yaml`** - Historical data visualization with temperature trends
- **`dashboard_control.yaml`** - Control panel for setpoints and modes
- **`dashboard_mobile.yaml`** - Mobile-optimized compact view
- **`entity_cards.yaml`** - Individual entity card examples with multiple styles
- **`automations.yaml`** - 15+ automation examples (schedules, safety, weather-based)
- **`README.md`** - Detailed setup instructions for dashboards and automations

### Example Dashboard Configuration

```yaml
# Add this to your Home Assistant configuration.yaml or dashboards
# See home_assistant/dashboard_overview.yaml for complete example

type: vertical-stack
cards:
  - type: entities
    title: Boiler Status
    entities:
      - entity: sensor.opentherm_gw_boiler_temp
      - entity: sensor.opentherm_gw_modulation
      - entity: binary_sensor.opentherm_gw_flame
      - entity: binary_sensor.opentherm_gw_fault
```

## Library Usage Example

For standalone library usage without Home Assistant:

```cpp
#include "opentherm.hpp"

int main() {
    stdio_init_all();
    
    // Initialize CYW43 for LED
    cyw43_arch_init();
    
    // Create OpenTherm interface (TX=GPIO16, RX=GPIO17)
    OpenTherm::Interface ot(16, 17);
    
    // Optional: Set custom timeout (default is 1000ms)
    ot.setTimeout(2000);
    
    while (true) {
        // Read boiler temperature
        float boiler_temp;
        if (ot.readBoilerTemperature(&boiler_temp)) {
            printf("Boiler: %.2f°C\n", boiler_temp);
        }
        
        // Read DHW temperature
        float dhw_temp;
        if (ot.readDHWTemperature(&dhw_temp)) {
            printf("DHW: %.2f°C\n", dhw_temp);
        }
        
        // Read modulation level
        float modulation;
        if (ot.readModulationLevel(&modulation)) {
            printf("Modulation: %.1f%%\n", modulation);
        }
        
        // Write control setpoint
        if (ot.writeControlSetpoint(65.5f)) {
            printf("Setpoint updated to 65.5°C\n");
        }
        
        sleep_ms(5000);
    }
}
```

## Advanced Usage

### Reading Status and Configuration

```cpp
// Read complete status flags
opentherm_status_t status;
if (ot.readStatus(&status)) {
    printf("CH Enable: %d\n", status.ch_enable);
    printf("DHW Enable: %d\n", status.dhw_enable);
    printf("Flame: %d\n", status.flame);
    printf("Fault: %d\n", status.fault);
}

// Read slave configuration
opentherm_config_t config;
if (ot.readSlaveConfig(&config)) {
    printf("DHW Present: %d\n", config.dhw_present);
    printf("Control Type: %s\n", config.control_type ? "On/Off" : "Modulating");
}

// Read fault flags
opentherm_fault_t fault;
if (ot.readFaultFlags(&fault)) {
    printf("Fault Code: %u\n", fault.code);
    printf("Service Request: %d\n", fault.service_request);
}
```

### Reading Multiple Sensors

```cpp
// Temperature sensors
float outside_temp, return_temp, room_temp;
ot.readOutsideTemperature(&outside_temp);
ot.readReturnWaterTemperature(&return_temp);
ot.readRoomTemperature(&room_temp);

// Pressure and flow
float pressure, flow;
ot.readCHWaterPressure(&pressure);  // bar
ot.readDHWFlowRate(&flow);          // l/min

// Exhaust temperature (signed integer)
int16_t exhaust_temp;
ot.readExhaustTemperature(&exhaust_temp);
```

### Reading Counters and Statistics

```cpp
uint16_t burner_starts, burner_hours;
ot.readBurnerStarts(&burner_starts);
ot.readBurnerHours(&burner_hours);

uint16_t ch_pump_starts, ch_pump_hours;
ot.readCHPumpStarts(&ch_pump_starts);
ot.readCHPumpHours(&ch_pump_hours);
```

### Writing Setpoints

```cpp
// Control setpoint (CH water temperature)
ot.writeControlSetpoint(70.0f);

// Room temperature setpoint
ot.writeRoomSetpoint(21.5f);

// DHW setpoint
ot.writeDHWSetpoint(55.0f);

// Maximum CH water setpoint
ot.writeMaxCHSetpoint(80.0f);
```

### Low-Level Frame Access

```cpp
// Send raw frame
uint32_t request = opentherm_build_read_request(OT_DATA_ID_STATUS);
ot.send(request);

// Receive raw frame
uint32_t response;
if (ot.receive(response)) {
    OpenTherm::Interface::printFrame(response);
    
    // Extract data value
    float temp = opentherm_get_f8_8(response);
    uint16_t value = opentherm_get_u16(response);
    int16_t signed_val = opentherm_get_s16(response);
}
```

## API Reference

### OpenTherm::Interface Class

#### Constructor
```cpp
Interface(unsigned int tx_pin, unsigned int rx_pin, 
          PIO pio_tx = pio0, PIO pio_rx = pio1)
```

#### Configuration
- `void setTimeout(uint32_t timeout_ms)` - Set timeout for operations (default 1000ms)
- `uint32_t getTimeout() const` - Get current timeout

#### Temperature Sensors (°C)
- `bool readBoilerTemperature(float* temp)`
- `bool readDHWTemperature(float* temp)`
- `bool readOutsideTemperature(float* temp)`
- `bool readReturnWaterTemperature(float* temp)`
- `bool readRoomTemperature(float* temp)`
- `bool readExhaustTemperature(int16_t* temp)`

#### Pressure & Flow
- `bool readCHWaterPressure(float* pressure)` - bar
- `bool readDHWFlowRate(float* flow_rate)` - l/min

#### Status & Configuration
- `bool readStatus(opentherm_status_t* status)`
- `bool readSlaveConfig(opentherm_config_t* config)`
- `bool readFaultFlags(opentherm_fault_t* fault)`

#### Setpoints
- `bool readControlSetpoint(float* setpoint)`
- `bool readDHWSetpoint(float* setpoint)`
- `bool readMaxCHSetpoint(float* setpoint)`
- `bool writeControlSetpoint(float temperature)`
- `bool writeRoomSetpoint(float temperature)`
- `bool writeDHWSetpoint(float temperature)`
- `bool writeMaxCHSetpoint(float temperature)`

#### Modulation & Counters
- `bool readModulationLevel(float* level)` - percentage
- `bool readBurnerStarts(uint16_t* count)`
- `bool readBurnerHours(uint16_t* hours)`
- `bool readCHPumpStarts(uint16_t* count)`
- `bool readCHPumpHours(uint16_t* hours)`
- `bool readDHWPumpStarts(uint16_t* count)`
- `bool readDHWPumpHours(uint16_t* hours)`

#### Version Information
- `bool readOpenThermVersion(float* version)`
- `bool readSlaveVersion(uint8_t* type, uint8_t* version)`

#### Low-Level Access
- `void send(uint32_t frame)` - Send raw frame
- `bool receive(uint32_t& frame)` - Receive raw frame (non-blocking)
- `static void printFrame(uint32_t frame_data)` - Print frame details

## Project Structure

```
.
├── CMakeLists.txt              # Main CMake configuration
├── build.sh                    # Build automation script
├── pico-sdk/                   # Pico SDK submodule
├── pico-kvstore/               # Key-value storage library submodule
├── picotool/                   # Picotool submodule
├── src/
│   ├── main.cpp                # Main gateway application with WiFi/MQTT
│   ├── opentherm.hpp           # OpenTherm library header
│   ├── opentherm.cpp           # OpenTherm library implementation
│   ├── opentherm_ha.hpp        # Home Assistant MQTT integration header
│   ├── opentherm_ha.cpp        # Home Assistant MQTT implementation
│   ├── config.hpp              # Configuration system header
│   ├── config.cpp              # Configuration system implementation
│   ├── lwipopts.h              # lwIP network stack configuration
│   ├── opentherm_write.pio     # PIO TX Manchester encoder
│   └── opentherm_read.pio      # PIO RX Manchester decoder
├── home_assistant/             # Home Assistant configuration examples
│   ├── README.md               # Setup instructions
│   ├── dashboard_overview.yaml # Complete overview dashboard
│   ├── dashboard_graphs.yaml   # Historical data visualization
│   ├── dashboard_control.yaml  # Control panel dashboard
│   ├── dashboard_mobile.yaml   # Mobile-optimized dashboard
│   ├── entity_cards.yaml       # Individual entity card examples
│   └── automations.yaml        # Automation examples
└── protocol/
    └── openthermProtocol.txt   # OpenTherm v2.2 specification
```

## How It Works

### PIO-Based Manchester Encoding

The TX PIO program automatically converts each data bit into Manchester encoding:
- Bit `1` = Active-to-Idle transition
- Bit `0` = Idle-to-Active transition

You simply send a 32-bit frame, and the PIO handles all timing and encoding in hardware.

### PIO-Based Manchester Decoding

The RX PIO program samples the incoming Manchester signal at 2x the bit rate and stores raw samples. The software then decodes these samples back to the original 32-bit frame, verifies parity, and returns the data.

### Frame Structure

Each OpenTherm frame contains:
- **Parity bit** (1 bit)
- **Message type** (3 bits): READ-DATA, WRITE-DATA, READ-ACK, etc.
- **Spare bits** (4 bits)
- **Data ID** (8 bits): Identifies the data item
- **Data value** (16 bits): The actual data (format depends on Data ID)

### Data Types

The library automatically handles different data formats:
- **f8.8**: Fixed-point temperature (-40 to 127°C)
- **u16**: Unsigned 16-bit integer (counters, hours)
- **s16**: Signed 16-bit integer (exhaust temperature)
- **u8/u8**: Two 8-bit values (flags, versions)
- **flags**: Bitfield structures for status and configuration

## Connection Behavior

The gateway implements robust connection handling:

### Startup Sequence
1. Initialize WiFi and attempt connection (30-second timeout per attempt)
2. If WiFi fails, blink LED 2 times and retry after 5 seconds
3. Once WiFi connected, attempt MQTT connection
4. If MQTT fails, blink LED 3 times and retry after 3 seconds
5. Once both connected, proceed to normal operation

### Connection Monitoring
- Checks WiFi and MQTT status every 5 seconds
- Automatically reconnects if either connection is lost
- Shows appropriate LED error pattern while reconnecting
- Re-publishes Home Assistant discovery after successful reconnection

### Configurable Timeouts

All timeouts can be adjusted in `src/main.cpp`:

```cpp
#define WIFI_CONNECT_TIMEOUT_MS 30000      // WiFi connection attempt timeout
#define WIFI_RETRY_DELAY_MS 5000           // Delay between WiFi retries
#define MQTT_RETRY_DELAY_MS 3000           // Delay between MQTT retries
#define CONNECTION_CHECK_DELAY_MS 5000     // Connection health check interval
```

## Debugging

Enable USB serial output to see debug messages:
```cpp
stdio_init_all();
sleep_ms(2000);  // Wait for USB connection
printf("Debug message\n");
```

Connect via serial terminal at 115200 baud to see:
- Connection attempt logs
- MQTT publish/subscribe events
- OpenTherm frame data
- Sensor readings and updates

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## References

- [OpenTherm Protocol Specification v2.2](http://www.opentherm.eu/)
- [Raspberry Pi Pico SDK Documentation](https://www.raspberrypi.com/documentation/pico-sdk/)
- [PIO Documentation](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_pio)