# Home Assistant MQTT Integration

This library provides a complete Home Assistant integration for the OpenTherm gateway using MQTT auto-discovery.

## Features

- **Automatic MQTT Discovery**: All 55 entities are automatically discovered by Home Assistant
- **Comprehensive Sensor Coverage**: Temperature, pressure, flow, modulation, and status sensors
- **Binary Sensors**: Fault, flame, CH mode, DHW mode, cooling status
- **Control Switches**: Enable/disable central heating and hot water
- **Number Entities**: Adjustable setpoints for control, room, DHW, and max CH temperatures
- **Counter Sensors**: Burner starts, pump starts, and operating hours
- **Time Synchronization** (NEW!): Sync boiler clock from Home Assistant via button or automation
- **Time/Date Monitoring** (NEW!): Read boiler's current day, time, date, and year
- **Temperature Bounds** (NEW!): Monitor boiler-reported min/max limits for setpoints
- **Configuration Sensors**: System capabilities and version information

## Exposed Entities

### Binary Sensors
- `binary_sensor.opentherm_gw_fault` - System fault status
- `binary_sensor.opentherm_gw_ch_mode` - Central heating mode active
- `binary_sensor.opentherm_gw_dhw_mode` - Hot water mode active
- `binary_sensor.opentherm_gw_flame` - Burner flame status
- `binary_sensor.opentherm_gw_cooling` - Cooling active
- `binary_sensor.opentherm_gw_diagnostic` - Diagnostic mode
- `binary_sensor.opentherm_gw_dhw_present` - DHW capability present
- `binary_sensor.opentherm_gw_cooling_supported` - Cooling capability
- `binary_sensor.opentherm_gw_ch2_present` - Second heating circuit present

### Switches
- `switch.opentherm_gw_ch_enable` - Enable/disable central heating
- `switch.opentherm_gw_dhw_enable` - Enable/disable hot water

### Temperature Sensors
- `sensor.opentherm_gw_boiler_temp` - Boiler water temperature (°C)
- `sensor.opentherm_gw_dhw_temp` - Hot water temperature (°C)
- `sensor.opentherm_gw_return_temp` - Return water temperature (°C)
- `sensor.opentherm_gw_outside_temp` - Outside temperature (°C)
- `sensor.opentherm_gw_room_temp` - Room temperature (°C)
- `sensor.opentherm_gw_exhaust_temp` - Exhaust temperature (°C)

### Number Entities (Setpoints)
- `number.opentherm_gw_control_setpoint` - Control setpoint (0-100°C)
- `number.opentherm_gw_room_setpoint` - Room setpoint (5-30°C)
- `number.opentherm_gw_dhw_setpoint` - Hot water setpoint (30-90°C)
- `number.opentherm_gw_max_ch_setpoint` - Max CH setpoint (30-90°C)

### Other Sensors
- `sensor.opentherm_gw_modulation` - Current modulation level (%)
- `sensor.opentherm_gw_max_modulation` - Maximum modulation level (%)
- `sensor.opentherm_gw_pressure` - CH water pressure (bar)
- `sensor.opentherm_gw_dhw_flow` - Hot water flow rate (l/min)
- `sensor.opentherm_gw_fault_code` - OEM fault code
- `sensor.opentherm_gw_opentherm_version` - OpenTherm protocol version

### Counter Sensors
- `sensor.opentherm_gw_burner_starts` - Total burner starts
- `sensor.opentherm_gw_ch_pump_starts` - CH pump starts
- `sensor.opentherm_gw_dhw_pump_starts` - DHW pump starts
- `sensor.opentherm_gw_burner_hours` - Burner operating hours
- `sensor.opentherm_gw_ch_pump_hours` - CH pump operating hours
- `sensor.opentherm_gw_dhw_pump_hours` - DHW pump operating hours

### Time/Date Sensors (NEW!)
- `sensor.opentherm_gw_day_of_week` - Current day from boiler clock
- `sensor.opentherm_gw_time_of_day` - Current time from boiler (HH:MM format)
- `sensor.opentherm_gw_date` - Current date from boiler (MM/DD format)
- `sensor.opentherm_gw_year` - Current year from boiler

### Temperature Bounds Sensors (NEW!)
- `sensor.opentherm_gw_dhw_setpoint_min` - Minimum DHW setpoint allowed by boiler (°C)
- `sensor.opentherm_gw_dhw_setpoint_max` - Maximum DHW setpoint allowed by boiler (°C)
- `sensor.opentherm_gw_ch_setpoint_min` - Minimum CH setpoint allowed by boiler (°C)
- `sensor.opentherm_gw_ch_setpoint_max` - Maximum CH setpoint allowed by boiler (°C)

### Action Buttons (NEW!)
- `button.opentherm_gw_sync_time` - Sync current time from Home Assistant to boiler

## Setup

### 1. Hardware Configuration

Connect your OpenTherm interface to the Pico W:
- TX Pin: GPIO 16 (configurable)
- RX Pin: GPIO 17 (configurable)

### 2. Configure WiFi and MQTT

Edit `src/opentherm_ha_example.cpp` and update these settings:

```cpp
// WiFi credentials
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// MQTT broker settings
#define MQTT_SERVER_IP "192.168.1.100"  // Your MQTT broker IP
#define MQTT_SERVER_PORT 1883
```

### 3. Build and Flash

```bash
./build.sh
# Flash opentherm_ha.uf2 to your Pico W
```

### 4. Home Assistant Configuration

No manual configuration needed! The gateway uses MQTT auto-discovery. All entities will appear automatically in Home Assistant under:
- Device: "OpenTherm Gateway"
- Integration: MQTT

### 5. Custom Configuration (Optional)

You can customize the MQTT topics and device name in the code:

```cpp
OpenTherm::HomeAssistant::Config ha_config = {
    .device_name = "My Boiler",           // Device name in Home Assistant
    .device_id = "my_boiler",             // Unique device ID
    .mqtt_prefix = "homeassistant",       // HA discovery prefix
    .state_topic_base = "boiler/state",   // State topic base
    .command_topic_base = "boiler/cmd",   // Command topic base
    .auto_discovery = true,                // Enable auto-discovery
    .update_interval_ms = 10000            // Update interval (10 seconds)
};
```

## MQTT Topics

### State Topics (Published by Gateway)
All state topics follow the pattern: `opentherm/state/{sensor_name}`

Examples:
- `opentherm/state/boiler_temp`
- `opentherm/state/flame`
- `opentherm/state/modulation`

### Command Topics (Subscribed by Gateway)
All command topics follow the pattern: `opentherm/opentherm_gw/cmd/{control_name}`

Examples:
- `opentherm/opentherm_gw/cmd/ch_enable` - Payload: `ON` or `OFF`
- `opentherm/opentherm_gw/cmd/dhw_enable` - Payload: `ON` or `OFF`
- `opentherm/opentherm_gw/cmd/control_setpoint` - Payload: float (e.g., `65.5`)
- `opentherm/opentherm_gw/cmd/room_setpoint` - Payload: float (e.g., `21.0`)
- `opentherm/opentherm_gw/cmd/dhw_setpoint` - Payload: float (e.g., `55.0`)
- `opentherm/opentherm_gw/cmd/max_ch_setpoint` - Payload: float (e.g., `80.0`)

### Discovery Topics
Auto-discovery configs are published to:
`homeassistant/{component}/{device_id}/{object_id}/config`

Note on behavior:
- Discovery publishing is handled by the library's `OpenTherm::Discovery` helper. It publishes each entity's discovery
  config to the Home Assistant discovery topic and uses a retry + exponential backoff strategy to improve reliability on
  constrained networks. Discovery payloads include `unique_id`, `state_topic`, and device information so entities
  are automatically associated with the "OpenTherm Gateway" device in Home Assistant.
- For simulator usage the code embeds the `device_id` into the `state_topic_base` (for example
  `opentherm/state/<device_id>/...`) so multiple simulated devices can coexist on the same broker. You can achieve
  the same behavior for real devices by setting `state_topic_base` and `command_topic_base` in `HomeAssistant::Config`.

If you need custom fields (for example a `default_entity_id` or a different `model` string) you can edit the discovery
builder in `src/mqtt_discovery.cpp` or request an API extension; the library currently centralizes discovery logic in
`OpenTherm::Discovery::publishDiscoveryConfigs`.

## Usage in Home Assistant

### Example Automation - Turn on Heating

```yaml
automation:
  - alias: "Turn on heating in morning"
    trigger:
      - platform: time
        at: "06:00:00"
    action:
      - service: switch.turn_on
        target:
          entity_id: switch.opentherm_gw_ch_enable
      - service: number.set_value
        target:
          entity_id: number.opentherm_gw_control_setpoint
        data:
          value: 65
```

### Example Automation - DHW Boost

```yaml
automation:
  - alias: "DHW boost when temperature low"
    trigger:
      - platform: numeric_state
        entity_id: sensor.opentherm_gw_dhw_temp
        below: 45
    action:
      - service: number.set_value
        target:
          entity_id: number.opentherm_gw_dhw_setpoint
        data:
          value: 60
```

### Dashboard Card Example

```yaml
type: entities
title: OpenTherm Boiler
entities:
  - entity: sensor.opentherm_gw_boiler_temp
  - entity: sensor.opentherm_gw_dhw_temp
  - entity: sensor.opentherm_gw_return_temp
  - entity: sensor.opentherm_gw_modulation
  - entity: binary_sensor.opentherm_gw_flame
  - entity: switch.opentherm_gw_ch_enable
  - entity: number.opentherm_gw_control_setpoint
  - entity: sensor.opentherm_gw_pressure
```

## Troubleshooting

### Gateway Not Connecting to WiFi
- Check SSID and password in `opentherm_ha_example.cpp`
- Ensure your WiFi network is 2.4GHz (Pico W doesn't support 5GHz)
- Check serial output for error messages

### Entities Not Appearing in Home Assistant
- Verify MQTT broker is running and accessible
- Check MQTT broker IP and port in configuration
- Ensure Home Assistant MQTT integration is configured
- Check MQTT broker logs for incoming messages
- Verify discovery prefix matches Home Assistant (default: `homeassistant`)

### Sensors Not Updating
- Check `update_interval_ms` setting (default 10 seconds)
- Verify OpenTherm connection to boiler
- Check serial output for communication errors
- Ensure boiler supports requested data IDs

### Commands Not Working
- Verify the gateway is subscribed to command topics (check serial output)
- Test publishing manually to MQTT command topics
- Check that boiler supports write operations for the data ID
- Ensure status has been read at least once before sending control commands

## API Usage

### Basic Setup

```cpp
#include "opentherm_ha.hpp"

// Create OpenTherm interface
OpenTherm::Interface ot(TX_PIN, RX_PIN);

// Configure Home Assistant
OpenTherm::HomeAssistant::Config config = {
    .device_name = "My Boiler",
    .device_id = "my_boiler",
    .mqtt_prefix = "homeassistant",
    .state_topic_base = "boiler/state",
    .command_topic_base = "boiler/cmd",
    .auto_discovery = true,
    .update_interval_ms = 10000
};

OpenTherm::HomeAssistant::HAInterface ha(ot, config);

// Set up MQTT callbacks
OpenTherm::HomeAssistant::MQTTCallbacks callbacks = {
    .publish = my_mqtt_publish_function,
    .subscribe = my_mqtt_subscribe_function
};

// Initialize
ha.begin(callbacks);

// Main loop
while (true) {
    ha.update();  // Reads sensors and publishes to MQTT
    sleep_ms(100);
}
```

### Manual Control

```cpp
// Manually trigger sensor updates
ha.publishStatus();
ha.publishTemperatures();
ha.publishModulation();
ha.publishCounters();

// Set values
ha.setControlSetpoint(65.0);
ha.setRoomSetpoint(21.0);
ha.setCHEnable(true);
ha.setDHWEnable(true);
```

## Network Configuration

The example uses the Pico SDK's lwIP MQTT client with threadsafe background processing. This means:
- WiFi and MQTT run in the background
- Your main loop can focus on OpenTherm communication
- Network operations don't block sensor readings

## Performance

- **Update Interval**: Configurable (default 10 seconds)
- **MQTT QoS**: 0 (fire and forget) for sensor data
- **MQTT Retain**: Discovery configs are retained, sensor data is not
- **Network Overhead**: ~1-2KB per update cycle

## Time Synchronization (NEW!)

The gateway can synchronize your boiler's clock from Home Assistant, keeping it accurate and automatically adjusting for DST changes.

### How It Works

The gateway:
1. Reads the boiler's current time/date (OpenTherm Data IDs 20-22)
2. Publishes it to Home Assistant sensors
3. Accepts time sync commands from Home Assistant
4. Writes the synchronized time back to the boiler

### Using Time Sync

#### Method 1: Press the Button

In Home Assistant, go to your OpenTherm Gateway device and press the **Sync Time to Boiler** button. The current Home Assistant time will be sent to the boiler.

#### Method 2: Automated Daily Sync

Add this automation to `automations.yaml`:

```yaml
- id: opentherm_daily_time_sync
  alias: "OpenTherm: Daily Time Sync"
  description: "Sync boiler time daily at 3 AM"
  trigger:
    - platform: time
      at: "03:00:00"
  action:
    - service: mqtt.publish
      data:
        topic: "opentherm/opentherm_gw/cmd/sync_time"
        payload: "{{ now().strftime('%Y-%m-%dT%H:%M:%S') }}"
```

#### Method 3: Sync on Home Assistant Startup

```yaml
- id: opentherm_time_sync_on_startup
  alias: "OpenTherm: Time Sync on Startup"
  description: "Sync boiler time when Home Assistant starts"
  trigger:
    - platform: homeassistant
      event: start
  action:
    - delay: "00:01:00"  # Wait for MQTT connection
    - service: mqtt.publish
      data:
        topic: "opentherm/opentherm_gw/cmd/sync_time"
        payload: "{{ now().strftime('%Y-%m-%dT%H:%M:%S') }}"
```

### Time Sync Command Format

The gateway accepts two formats via MQTT:

**ISO 8601 (recommended):**
```
Topic: opentherm/opentherm_gw/cmd/sync_time
Payload: 2025-01-17T14:30:00
```

**Unix Timestamp:**
```
Topic: opentherm/opentherm_gw/cmd/sync_time
Payload: 1737121800
```

### Viewing Boiler Time

Monitor your boiler's clock with these sensors:
- `sensor.opentherm_gw_day_of_week` - e.g., "Monday"
- `sensor.opentherm_gw_time_of_day` - e.g., "14:30"
- `sensor.opentherm_gw_date` - e.g., "01/17"
- `sensor.opentherm_gw_year` - e.g., "2025"

### Daylight Saving Time

The gateway can automatically sync after DST changes. See the included `home_assistant/automations.yaml` for complete examples that detect:
- Spring forward (last Sunday in March for Europe)
- Fall back (last Sunday in October for Europe)

### Troubleshooting Time Sync

**Sync fails silently:**
- Not all boilers support time/date writes (Data IDs 20-22)
- Check the gateway's serial output for error messages
- The gateway will report which fields succeeded/failed

**Wrong timezone:**
- The gateway syncs the exact time from Home Assistant
- Ensure HA timezone is correct in `configuration.yaml`
- For Docker setups, set the `TZ` environment variable

**Time drifts:**
- Some boilers have poor internal clocks
- Set up daily sync automation to maintain accuracy
- Consider syncing after power outages

## Temperature Bounds Monitoring (NEW!)

The gateway reports your boiler's min/max temperature limits, helping you avoid invalid setpoints.

### Available Sensors

- `sensor.opentherm_gw_dhw_setpoint_min` - e.g., 40°C
- `sensor.opentherm_gw_dhw_setpoint_max` - e.g., 65°C
- `sensor.opentherm_gw_ch_setpoint_min` - e.g., 30°C
- `sensor.opentherm_gw_ch_setpoint_max` - e.g., 90°C

These sensors update once at startup and reflect the physical limits configured in your boiler.

### Automatic Bounds Validation

Add this automation to alert when you try to set a value outside the boiler's range:

```yaml
- id: opentherm_bounds_check
  alias: "OpenTherm: Setpoint Bounds Check"
  description: "Alert if setpoint exceeds boiler bounds"
  trigger:
    - platform: state
      entity_id:
        - number.opentherm_gw_dhw_setpoint
        - number.opentherm_gw_max_ch_setpoint
  condition:
    - condition: template
      value_template: >
        {% set entity = trigger.entity_id %}
        {% set value = trigger.to_state.state | float %}
        {% if 'dhw' in entity %}
          {% set min = states('sensor.opentherm_gw_dhw_setpoint_min') | float %}
          {% set max = states('sensor.opentherm_gw_dhw_setpoint_max') | float %}
        {% else %}
          {% set min = states('sensor.opentherm_gw_ch_setpoint_min') | float %}
          {% set max = states('sensor.opentherm_gw_ch_setpoint_max') | float %}
        {% endif %}
        {{ value < min or value > max }}
  action:
    - service: persistent_notification.create
      data:
        title: "⚠️ Setpoint Out of Bounds"
        message: >
          Requested setpoint is outside the boiler's valid range!
          The boiler may reject this value.
```

See `home_assistant/automations.yaml` for the complete version with detailed messages.

### Using Bounds in UI

Configure number entity ranges based on the boiler's actual limits:

```yaml
type: entities
entities:
  - entity: number.opentherm_gw_dhw_setpoint
    name: DHW Setpoint
    # These will automatically use the boiler's min/max if supported
```

For custom cards with sliders, use template sensors:

```yaml
template:
  - sensor:
      - name: "DHW Setpoint Range"
        state: >
          {{ states('sensor.opentherm_gw_dhw_setpoint_min') | float }} -
          {{ states('sensor.opentherm_gw_dhw_setpoint_max') | float }}°C
```

### Troubleshooting Bounds

**Sensors show "unknown":**
- Not all boilers support bounds queries (Data IDs 48-49)
- This is normal and doesn't affect functionality
- You can still set setpoints; the boiler will reject invalid values

**Bounds seem wrong:**
- Some boilers report factory defaults, not user-configured limits
- Cross-reference with your boiler's manual
- The gateway reports exactly what the boiler provides

## License

This library is part of the PicoOpenTherm project and follows the same license.
