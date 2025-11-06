# Quick Start Guide - PicoOpenTherm Docker Setup

Get your complete OpenTherm Gateway testing environment running in 5 minutes!

## Prerequisites

- Docker and Docker Compose installed
- PicoOpenTherm device with firmware flashed
- Network connectivity between your computer and the Pico

## Step 1: Start the Services

```bash
cd docker
docker-compose up -d
```

This starts:
- **Mosquitto MQTT Broker** on port 1883
- **Home Assistant** on port 8123

## Step 2: Access Home Assistant

1. Open your browser to: **http://localhost:8123**
2. Wait for initial setup (may take 1-2 minutes on first run)
3. Complete the setup wizard:
   - Create your admin account
   - Set your home location (or skip)
   - Choose your preferences

## Step 3: Configure Your PicoOpenTherm Device

Your device needs to connect to the MQTT broker. Create a `secrets.cfg` file in your project root:

```ini
# WiFi Configuration
wifi.ssid=YourWiFiNetwork
wifi.password=YourWiFiPassword

# MQTT Configuration
mqtt.server_ip=192.168.1.100     # Your Docker host IP (NOT localhost!)
mqtt.server_port=1883
mqtt.client_id=picoopentherm

# Device Configuration
device.name=OpenTherm Gateway
device.id=opentherm_gw

# OpenTherm GPIO Pins
opentherm.tx_pin=16
opentherm.rx_pin=17
```

**Important**: Use your **Docker host's IP address**, not `localhost` or `127.0.0.1`!

Find your IP:
- **Linux/Mac**: `ip addr show` or `ifconfig`
- **Windows**: `ipconfig`
- **WSL**: Use your Windows IP, not WSL IP

## Step 4: Flash Your Device

```bash
# From project root
./provision-config.sh

# Flash firmware (assuming Pico is in BOOTSEL mode)
picotool load build/picoopentherm.uf2
```

Or use drag-and-drop if you prefer.

## Step 5: Watch Auto-Discovery

After your device boots and connects:

1. In Home Assistant, go to: **Settings → Devices & Services**
2. Click on **MQTT** integration
3. You should see **"OpenTherm Gateway"** appear automatically!
4. Click on it to see all discovered entities

## Step 6: Explore the Dashboards

Two dashboards are pre-configured:

1. **Main Dashboard** (http://localhost:8123)
   - Click "OpenTherm" tab at the top
   - View temperatures, status, and graphs

2. **OpenTherm Gateway Dashboard**
   - Click "OpenTherm Gateway" in the left sidebar
   - Detailed monitoring and configuration

## What You Get Out of the Box

### Instant Monitoring
- ✅ Room temperature and setpoint
- ✅ Boiler and return water temperatures
- ✅ Modulation level and flame status
- ✅ Water pressure
- ✅ 24-hour history graphs

### Smart Automations (Disabled by Default)
- ✅ Low pressure alerts
- ✅ High temperature warnings
- ✅ Offline notifications
- ✅ Energy-saving schedules
- ✅ Daily statistics

**Enable them**: Settings → Automations & Scenes → Toggle on desired automations

### Useful Scripts
- ✅ Quick temperature presets (comfort/eco)
- ✅ Heating boost with auto-reset
- ✅ System health check
- ✅ Maintenance reminders

**Run them**: Developer Tools → Services → Select script

## Common First-Time Issues

### Device Not Connecting

**Check MQTT broker:**
```bash
docker-compose logs mosquitto
```

**Test MQTT from device:**
- Verify WiFi credentials
- Ping the MQTT broker IP from another device
- Check firewall isn't blocking port 1883

### No Auto-Discovery

**Watch MQTT topics:**
```bash
docker exec -it picoopentherm-mosquitto mosquitto_sub -h localhost -t 'homeassistant/#' -v
```

You should see discovery messages when device connects.

**If nothing appears:**
- Device may not be connected to MQTT
- Check device serial console for errors
- Verify MQTT credentials (if auth enabled)

### Dashboard Doesn't Load

**Check configuration:**
```bash
docker-compose exec homeassistant hass --script check_config
```

**View logs:**
```bash
docker-compose logs homeassistant
```

## Next Steps

1. **Enable automations** you want (Settings → Automations & Scenes)
2. **Customize scripts** for your preferred temperatures
3. **Create schedules** for energy saving
4. **Add mobile notifications** (install HA mobile app)
5. **Explore integrations** (weather, calendars, etc.)

## Useful Commands

```bash
# View all logs
docker-compose logs -f

# Restart services
docker-compose restart

# Stop everything
docker-compose down

# Update to latest images
docker-compose pull
docker-compose up -d

# Shell access
docker exec -it picoopentherm-homeassistant bash
```

## Documentation

- **README.md** - Complete Docker documentation
- **FEATURES.md** - Full entity and feature reference
- **STRUCTURE.md** - File structure overview
- **[Main README](../README.md)** - PicoOpenTherm project docs
- **[PROVISIONING.md](../PROVISIONING.md)** - Device configuration guide

## Getting Help

If you encounter issues:

1. Check logs: `docker-compose logs -f`
2. Verify device console output (serial monitor)
3. Test MQTT manually (mosquitto_sub/mosquitto_pub)
4. Check Home Assistant → Settings → System → Logs
5. Open an issue on GitHub with logs and error messages

## Success Checklist

- [ ] Docker services running (`docker-compose ps` shows both services up)
- [ ] Home Assistant accessible at http://localhost:8123
- [ ] Device connected to WiFi (check device logs)
- [ ] Device connected to MQTT (check mosquitto logs)
- [ ] Entities discovered in HA (Settings → Devices & Services → MQTT)
- [ ] Dashboard showing real data
- [ ] Thermostat responding to temperature changes

---

**You're all set!** Your complete OpenTherm Gateway monitoring and control system is ready to use.
