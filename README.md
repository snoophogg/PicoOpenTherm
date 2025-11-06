# PicoOpenTherm

A complete OpenTherm v2.2 protocol implementation for Raspberry Pi Pico W using PIO (Programmable I/O) state machines for hardware-accelerated Manchester encoding/decoding.

## Features

- **Hardware Manchester Encoding**: PIO-based TX for 1000 bps Manchester encoding
- **Hardware Manchester Decoding**: PIO-based RX with automatic sampling
- **Complete OpenTherm Protocol**: Full implementation of OpenTherm v2.2 specification
- **High-Level API**: Easy-to-use C++ interface for reading sensors and controlling setpoints
- **Comprehensive Data IDs**: Support for all standard OpenTherm data IDs
- **Type-Safe Conversions**: Automatic f8.8 to float, s16, and flag decoding
- **Configurable Timeout**: Adjustable timeout for all read/write operations
- **Built with CMake**: Uses official Pico SDK
- **Optimized for Pico W**: Takes advantage of dual PIO blocks

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

Connect your OpenTherm device to the Pico W:

- **TX Pin** (default GPIO 16): Connect to OpenTherm device RX
- **RX Pin** (default GPIO 17): Connect to OpenTherm device TX
- **Ground**: Common ground connection

**Important**: OpenTherm uses a voltage signaling standard. You may need appropriate level shifting or isolation circuitry depending on your specific hardware.

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

This will generate two firmware files:
- `blink_pio.uf2` - Simple PIO blink example
- `opentherm_test.uf2` - OpenTherm protocol test program

## Flashing

1. Connect your Pico W while holding the BOOTSEL button
2. Copy the generated `.uf2` file to the Pico W drive
3. The Pico will automatically reboot and start running the program

## Quick Start Example

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
├── pico_sdk_import.cmake       # Pico SDK import helper
├── build.sh                    # Build automation script
├── pico-sdk/                   # Pico SDK submodule
├── picotool/                   # Picotool submodule
├── src/
│   ├── blink.c                 # Simple PIO blink example
│   ├── blink.pio               # PIO blink program
│   ├── opentherm.hpp           # OpenTherm library header
│   ├── opentherm.cpp           # OpenTherm library implementation
│   ├── opentherm_test.cpp      # OpenTherm test program
│   ├── opentherm_write.pio     # PIO TX Manchester encoder
│   └── opentherm_read.pio      # PIO RX Manchester decoder
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

## Debugging

Enable USB serial output to see debug messages:
```cpp
stdio_init_all();
sleep_ms(2000);  // Wait for USB connection
printf("Debug message\n");
```

Connect via serial terminal at 115200 baud.

## License

This project is open source. See LICENSE file for details.

## References

- [OpenTherm Protocol Specification v2.2](http://www.opentherm.eu/)
- [Raspberry Pi Pico SDK Documentation](https://www.raspberrypi.com/documentation/pico-sdk/)
- [PIO Documentation](https://www.raspberrypi.com/documentation/pico-sdk/hardware.html#hardware_pio)