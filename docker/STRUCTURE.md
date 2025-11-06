# Docker Setup Structure

```
docker/
├── README.md                          # Main Docker documentation
├── FEATURES.md                        # Complete feature reference
├── docker-compose.yml                 # Service definitions
│
├── mosquitto/                         # MQTT Broker
│   ├── config/
│   │   └── mosquitto.conf            # Broker configuration
│   ├── data/                         # (Runtime - gitignored)
│   └── log/                          # (Runtime - gitignored)
│
└── homeassistant/                    # Home Assistant
    ├── configuration.yaml            # Main HA configuration
    ├── secrets.yaml                  # Runtime secrets (gitignored)
    ├── secrets.yaml.example          # Secrets template
    │
    ├── ui-lovelace.yaml             # Main dashboard config
    ├── lovelace/
    │   └── dashboards.yaml          # OpenTherm dashboard
    │
    ├── automations.yaml             # 8 pre-configured automations
    ├── scripts.yaml                 # 6 reusable scripts
    └── scenes.yaml                  # Empty (UI-editable)
```

## Pre-configured Content

### Dashboards (2)
- **Main Dashboard**: Home view + detailed OpenTherm view
- **OpenTherm Gateway Dashboard**: Dedicated monitoring dashboard

### Automations (8)
1. Low Water Pressure Alert
2. High Boiler Temperature Alert
3. Gateway Offline Alert
4. Fault Code Notification
5. Night Setback (energy saving)
6. Morning Warmup
7. Configuration Change Notification
8. Daily Statistics Summary

### Scripts (6)
1. Set Comfort Temperature
2. Set Eco Temperature
3. Boost Heating
4. Check System Health
5. Schedule Maintenance Check
6. Reset Gateway Configuration

### Entities Monitored
- **Climate**: 1 thermostat
- **Sensors**: 20+ (temperatures, status, diagnostics)
- **Binary Sensors**: 3 (CH, DHW, flame)
- **Configuration**: 4 writable entities
- **Buttons**: 1 restart button

## Quick Access

| Feature | Location | Access |
|---------|----------|--------|
| Main Dashboard | http://localhost:8123 | Default view |
| OpenTherm Dashboard | Sidebar → "OpenTherm Gateway" | One click |
| Enable Automations | Settings → Automations & Scenes | Enable individually |
| Run Scripts | Developer Tools → Services | Call by name |
| Edit UI | Settings → Dashboards | Requires storage mode |
| MQTT Explorer | http://localhost:9001 | WebSocket |

## Getting Started

1. **Start services**: `docker-compose up -d`
2. **Access HA**: http://localhost:8123
3. **Complete setup wizard** (first time only)
4. **Connect PicoOpenTherm** to MQTT broker
5. **Auto-discovery** populates all entities
6. **Enable desired automations**
7. **Explore dashboards** and try scripts

See README.md for detailed documentation.
See FEATURES.md for complete entity reference.
