# PicoOpenTherm - Complete Entity Reference

This document lists all available Home Assistant entities published by the PicoOpenTherm gateway and suggests potential additions.

## Currently Implemented Entities

### Binary Sensors (Status Indicators)
| Entity ID | Name | Description |
|-----------|------|-------------|
| `binary_sensor.opentherm_gw_fault` | Fault | Boiler fault status |
| `binary_sensor.opentherm_gw_ch_mode` | Central Heating Mode | CH currently active |
| `binary_sensor.opentherm_gw_dhw_mode` | Hot Water Mode | DHW currently active |
| `binary_sensor.opentherm_gw_flame` | Flame Status | Burner flame on/off |
| `binary_sensor.opentherm_gw_cooling` | Cooling Active | Cooling mode active |
| `binary_sensor.opentherm_gw_diagnostic` | Diagnostic Mode | System diagnostic mode |
| `binary_sensor.opentherm_gw_dhw_present` | DHW Present | Boiler has DHW capability |
| `binary_sensor.opentherm_gw_cooling_supported` | Cooling Supported | Boiler supports cooling |
| `binary_sensor.opentherm_gw_ch2_present` | CH2 Present | Second heating circuit present |

### Switches (Controls)
| Entity ID | Name | Description |
|-----------|------|-------------|
| `switch.opentherm_gw_ch_enable` | Central Heating Enable | Enable/disable CH |
| `switch.opentherm_gw_dhw_enable` | Hot Water Enable | Enable/disable DHW |

### Sensors (Temperature)
| Entity ID | Name | Unit | Description |
|-----------|------|------|-------------|
| `sensor.opentherm_gw_boiler_temp` | Boiler Temperature | °C | CH water temperature |
| `sensor.opentherm_gw_dhw_temp` | Hot Water Temperature | °C | DHW temperature |
| `sensor.opentherm_gw_return_temp` | Return Water Temperature | °C | Return circuit temp |
| `sensor.opentherm_gw_outside_temp` | Outside Temperature | °C | Outside temperature |
| `sensor.opentherm_gw_room_temp` | Room Temperature | °C | Room temperature |
| `sensor.opentherm_gw_exhaust_temp` | Exhaust Temperature | °C | Exhaust gas temp |

### Numbers (Setpoints)
| Entity ID | Name | Unit | Range | Description |
|-----------|------|------|-------|-------------|
| `number.opentherm_gw_control_setpoint` | Control Setpoint | °C | 0-100 | CH water setpoint |
| `number.opentherm_gw_room_setpoint` | Room Setpoint | °C | 5-30 | Room temp setpoint |
| `number.opentherm_gw_dhw_setpoint` | Hot Water Setpoint | °C | 30-90 | DHW setpoint |
| `number.opentherm_gw_max_ch_setpoint` | Max CH Setpoint | °C | 30-90 | Maximum CH setpoint |

### Sensors (Performance)
| Entity ID | Name | Unit | Description |
|-----------|------|------|-------------|
| `sensor.opentherm_gw_modulation` | Modulation Level | % | Current flame modulation |
| `sensor.opentherm_gw_max_modulation` | Max Modulation Level | % | Maximum modulation |
| `sensor.opentherm_gw_pressure` | CH Water Pressure | bar | System pressure |
| `sensor.opentherm_gw_dhw_flow` | Hot Water Flow Rate | l/min | DHW flow rate |

### Sensors (Statistics)
| Entity ID | Name | Unit | Description |
|-----------|------|------|-------------|
| `sensor.opentherm_gw_burner_starts` | Burner Starts | starts | Total burner starts |
| `sensor.opentherm_gw_ch_pump_starts` | CH Pump Starts | starts | Total CH pump starts |
| `sensor.opentherm_gw_dhw_pump_starts` | DHW Pump Starts | starts | Total DHW pump starts |
| `sensor.opentherm_gw_burner_hours` | Burner Operating Hours | h | Total burner hours |
| `sensor.opentherm_gw_ch_pump_hours` | CH Pump Operating Hours | h | Total CH pump hours |
| `sensor.opentherm_gw_dhw_pump_hours` | DHW Pump Operating Hours | h | Total DHW pump hours |

### Sensors (Diagnostics)
| Entity ID | Name | Description |
|-----------|------|-------------|
| `sensor.opentherm_gw_fault_code` | Fault Code | OEM fault code |
| `sensor.opentherm_gw_diagnostic_code` | Diagnostic Code | OEM diagnostic code |
| `sensor.opentherm_gw_opentherm_version` | OpenTherm Version | Protocol version |

### Sensors (Time & Date)
| Entity ID | Name | Description |
|-----------|------|-------------|
| `sensor.opentherm_gw_day_of_week` | Day of Week | Boiler day of week |
| `sensor.opentherm_gw_time_of_day` | Time of Day | Boiler time (HH:MM) |
| `sensor.opentherm_gw_date` | Date | Boiler date (MM/DD) |
| `sensor.opentherm_gw_year` | Year | Boiler year |

### Buttons (Actions)
| Entity ID | Name | Description |
|-----------|------|-------------|
| `button.opentherm_gw_sync_time` | Sync Time to Boiler | Synchronize time from HA |
| `button.opentherm_gw_restart` | Restart Gateway | Restart the gateway |

### Sensors (Temperature Bounds)
| Entity ID | Name | Unit | Description |
|-----------|------|------|-------------|
| `sensor.opentherm_gw_dhw_setpoint_min` | DHW Setpoint Min | °C | Min DHW temperature |
| `sensor.opentherm_gw_dhw_setpoint_max` | DHW Setpoint Max | °C | Max DHW temperature |
| `sensor.opentherm_gw_ch_setpoint_min` | CH Setpoint Min | °C | Min CH temperature |
| `sensor.opentherm_gw_ch_setpoint_max` | CH Setpoint Max | °C | Max CH temperature |

### Sensors (WiFi & Network)
| Entity ID | Name | Unit | Description |
|-----------|------|------|-------------|
| `sensor.opentherm_gw_wifi_rssi` | WiFi Signal Strength | dBm | WiFi signal strength |
| `sensor.opentherm_gw_wifi_link_status` | WiFi Link Status | - | Connection status |
| `sensor.opentherm_gw_ip_address` | IP Address | - | Gateway IP address |
| `sensor.opentherm_gw_wifi_ssid` | WiFi SSID | - | Connected network name |

### Sensors (System Health)
| Entity ID | Name | Unit | Description |
|-----------|------|------|-------------|
| `sensor.opentherm_gw_uptime` | Uptime | s | Gateway uptime in seconds |
| `sensor.opentherm_gw_free_heap` | Free Heap Memory | B | Available RAM |

### Text Entities (Configuration)
| Entity ID | Name | Description |
|-----------|------|-------------|
| `text.opentherm_gw_device_name` | Device Name | Gateway device name |
| `text.opentherm_gw_device_id` | Device ID | Gateway unique ID |

### Numbers (Configuration)
| Entity ID | Name | Unit | Range | Description |
|-----------|------|------|-------|-------------|
| `number.opentherm_gw_opentherm_tx_pin` | OpenTherm TX Pin | - | 0-28 | GPIO TX pin |
| `number.opentherm_gw_opentherm_rx_pin` | OpenTherm RX Pin | - | 0-28 | GPIO RX pin |
| `number.opentherm_gw_update_interval` | Update Interval | ms | 1000-300000 | Sensor update interval |

## Total Entity Count: **49 entities**

- 9 Binary Sensors
- 2 Switches
- 27 Sensors
- 4 Numbers (Setpoints)
- 2 Buttons
- 2 Text Entities
- 3 Numbers (Configuration)

## Potential Missing Entities

### High Priority Additions

#### 1. **Master/Slave Status Indicators** (OpenTherm Protocol)
- `binary_sensor.opentherm_gw_master_ch_enabled` - Master requests CH
- `binary_sensor.opentherm_gw_master_dhw_enabled` - Master requests DHW
- `binary_sensor.opentherm_gw_master_cooling_enabled` - Master requests cooling
- `binary_sensor.opentherm_gw_ch2_mode` - Second heating circuit active

#### 2. **Additional Temperature Sensors** (if supported by boiler)
- `sensor.opentherm_gw_solar_storage_temp` - Solar storage temperature (OT ID 31)
- `sensor.opentherm_gw_solar_collector_temp` - Solar collector temperature (OT ID 32)
- `sensor.opentherm_gw_ch2_water_temp` - CH2 circuit temperature (OT ID 28)
- `sensor.opentherm_gw_ch2_setpoint` - CH2 setpoint (OT ID 8)

#### 3. **Enhanced Diagnostics**
- `sensor.opentherm_gw_burner_failures` - Unsuccessful burner starts (OT ID 115)
- `sensor.opentherm_gw_dhw_burner_starts` - DHW-specific burner starts (OT ID 117)
- `sensor.opentherm_gw_dhw_burner_hours` - DHW-specific burner hours (OT ID 118)

#### 4. **OEM-Specific Data**
- `sensor.opentherm_gw_oem_fault_code` - OEM fault code (OT ID 5)
- `sensor.opentherm_gw_service_request` - Service request indicator
- `sensor.opentherm_gw_lockout_reset` - Lockout/reset indicator

#### 5. **Remote Parameters** (if supported)
- `number.opentherm_gw_ch2_setpoint` - CH2 setpoint control
- `number.opentherm_gw_cooling_control` - Cooling control signal

#### 6. **System Performance Metrics**
- `sensor.opentherm_gw_max_capacity` - Maximum boiler capacity (kW)
- `sensor.opentherm_gw_min_modulation_level` - Minimum modulation level (%)
- `sensor.opentherm_gw_dhw_bounds_min` - DHW lower bound
- `sensor.opentherm_gw_dhw_bounds_max` - DHW upper bound

### Medium Priority Additions

#### 7. **Network Statistics**
- `sensor.opentherm_gw_mqtt_messages_sent` - MQTT messages published
- `sensor.opentherm_gw_mqtt_messages_failed` - Failed MQTT publishes
- `sensor.opentherm_gw_reconnection_count` - WiFi reconnection count
- `sensor.opentherm_gw_last_reconnection` - Last reconnection timestamp

#### 8. **System Diagnostics**
- `sensor.opentherm_gw_cpu_temperature` - RP2040 internal temperature
- `sensor.opentherm_gw_total_heap` - Total RAM available
- `sensor.opentherm_gw_heap_usage_percent` - Memory usage percentage

#### 9. **Communication Statistics**
- `sensor.opentherm_gw_successful_requests` - Successful OT requests
- `sensor.opentherm_gw_failed_requests` - Failed OT requests
- `sensor.opentherm_gw_timeout_count` - Communication timeouts
- `sensor.opentherm_gw_last_communication` - Last successful OT message

### Low Priority / Nice-to-Have

#### 10. **Historical Statistics** (calculated)
- `sensor.opentherm_gw_avg_boiler_temp_1h` - Average boiler temp (1 hour)
- `sensor.opentherm_gw_avg_modulation_1h` - Average modulation (1 hour)
- `sensor.opentherm_gw_burner_runtime_today` - Today's burner runtime
- `sensor.opentherm_gw_energy_estimate_today` - Estimated energy use today

#### 11. **Advanced Controls**
- `select.opentherm_gw_ch_mode` - Select CH mode (auto/manual/off)
- `select.opentherm_gw_comfort_mode` - Comfort/eco/schedule modes
- `switch.opentherm_gw_summer_winter` - Summer/winter mode toggle

#### 12. **Configuration Helpers**
- `button.opentherm_gw_factory_reset` - Factory reset configuration
- `button.opentherm_gw_clear_statistics` - Clear all counters
- `switch.opentherm_gw_auto_discovery` - Enable/disable auto-discovery

## Implementation Notes

### Already Fully Implemented ✅
- All basic OpenTherm status, temperature, and control entities
- Time/Date synchronization with automations
- WiFi monitoring and network statistics
- System health monitoring (uptime, memory)
- Temperature bounds (read from boiler)
- Complete MQTT discovery integration
- Dashboard views and card examples

### Currently NOT Implemented ❌
- Master/slave status bits (separate from current status)
- Solar thermal sensors (OT IDs 31-32)
- CH2 support (second heating circuit)
- Burner failure statistics
- OEM diagnostic data beyond basic fault codes
- Network/MQTT statistics counters
- CPU temperature monitoring
- OpenTherm communication statistics
- Historical/calculated metrics

### Implementation Priority

**Immediate (Next Release):**
1. None required - all essential entities are implemented

**Short Term (Good additions if user requests):**
1. Master/Slave status bits (easy to add, good for diagnostics)
2. OEM fault code (OT ID 5) - separate from diagnostic code
3. MQTT/Network statistics (useful for troubleshooting)

**Long Term (If requested or needed):**
1. Solar thermal support (requires hardware support)
2. CH2 support (uncommon feature)
3. Historical statistics (requires state tracking/storage)
4. CPU temperature monitoring (requires RP2040 ADC)

## Dashboard Coverage

All implemented entities have corresponding dashboard cards in:
- `home_assistant/dashboard_overview.yaml` - Main overview
- `home_assistant/dashboard_control.yaml` - Control panel
- `home_assistant/dashboard_graphs.yaml` - Historical graphs
- `home_assistant/dashboard_mobile.yaml` - Mobile-optimized
- `home_assistant/dashboard_wifi.yaml` - WiFi monitoring
- `home_assistant/dashboard_system.yaml` - System & time (NEW)
- `home_assistant/entity_cards.yaml` - Individual card examples
- `home_assistant/automations.yaml` - Automation examples

## Conclusion

The PicoOpenTherm gateway currently implements **49 entities** covering:
- ✅ All essential OpenTherm status and control
- ✅ Complete temperature monitoring
- ✅ System statistics and diagnostics
- ✅ Time/Date synchronization
- ✅ WiFi and network monitoring
- ✅ Gateway configuration
- ✅ System health

The only notable missing features are **niche/advanced** capabilities:
- Master/slave status bits (diagnostic detail)
- Solar thermal sensors (uncommon hardware)
- CH2 support (rare configuration)
- Communication statistics (advanced debugging)
- Historical metrics (requires storage)

For most residential heating systems, the current implementation is **complete and comprehensive**.
