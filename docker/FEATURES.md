# Pre-configured Home Assistant Features

This document lists all the pre-configured UI elements, automations, and scripts included in the Docker setup for PicoOpenTherm.

## Dashboard Views

### Main Dashboard (ui-lovelace.yaml)

**Home View:**
- Weather forecast card
- OpenTherm Gateway overview with key entities
- Quick access to thermostat control

**OpenTherm View:**
- Thermostat control card
- Status glance card (CH/DHW/Flame/Modulation/Pressure)
- Temperature entities list
- Temperature history graph (24h)
- Boiler activity graph (24h)
- Gateway configuration panel
- Network & connectivity information
- Diagnostics & statistics

### Dedicated OpenTherm Dashboard (lovelace/dashboards.yaml)

Accessible via sidebar → "OpenTherm Gateway"

- Gateway status badges (connection, uptime, WiFi)
- Full thermostat control
- Current temperatures (room, setpoint, boiler, return, DHW, outside)
- Temperature history (24h)
- Modulation & heating activity (24h)
- Runtime configuration (device name, ID, GPIO pins)
- Restart button
- Complete diagnostics (fault codes, burner stats, pump stats)

## Automations (automations.yaml)

All automations are **disabled by default** - enable as needed.

| ID | Name | Trigger | Action |
|----|------|---------|--------|
| `opentherm_low_pressure_alert` | Low Water Pressure Alert | Pressure < 1.0 bar for 5 min | Persistent notification |
| `opentherm_high_temp_alert` | High Boiler Temperature Alert | Boiler temp > 80°C for 5 min | Persistent notification |
| `opentherm_gateway_offline` | Gateway Offline Alert | Status unavailable for 5 min | Persistent notification |
| `opentherm_fault_code` | Fault Code Notification | Fault code changes (non-zero) | Persistent notification with codes |
| `opentherm_night_setback` | Night Setback | 22:00 daily | Set temperature to 18°C |
| `opentherm_morning_warmup` | Morning Warmup | 06:00 daily | Set temperature to 20°C |
| `opentherm_config_changed` | Configuration Change Notification | Config entity changes | Notify about restart |
| `opentherm_daily_summary` | Daily Statistics Summary | 23:59 daily | Daily stats notification |

## Scripts (scripts.yaml)

| Script | Parameters | Description |
|--------|------------|-------------|
| `set_comfort_temp` | `temperature` (default: 21°C) | Set thermostat to comfort mode |
| `set_eco_temp` | `temperature` (default: 16°C) | Set thermostat to eco mode |
| `boost_heating` | `temperature` (default: 23°C)<br>`duration` (default: 30 min) | Temporary temperature boost with auto-reset |
| `check_system_health` | None | Verify all parameters, report warnings |
| `schedule_maintenance_check` | None | Weekly maintenance checklist notification |
| `reset_gateway_config` | None | Reset device name, ID, and GPIO pins to defaults |

## Entity Reference

### Climate
- `climate.opentherm_thermostat` - Main thermostat control

### Sensors
- `sensor.opentherm_gw_status` - Connection status
- `sensor.opentherm_gw_uptime` - Device uptime
- `sensor.opentherm_gw_wifi_signal` - WiFi RSSI
- `sensor.opentherm_gw_ip_address` - Device IP
- `sensor.opentherm_gw_room_temperature` - Current room temperature
- `sensor.opentherm_gw_room_setpoint` - Room setpoint
- `sensor.opentherm_gw_boiler_water_temperature` - Boiler water temp
- `sensor.opentherm_gw_return_water_temperature` - Return water temp
- `sensor.opentherm_gw_dhw_temperature` - Domestic hot water temp
- `sensor.opentherm_gw_outside_temperature` - Outside temp (if available)
- `sensor.opentherm_gw_modulation_level` - Current modulation %
- `sensor.opentherm_gw_ch_water_pressure` - Central heating pressure
- `sensor.opentherm_gw_max_modulation_level` - Max modulation %
- `sensor.opentherm_gw_oem_fault_code` - Boiler fault code
- `sensor.opentherm_gw_oem_diagnostic_code` - Diagnostic code
- `sensor.opentherm_gw_burner_starts` - Burner start count
- `sensor.opentherm_gw_ch_pump_starts` - CH pump start count
- `sensor.opentherm_gw_dhw_pump_starts` - DHW pump start count
- `sensor.opentherm_gw_burner_hours` - Burner running hours
- `sensor.opentherm_gw_ch_pump_hours` - CH pump running hours
- `sensor.opentherm_gw_dhw_pump_hours` - DHW pump running hours

### Binary Sensors
- `binary_sensor.opentherm_gw_ch_active` - Central heating active
- `binary_sensor.opentherm_gw_dhw_active` - Hot water active
- `binary_sensor.opentherm_gw_flame_status` - Flame on/off

### Configuration Entities
- `text.opentherm_gw_device_name` - Device name (writable)
- `text.opentherm_gw_device_id` - Device ID (writable)
- `number.opentherm_gw_tx_pin` - TX GPIO pin (writable, 0-28)
- `number.opentherm_gw_rx_pin` - RX GPIO pin (writable, 0-28)

### Buttons
- `button.opentherm_gw_restart` - Restart the gateway

## Usage Examples

### Call a Script from Automation

```yaml
- service: script.boost_heating
  data:
    temperature: 23
    duration: 45
```

### Create a Custom Card

```yaml
type: button
entity: script.set_comfort_temp
tap_action:
  action: call-service
  service: script.set_comfort_temp
  data:
    temperature: 22
```

### Temperature Schedule Automation

```yaml
- alias: Weekend Morning
  trigger:
    - platform: time
      at: "08:00:00"
  condition:
    - condition: time
      weekday:
        - sat
        - sun
  action:
    - service: script.set_comfort_temp
      data:
        temperature: 21
```

## Customization Tips

1. **Adjust automation times:** Edit `automations.yaml` trigger times to match your schedule
2. **Change temperature presets:** Modify script default values
3. **Add notification services:** Replace `persistent_notification` with mobile app notifications
4. **Create dashboard shortcuts:** Add script entities to dashboard for one-tap control
5. **Customize alert thresholds:** Adjust numeric_state triggers in automations

## Converting to UI Mode

To enable UI editing of dashboards:

1. Edit `configuration.yaml`:
   ```yaml
   lovelace:
     mode: storage  # Change from 'yaml'
   ```

2. Restart Home Assistant

3. Your YAML configs become templates - edit via UI going forward

4. To export UI changes back to YAML, use Developer Tools → YAML

## Troubleshooting

### Dashboard not loading
- Check `configuration.yaml` syntax: `docker-compose exec homeassistant hass --script check_config`
- View logs: `docker-compose logs homeassistant`

### Automations not triggering
- Verify they're enabled: Settings → Automations & Scenes
- Check entity names match your actual entities
- Test trigger manually: Settings → Automations → Select automation → Run

### Scripts not appearing
- Restart Home Assistant after editing `scripts.yaml`
- Check YAML syntax in Developer Tools → YAML tab

### Entities not found
- Wait for PicoOpenTherm device to connect and publish MQTT discovery
- Check MQTT integration: Settings → Devices & Services → MQTT
- Verify MQTT topics: `docker exec -it picoopentherm-mosquitto mosquitto_sub -h localhost -t 'homeassistant/#' -v`
