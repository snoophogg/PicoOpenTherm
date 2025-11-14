# Tools: MQTT Simulator Validator

This small `tools` helper contains a simple MQTT validator script for the PicoOpenTherm simulator.

Files
- `subscribe_simulator.py` — subscribes to Home Assistant discovery topics (`homeassistant/#`) and simulator state topics (`opentherm/state/#`), prints messages with timestamps, and can publish a test `room_setpoint` command to exercise the simulator.
- `requirements.txt` — Python dependencies for the validator script (`paho-mqtt`).

Quick start

1. Install the Python dependency (recommended inside a venv):

```bash
python3 -m venv .venv && source .venv/bin/activate
python3 -m pip install -r tools/requirements.txt
```

2. Run the validator against your MQTT broker (default host `localhost:1883`):

```bash
python3 tools/subscribe_simulator.py --host mqtt.local --port 1883
```

3. Publish a test setpoint to exercise command handling (script will publish and keep listening):

```bash
python3 tools/subscribe_simulator.py --host mqtt.local --test
```

Options
- `--host` — MQTT broker hostname (default: `localhost`)
- `--port` — MQTT broker port (default: `1883`)
- `--test` — publish a test `room_setpoint` command to `opentherm/cmd/<device>/room_setpoint`
- `--device` — device id used in the command topic (default: `opentherm_gw`)

Notes
- The validator is intentionally small and interactive — it prints incoming discovery and state messages so you can inspect them while the simulator or device is running.
- For automated checks, the script can be extended to assert that a list of expected discovery topics arrives within a timeout (I can add that if you want).

Build and run simulator (local)
- To build the firmware and simulator (on the development machine):

```bash
./build.sh
```

- The simulator binary `picoopentherm_simulator.uf2` and `picoopentherm_simulator.dis` will be produced in `build/`.

Support
- If you want automated verification added to the tool (wait for a list of discovery topics, verify payload formats), tell me which topics/payloads to assert and I'll implement it.
