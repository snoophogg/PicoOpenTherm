# Home Assistant Configuration Examples for OpenTherm Gateway

This folder contains ready-to-use dashboard configurations, entity cards, and automation examples for the OpenTherm Gateway.

## Contents

### Dashboard Files

- **`dashboard_overview.yaml`** - Complete overview dashboard with all entities organized by category
- **`dashboard_graphs.yaml`** - Historical temperature and performance graphs
- **`dashboard_control.yaml`** - Quick control interface with sliders and buttons
- **`dashboard_mobile.yaml`** - Mobile-optimized tabbed dashboard
- **`dashboard_wifi.yaml`** ⭐ **NEW!** - WiFi monitoring dashboard with signal strength trends, network diagnostics, and alerts
- **`entity_cards.yaml`** - Individual card examples for each entity type

### Automation Files

- **`automations.yaml`** - Example automations including:
  - Heating schedules (morning, evening, night)
  - Hot water control and boost
  - Safety alerts (high temp, low pressure, faults)
  - Weather-based compensation
  - Presence-based control (away/home modes)
  - Efficiency monitoring
  - **WiFi monitoring** ⭐ **NEW!** - Signal strength alerts, disconnection notifications, daily reports
  - **Time synchronization** - Daily sync, DST handling, manual button trigger
  - **Temperature bounds** - Setpoint validation against boiler limits

## Installation

### Method 1: Manual Dashboard Creation

1. In Home Assistant, go to **Settings** → **Dashboards**
2. Click **+ ADD DASHBOARD**
3. Choose **New dashboard from scratch**
4. Click the **⋮** menu → **Edit Dashboard** → **Raw configuration editor**
5. Copy the contents of one of the dashboard YAML files
6. Paste and save

### Method 2: Using Lovelace YAML Mode

1. Enable YAML mode in your `configuration.yaml`:
```yaml
lovelace:
  mode: yaml
  dashboards:
    lovelace-opentherm:
      mode: yaml
      title: OpenTherm
      icon: mdi:water-boiler
      show_in_sidebar: true
      filename: opentherm_dashboard.yaml
```

2. Copy one of the dashboard files to your HA config directory
3. Restart Home Assistant

### Method 3: Individual Cards

Copy individual card configurations from `entity_cards.yaml` and paste them into your existing dashboards:

1. Edit your dashboard
2. Click **+ ADD CARD**
3. Choose **Manual** at the bottom
4. Paste the YAML for the desired card

## Dashboard Examples

### Overview Dashboard
Shows all entities organized by category:
- System Status (fault, flame, modes)
- System Control (switches and setpoints)
- Temperatures (boiler, DHW, room, outside)
- Performance (modulation, pressure, flow)
- Statistics (counters and hours)
- Configuration (capabilities and version)

### Graphs Dashboard
Historical visualization:
- Boiler circuit temperatures with setpoint
- Hot water temperature tracking
- Room vs outside temperature
- Modulation level over time
- Apex Charts alternative (requires HACS)

### Control Dashboard
Quick access controls:
- Toggle buttons for CH and DHW
- Slider controls for all setpoints
- Status indicators (flame, CH, DHW)
- Current temperature readings

### Mobile Dashboard
Optimized for phones with tabs:
- **Overview** - Key temperatures and controls
- **Control** - All switches and setpoints
- **Temperatures** - All temp sensors and history
- **Performance** - Modulation, pressure, flow
- **Statistics** - Counters and operating hours

### WiFi Monitoring Dashboard ⭐ NEW!
Comprehensive WiFi and network monitoring:
- **Overview Tab**:
  - Real-time WiFi signal gauge (RSSI in dBm)
  - Connection status, IP address, SSID
  - Device uptime and memory usage
  - 24-hour signal strength history graph
  - Signal quality interpretation guide

- **Trends Tab**:
  - 7-day signal strength trends with hourly averages
  - Device uptime trends
  - 30-day statistics (mean, min, max)
  - ApexCharts integration (optional, requires HACS)

- **Diagnostics Tab**:
  - Detailed network information
  - Signal strength bar chart with color-coded quality
  - Connection event logbook (24h history)
  - Troubleshooting guide with solutions

- **Alerts Tab**:
  - Current alert status and thresholds
  - Active automation management
  - Alert configuration documentation
  - 7-day connection status history

**Signal Quality Reference:**
- -30 to -50 dBm: Excellent (maximum performance)
- -50 to -60 dBm: Very Good (reliable connection)
- -60 to -70 dBm: Good (stable for most uses)
- -70 to -80 dBm: Fair (may experience issues)
- -80 to -90 dBm: Poor (frequent disconnects)
- < -90 dBm: Very Poor (unreliable)

## Automation Examples

### Heating Schedules
```yaml
# Morning heating (weekdays 6am)
# Evening reduction (10pm)
# Night off (11:30pm)
```

### Hot Water Control
```yaml
# Boost when temperature drops below 45°C
# Economy mode at night (lower to 45°C)
```

### Safety Alerts
```yaml
# High temperature alert (>85°C)
# Low pressure alert (<0.8 bar)
# Fault detection with notifications
```

### Weather Compensation
```yaml
# Automatically adjust CH setpoint based on outside temperature
# Ranges from 50°C (>15°C outside) to 75°C (<-5°C outside)
```

### Presence Control
```yaml
# Away mode: Lower to 50°C and 16°C room temp
# Home mode: Restore to 65°C and 21°C room temp
```

### WiFi Monitoring ⭐ NEW!
```yaml
# Weak Signal Alert
# - Trigger: RSSI < -75 dBm for 5 minutes
# - Action: Persistent notification with recommendation

# Disconnection Alert
# - Trigger: Link status not 'connected' for 2 minutes
# - Action: Notification with last known IP
# - Auto-clear on reconnection

# Reconnection Notification
# - Trigger: Link status changes to 'connected'
# - Action: Success notification with new IP and signal strength
# - Auto-dismiss previous disconnection alert

# Daily WiFi Report
# - Schedule: Daily at 23:55
# - Content: Signal strength, status, IP, SSID, uptime
```

## Custom Cards Used

Some examples use custom cards from HACS. Install these for full functionality:

### Required for Some Examples
- **slider-entity-row** - Better sliders for setpoints
- **multiple-entity-row** - Compact entity display
- **bar-card** - Progress bars for statistics

### Optional but Recommended
- **mushroom-cards** - Modern card designs
- **apexcharts-card** - Advanced graphs
- **card-mod** - Card styling

Install from HACS: **Frontend** → Search for card name → Install

## Customization

### Changing Update Intervals

In the gateway configuration (`main.cpp`):
```cpp
.update_interval_ms = 10000  // 10 seconds (adjust as needed)
```

### Changing MQTT Topics

Update topics in both the gateway and dashboard entity IDs:
```cpp
.state_topic_base = "opentherm/state",    // Default
.command_topic_base = "opentherm/cmd",    // Default
```

### Changing Device Name

Update in gateway config and all entity IDs:
```cpp
.device_name = "My Boiler",  // Shows in HA
.device_id = "my_boiler",    // Used in entity IDs
```

## Tips

### Temperature Units
All temperatures are in Celsius. If you use Fahrenheit in HA, the entities will automatically convert for display.

### Gauge Ranges
Adjust gauge min/max values based on your boiler specifications:
```yaml
type: gauge
entity: sensor.opentherm_gw_boiler_temp
min: 0
max: 100  # Adjust to your boiler's max temp
```

### Notification Services
Replace `notify.notify` in automations with your preferred notification service:
- `notify.mobile_app_your_phone`
- `notify.telegram`
- `notify.pushbullet`
- etc.

### History Graph Duration
Adjust `hours_to_show` based on your needs:
```yaml
hours_to_show: 24  # 1 day
hours_to_show: 168  # 1 week
```

## Troubleshooting

### Entities Not Showing
1. Verify MQTT broker is connected
2. Check entity IDs match your device_id
3. Ensure gateway is publishing to MQTT

### Cards Not Rendering
1. Check YAML syntax with HA's built-in validator
2. Ensure custom cards are installed if used
3. Clear browser cache

### Automations Not Triggering
1. Check automation is enabled
2. Verify entity states are updating
3. Check automation traces in HA

## Additional Resources

- [Home Assistant Lovelace Docs](https://www.home-assistant.io/lovelace/)
- [MQTT Discovery](https://www.home-assistant.io/integrations/mqtt/#mqtt-discovery)
- [Home Assistant Community Forum](https://community.home-assistant.io/)

## Contributing

Have a cool dashboard or automation? Feel free to contribute examples!
