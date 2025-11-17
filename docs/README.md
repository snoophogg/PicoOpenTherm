# PicoOpenTherm Documentation

Complete documentation for PicoOpenTherm - OpenTherm v2.2 protocol implementation for Raspberry Pi Pico W with Home Assistant integration.

## Table of Contents

### Getting Started
- [Main README](../README.md) - Project overview, features, and quick start guide
- [Provisioning](PROVISIONING.md) - How to configure WiFi and MQTT settings on first boot
- [Hardware Setup](../README.md#hardware-setup) - OpenTherm adapter connection guide

### Home Assistant Integration
- **[Home Assistant Integration Guide](HOME_ASSISTANT.md)** - Complete MQTT auto-discovery and entity reference
  - 55 auto-discovered entities
  - Time synchronization from Home Assistant
  - Temperature bounds monitoring
  - MQTT topic reference
  - Troubleshooting guide

- **[Home Assistant Examples](HOME_ASSISTANT_EXAMPLES.md)** - Ready-to-use configurations
  - Dashboard YAML examples
  - Automation templates
  - Card configurations
  - Custom integrations

### Development & Testing
- **[Simulator](SIMULATOR.md)** - Host-based OpenTherm boiler simulator
  - Test without physical hardware
  - Multi-device testing
  - Automated test scenarios

- **[Host Simulator](HOST_SIMULATOR.md)** - Advanced simulation features
  - Network testing
  - Protocol validation
  - Integration testing

### Advanced Features
- **[Key-Value Store Utility](KVSTORE-UTIL.md)** - Flash configuration management
  - Read/write persistent configuration
  - Factory reset procedures
  - Backup and restore

## Quick Links by Topic

### For Users

**First-Time Setup:**
1. [Flash firmware](../README.md#build--flash)
2. [Provision WiFi/MQTT](PROVISIONING.md)
3. [Configure Home Assistant](HOME_ASSISTANT.md#setup)
4. [Add automations](HOME_ASSISTANT_EXAMPLES.md#automation-examples)

**Feature Guides:**
- [Time Synchronization](HOME_ASSISTANT.md#time-synchronization-new) - Sync boiler clock from HA
- [Temperature Bounds](HOME_ASSISTANT.md#temperature-bounds-monitoring-new) - Monitor setpoint limits
- [MQTT Topics](HOME_ASSISTANT.md#mqtt-topics) - Understanding the topic structure
- [Troubleshooting](HOME_ASSISTANT.md#troubleshooting) - Common issues and solutions

**Dashboard & Automation:**
- [Overview Dashboard](HOME_ASSISTANT_EXAMPLES.md#overview-dashboard) - Complete system view
- [Heating Schedules](HOME_ASSISTANT_EXAMPLES.md#heating-schedules) - Time-based automation
- [Safety Alerts](HOME_ASSISTANT_EXAMPLES.md#safety-alerts) - Temperature and pressure monitoring
- [Weather Compensation](HOME_ASSISTANT_EXAMPLES.md#weather-compensation) - Adaptive heating

### For Developers

**Development:**
- [Building from Source](../README.md#quick-start)
- [Protocol Implementation](../README.md#opentherm-overview)
- [Testing with Simulator](SIMULATOR.md)
- [Host Development](HOST_SIMULATOR.md)

**Configuration:**
- [Runtime Configuration](KVSTORE-UTIL.md) - Managing flash storage
- [Provisioning Tool](PROVISIONING.md) - Configuration workflow
- [Network Setup](HOME_ASSISTANT.md#network-configuration)

**API Reference:**
- [C++ API Usage](HOME_ASSISTANT.md#api-usage)
- [MQTT Integration](HOME_ASSISTANT.md#mqtt-topics)
- [OpenTherm Protocol](../README.md#opentherm-protocol-implementation)

## Entity Reference

### Complete Entity List (55 Total)

**Binary Sensors (9):**
- System status (fault, flame, CH mode, DHW mode, cooling, diagnostic)
- Capabilities (DHW present, cooling supported, CH2 present)

**Switches (2):**
- Central Heating Enable
- Domestic Hot Water Enable

**Temperature Sensors (6):**
- Boiler, DHW, Return, Outside, Room, Exhaust

**Setpoint Controls (4):**
- Control Setpoint, Room Setpoint, DHW Setpoint, Max CH Setpoint

**Performance Sensors (4):**
- Modulation level, Max modulation, Pressure, DHW flow

**Statistics (6):**
- Burner/CH/DHW pump starts and operating hours

**Time/Date Sensors (4):**
- Day of week, Time, Date, Year

**Temperature Bounds (4):**
- DHW and CH min/max setpoints

**Other Sensors (3):**
- Fault code, Diagnostic code, OpenTherm version

**Configuration (4):**
- Device name, Device ID, TX/RX GPIO pins

**Action Buttons (1):**
- Sync Time to Boiler

See [HOME_ASSISTANT.md](HOME_ASSISTANT.md#exposed-entities) for detailed entity descriptions.

## MQTT Topics Overview

### State Topics (Published by Gateway)
```
opentherm/state/boiler_temp
opentherm/state/dhw_temp
opentherm/state/flame
opentherm/state/modulation
... (all sensor data)
```

### Command Topics (Subscribed by Gateway)
```
opentherm/cmd/ch_enable        # ON/OFF
opentherm/cmd/dhw_enable       # ON/OFF
opentherm/cmd/control_setpoint # Float (°C)
opentherm/cmd/sync_time        # ISO 8601 or Unix timestamp
```

### Discovery Topics
```
homeassistant/{component}/opentherm_gw/{entity}/config
```

See [HOME_ASSISTANT.md - MQTT Topics](HOME_ASSISTANT.md#mqtt-topics) for complete reference.

## New Features

### Time Synchronization ⏰
Sync your boiler's clock from Home Assistant:
- Button entity for manual sync
- Automated daily sync
- DST change detection
- ISO 8601 and Unix timestamp support

**Documentation:** [Time Synchronization Guide](HOME_ASSISTANT.md#time-synchronization-new)

### Temperature Bounds 🌡️
Monitor your boiler's setpoint limits:
- DHW min/max sensors
- CH min/max sensors
- Automatic bounds validation
- UI integration examples

**Documentation:** [Temperature Bounds Guide](HOME_ASSISTANT.md#temperature-bounds-monitoring-new)

## Troubleshooting

**Common Issues:**
- [WiFi Connection Problems](PROVISIONING.md#troubleshooting)
- [MQTT Not Connecting](HOME_ASSISTANT.md#entities-not-appearing)
- [Time Sync Issues](HOME_ASSISTANT.md#troubleshooting-time-sync)
- [Entities Not Appearing](HOME_ASSISTANT.md#troubleshooting)

**LED Error Codes:**
- 1 blink: Normal operation
- 2 blinks: WiFi error
- 3 blinks: MQTT error
- Rapid blinking: Fatal error

## Examples & Templates

### Quick Start Automations

**Daily Time Sync:**
```yaml
- id: opentherm_daily_time_sync
  trigger:
    - platform: time
      at: "03:00:00"
  action:
    - service: mqtt.publish
      data:
        topic: "opentherm/cmd/sync_time"
        payload: "{{ now().strftime('%Y-%m-%dT%H:%M:%S') }}"
```

**Temperature Alert:**
```yaml
- id: high_temp_alert
  trigger:
    - platform: numeric_state
      entity_id: sensor.opentherm_gw_boiler_temp
      above: 85
  action:
    - service: notify.notify
      data:
        message: "Boiler temperature high: {{ states('sensor.opentherm_gw_boiler_temp') }}°C"
```

More examples: [HOME_ASSISTANT_EXAMPLES.md](HOME_ASSISTANT_EXAMPLES.md)

## Support & Contribution

- **Issues**: Report bugs on GitHub
- **Documentation**: All docs are in this folder
- **Examples**: Share your automations and dashboards!

## License

See main repository LICENSE file.
