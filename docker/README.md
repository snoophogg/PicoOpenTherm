# Docker Development Environment

This directory contains a complete Docker Compose setup for testing PicoOpenTherm with Home Assistant and Mosquitto MQTT broker.

**📚 Documentation:**
- **[QUICKSTART.md](QUICKSTART.md)** - Get started in 5 minutes (recommended for first-time users)
- **[FEATURES.md](FEATURES.md)** - Complete reference of all pre-configured features
- **[STRUCTURE.md](STRUCTURE.md)** - File structure overview

## Quick Start

```bash
# Start all services
cd docker
docker-compose up -d

# View logs
docker-compose logs -f

# Stop all services
docker-compose down
```

After starting, access Home Assistant at: **http://localhost:8123**

The setup includes **pre-configured dashboards and automations** for PicoOpenTherm - see [Pre-configured UI](#pre-configured-ui) below.

📋 **For a complete list of all pre-configured features, see [FEATURES.md](FEATURES.md)**

## Services

### Mosquitto MQTT Broker
- **Port**: 1883 (MQTT)
- **Port**: 9001 (WebSocket)
- **Config**: `mosquitto/config/mosquitto.conf`
- **Data**: `mosquitto/data/`
- **Logs**: `mosquitto/log/`

### Home Assistant
- **Port**: 8123 (Web UI)
- **Config**: `homeassistant/configuration.yaml`
- **Data**: `homeassistant/` (persisted)

### MQTT Explorer (Debugging Tool)
- **Port**: 4000 (Web UI)
- **Access**: http://localhost:4000
- **Purpose**: View all MQTT messages in real-time
- **Connection**: Pre-configured to connect to mosquitto:1883

## Configuration

### MQTT Auto-Discovery

**Note**: Recent versions of Home Assistant require you to manually add the MQTT integration before auto-discovery works. See [First-Time Setup](#first-time-setup) for instructions.

The MQTT broker is configured for auto-discovery:

```yaml
mqtt:
  broker: mosquitto  # Container name on bridge network
  port: 1883
  discovery: true
  discovery_prefix: homeassistant
```

Once the MQTT integration is added in Home Assistant, all PicoOpenTherm entities will be automatically discovered and registered.

### First-Time Setup

1. **Start the containers:**
   ```bash
   cd docker
   docker-compose up -d
   ```

2. **Access Home Assistant:**
   - Open http://localhost:8123
   - Complete the initial setup wizard
   - Create your admin account

3. **Configure MQTT Integration:**
   
   Recent versions of Home Assistant require manual MQTT configuration:
   
   - Go to **Settings → Devices & Services**
   - Click **+ Add Integration**
   - Search for and select **MQTT**
   - Configure the connection:
     - **Broker**: `mosquitto` (the Mosquitto container name)
     - **Port**: `1883`
     - Leave **Username** and **Password** empty (unless you configured authentication)
   - Click **Submit**
   
   The MQTT integration will now be active and ready for auto-discovery.

4. **Configure your PicoOpenTherm device:**
   - Set MQTT broker IP to your host machine's IP address
   - Use port 1883
   - The device will auto-discover via MQTT once connected

5. **View discovered entities:**
   - Go to Settings → Devices & Services
   - Click on the "MQTT" integration
   - Your OpenTherm Gateway will appear automatically after the device connects

## Pre-configured UI

The Docker setup includes **pre-configured Lovelace dashboards and example automations** to get you started immediately.

### Dashboards

Two dashboards are pre-configured:

1. **Home Dashboard** (`ui-lovelace.yaml`)
   - Main overview with quick access to OpenTherm controls
   - Detailed OpenTherm view with all sensors and controls
   - Temperature and modulation history graphs
   - Network diagnostics and statistics

2. **OpenTherm Gateway Dashboard** (`lovelace/dashboards.yaml`)
   - Dedicated dashboard for detailed monitoring
   - Gateway status and configuration
   - Temperature charts (24-hour history)
   - Boiler activity graphs
   - Runtime configuration controls
   - Diagnostics and fault codes

To access the dashboards after starting Home Assistant:
- Main dashboard: http://localhost:8123
- OpenTherm dashboard: Click "OpenTherm Gateway" in the sidebar

### Example Automations

Eight pre-configured automations are included in `automations.yaml`:

1. **Low Water Pressure Alert** - Notifies when CH pressure drops below 1.0 bar
2. **High Boiler Temperature Alert** - Warns when boiler water exceeds 80°C
3. **Gateway Offline Alert** - Detects when the gateway goes offline for 5+ minutes
4. **Fault Code Notification** - Alerts when the boiler reports fault codes
5. **Night Setback** - Reduces temperature to 18°C at 10 PM (energy saving)
6. **Morning Warmup** - Raises temperature to 20°C at 6 AM
7. **Configuration Change Notification** - Tracks changes to gateway settings
8. **Daily Statistics Summary** - Provides end-of-day heating usage summary

All automations are **disabled by default**. Enable them via:
- Settings → Automations & Scenes → Enable desired automations

### Example Scripts

Seven reusable scripts are included in `scripts.yaml`:

1. **Set Comfort Temperature** - Quick preset for comfort mode (21°C default)
2. **Set Eco Temperature** - Energy-saving mode (16°C default)
3. **Boost Heating** - Temporary temperature boost with auto-reset
4. **Check System Health** - Verify all parameters are within normal ranges
5. **Schedule Maintenance Check** - Weekly maintenance reminder with checklist
6. **Reset Gateway Configuration** - Reset device to default settings

Scripts can be called from:
- Developer Tools → Services
- Automations (as actions)
- Dashboard cards (button/script entities)

Example: Call the boost script from Developer Tools → Services:
```yaml
service: script.boost_heating
data:
  temperature: 23
  duration: 30
```

### Customization

**Switch to UI-editable mode:**

Edit `configuration.yaml` and change:
```yaml
lovelace:
  mode: storage  # Change from 'yaml' to 'storage'
```

Then restart Home Assistant. You can now edit dashboards via the UI, and your YAML configurations serve as templates.

**Modify automations:**

All automations can be edited via:
- Settings → Automations & Scenes
- Click on an automation to edit trigger/condition/action
- Or edit `automations.yaml` directly (requires HA restart)

## Network Configuration

The Docker Compose file uses **bridge networking** with explicit port mappings:
- Mosquitto MQTT: 1883 (standard MQTT)
- Mosquitto WebSocket: 9001
- Home Assistant: 8123 (web UI)

Home Assistant connects to Mosquitto using the container name `mosquitto` within the Docker network.

**Note**: The previous setup used host networking, but bridge networking is now configured for better isolation and standard Docker practices.

If you need to switch back to host networking, modify `docker-compose.yml`:

```yaml
homeassistant:
  # Remove networks and ports sections
  # Add:
  network_mode: host
```

And update the MQTT broker to `localhost` in Home Assistant's MQTT integration configuration.

## Security

### Production Setup

For production use, enable authentication:

**1. Mosquitto Authentication:**

```bash
# Create password file
docker exec -it picoopentherm-mosquitto mosquitto_passwd -c /mosquitto/config/passwd admin

# Edit mosquitto.conf
allow_anonymous false
password_file /mosquitto/config/passwd
```

**2. Home Assistant MQTT:**

Configure via the MQTT integration in the UI:
- Go to Settings → Devices & Services → MQTT → Configure
- Update with your credentials:
  - Broker: `mosquitto`
  - Port: `1883`
  - Username: `admin`
  - Password: `your_secure_password`

**3. Home Assistant Access:**

Edit `homeassistant/configuration.yaml`:
```yaml
http:
  server_host: 0.0.0.0
  use_x_forwarded_for: true
  trusted_proxies:
    - 172.18.0.0/16
  # For external access, add SSL:
  # ssl_certificate: /ssl/fullchain.pem
  # ssl_key: /ssl/privkey.pem
```

## Volume Persistence

All data is persisted in local directories:

```
docker/
├── mosquitto/
│   ├── config/      # Mosquitto configuration
│   ├── data/        # MQTT message persistence
│   └── log/         # Mosquitto logs
└── homeassistant/   # All HA configuration and data
```

These directories will persist between container restarts and recreations.

## Useful Commands

### View Logs

```bash
# All services
docker-compose logs -f

# Specific service
docker-compose logs -f homeassistant
docker-compose logs -f mosquitto
```

### Restart Services

```bash
# Restart all
docker-compose restart

# Restart specific service
docker-compose restart homeassistant
docker-compose restart mosquitto
```

### Update Images

```bash
# Pull latest images
docker-compose pull

# Recreate containers with new images
docker-compose up -d
```

### Shell Access

```bash
# Home Assistant shell
docker exec -it picoopentherm-homeassistant bash

# Mosquitto shell
docker exec -it picoopentherm-mosquitto sh
```

### Test MQTT

```bash
# Subscribe to all topics
docker exec -it picoopentherm-mosquitto mosquitto_sub -h localhost -t '#' -v

# Subscribe to OpenTherm topics
docker exec -it picoopentherm-mosquitto mosquitto_sub -h localhost -t 'opentherm/#' -v

# Publish test message
docker exec -it picoopentherm-mosquitto mosquitto_pub -h localhost -t 'test/topic' -m 'Hello MQTT'
```

## Troubleshooting

### Home Assistant not connecting to MQTT

**Check Mosquitto is running:**
```bash
docker-compose ps
docker-compose logs mosquitto
```

**Test MQTT connection:**
```bash
docker exec -it picoopentherm-mosquitto mosquitto_sub -h localhost -t 'test' -v
```

**Verify configuration:**
```bash
cat homeassistant/configuration.yaml | grep -A 10 mqtt
```

### PicoOpenTherm not discovering

**Check MQTT topics:**
```bash
docker exec -it picoopentherm-mosquitto mosquitto_sub -h localhost -t 'homeassistant/#' -v
```

**Verify device MQTT settings:**
- Broker IP should be your host machine's IP (not localhost or 127.0.0.1)
- Port should be 1883
- Check device console logs for connection errors

### Permission Issues

If you encounter permission issues with volumes:

```bash
# Fix ownership (Linux/macOS)
sudo chown -R $USER:$USER mosquitto homeassistant

# On Windows/WSL, permissions should work automatically
```

### Port Conflicts

If ports 1883 or 8123 are already in use:

```bash
# Check what's using the port
sudo lsof -i :1883
sudo lsof -i :8123

# Modify docker-compose.yml to use different ports
ports:
  - "1884:1883"  # Change external port
  - "8124:8123"
```

## Development Workflow

### Testing Configuration Changes

1. **Modify PicoOpenTherm code**
2. **Build and flash firmware**
3. **Watch MQTT discovery in Home Assistant:**
   ```bash
   docker exec -it picoopentherm-mosquitto mosquitto_sub -h localhost -t 'homeassistant/+/opentherm_gw/#' -v
   ```
4. **Check entities in HA UI** (Settings → Devices & Services → MQTT)

### Debugging MQTT Messages

Monitor all OpenTherm MQTT traffic:
```bash
# State updates
docker exec -it picoopentherm-mosquitto mosquitto_sub -h localhost -t 'opentherm/state/#' -v

# Commands
docker exec -it picoopentherm-mosquitto mosquitto_sub -h localhost -t 'opentherm/cmd/#' -v

# Discovery
docker exec -it picoopentherm-mosquitto mosquitto_sub -h localhost -t 'homeassistant/#' -v
```

## Clean Slate

To completely reset the environment:

```bash
# Stop and remove containers
docker-compose down

# Remove all data (WARNING: destroys all configuration!)
rm -rf mosquitto/data/* mosquitto/log/* homeassistant/*

# Keep config files
cp homeassistant/configuration.yaml homeassistant/configuration.yaml.backup
cp mosquitto/config/mosquitto.conf mosquitto/config/mosquitto.conf.backup

# Start fresh
docker-compose up -d
```

## Integration with PicoOpenTherm

### Configure Device

In your `secrets.cfg`:
```ini
mqtt.server_ip=192.168.1.100  # Your Docker host IP
mqtt.server_port=1883
mqtt.client_id=picoopentherm
```

### Provision and Flash

```bash
# From project root
./provision-config.sh
picotool load build/picoopentherm.uf2
```

### Monitor Connection

```bash
# Watch for device connection
docker-compose logs -f mosquitto | grep opentherm
```

## References

- [Home Assistant Docker Installation](https://www.home-assistant.io/installation/linux#docker-compose)
- [Eclipse Mosquitto Docker](https://hub.docker.com/_/eclipse-mosquitto)
- [Home Assistant MQTT Integration](https://www.home-assistant.io/integrations/mqtt/)
