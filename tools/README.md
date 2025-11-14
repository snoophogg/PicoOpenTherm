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
 - `--verify` — automatically verify expected Home Assistant discovery topics are published (default: off)
 - `--timeout` — timeout in seconds for verification and test round-trip (default: 15)

Notes
- The validator is intentionally small and interactive — it prints incoming discovery and state messages so you can inspect them while the simulator or device is running.
- For automated checks, use `--verify` to have the script wait for the expected discovery topics and fail if some are missing within the timeout. Use `--test` together with `--verify` to also exercise a setpoint command and verify the simulator publishes the resulting state update.

Build and run simulator (local)
- To build the firmware and simulator (on the development machine):

```bash
./build.sh
```

- The simulator binary `picoopentherm_simulator.uf2` and `picoopentherm_simulator.dis` will be produced in `build/`.

Host-native simulator and Docker test

- A host-native simulator is available (`src/host_simulator.cpp`) which builds to a native Linux binary and publishes the same Home Assistant discovery and state topics via `libmosquitto`.
- To build the host simulator via CMake, enable the option `BUILD_HOST_SIMULATOR`:

```bash
mkdir -p build-host && cd build-host
cmake -DBUILD_HOST_SIMULATOR=ON ..
cmake --build . --target host_simulator -j
```

This target requires `libmosquitto-dev` and `pkg-config` to be installed on the host.

- A Docker-based test runner builds and runs the host simulator inside the container and runs the Python validator. To run locally:

```bash
cd tools/docker-test
docker compose up --build --abort-on-container-exit
```

The test-runner compiles `host_simulator`, publishes retained discovery/state messages, and runs the validator (`tools/subscribe_simulator.py --verify --test`).

- For a quick local single-command run, see `scripts/run-host-test.sh` at the repository root. It builds the host simulator, starts a Mosquitto container using the repo config, runs the simulator, and runs the validator.

Also see `docs/HOST_SIMULATOR.md` for full instructions and troubleshooting.

Support
- If you want automated verification added to the tool (wait for a list of discovery topics, verify payload formats), tell me which topics/payloads to assert and I'll implement it.
