Host-native Simulator & Docker Test — PicoOpenTherm

This document explains how to build and run the host-native simulator and the Docker-based test runner used for validating Home Assistant discovery and simulator state publishing.

Contents
- Prerequisites
- Build host-native simulator (native)
- Run the simulator against Mosquitto
- Run the Python validator (tools/subscribe_simulator.py)
- Docker-based test-runner (tools/docker-test)
- Troubleshooting

## Prerequisites

- `cmake`, `make` or `ninja`
- `pkg-config` and `libmosquitto-dev` (for native build)
- Docker & docker-compose (optional, for `tools/docker-test`)
- Python 3.8+ and `venv` (for the validator script)

On Debian/Ubuntu you can install the native deps with:

```bash
sudo apt update
sudo apt install -y cmake build-essential pkg-config libmosquitto-dev python3-venv python3-pip
```

## Build host-native simulator (native)

The host simulator is built separately from the Pico cross-build (the repository contains an ExternalProject-based target that isolates the native build). To build it manually:

```bash
# configure and build in an isolated directory
mkdir -p build-host && cd build-host
cmake -DBUILD_HOST_SIMULATOR=ON ..
cmake --build . --target host_simulator -j

# The native binary will be produced inside the external build directory, usually:
# build-host/host-external/host_simulator
```

If you prefer a single-step manual compile (no CMake), you can compile with system g++:

```bash
g++ -std=c++17 -Isrc -Isrc/generated src/host_simulator.cpp src/simulated_opentherm.cpp src/opentherm_protocol.cpp -lmosquitto -lpthread -o host_simulator
```

## Run the simulator against Mosquitto

Start a broker (local Docker example):

```bash
docker run -d --rm --name picoop-mosquitto -p 1883:1883 -v $(pwd)/docker/mosquitto/config:/mosquitto/config eclipse-mosquitto:2.0.18
```

Run the native simulator (defaults to device id `opentherm_gw`):

```bash
./build-host/host-external/host_simulator localhost 1883 test_gw &
# or if you built manually with g++:
./host_simulator localhost 1883 test_gw &
```

The simulator publishes Home Assistant discovery payloads (retained) and periodic `opentherm/state/<device>/...` topics. It also subscribes to `opentherm/opentherm_gw/cmd/<device>/#` to accept setpoint commands.

## Run the Python validator

The validator in `tools/subscribe_simulator.py` subscribes to `homeassistant/#` and `opentherm/state/#` and can verify discovery topics and publish a test command.

Create a venv and install the dependency:

```bash
python3 -m venv tools/venv
tools/venv/bin/pip install -r tools/requirements.txt
```

Run the validator and verify discovery + publish a test setpoint:

```bash
tools/venv/bin/python tools/subscribe_simulator.py --host localhost --port 1883 --verify --test --device test_gw
```

Expected outcome:
- The script prints retained Home Assistant discovery messages and periodic state messages.
- With `--verify` it will assert the expected discovery topics are present and exit non-zero if any are missing.
- With `--test` it publishes a `room_setpoint` command and waits for the state update.

## Docker-based test-runner (tools/docker-test)

`tools/docker-test` contains a Dockerfile and `docker-compose.yml` that build the host simulator inside a container, start a Mosquitto broker, and run the Python validator.

From the repository root run:

```bash
cd tools/docker-test
docker compose up --build --abort-on-container-exit --exit-code-from test-runner
```

Notes:
- The Dockerfile and compose file are configured to use the repository mosquitto config and pin Mosquitto to `2.0.18` for reproducible behavior.
- On some CI runners, building the image from the repository root is required so the Docker build context contains `src/` and `tools/`.

## Troubleshooting

- If discovery topics are missing: ensure the simulator is connected to the same broker (hostname/port) and that retained messages are enabled in the broker configuration (`persistence true`).
- If CMake tries to use the Pico cross-toolchain for the host target, prefer building the host simulator from the `host` ExternalProject target (the top-level CMake provides an ExternalProject that isolates native toolchain).
- If `python3 -m venv` fails, install the `python3-venv` package on Debian/Ubuntu.

## Summary

This repository provides both Pico firmware and a host-native simulator for easy local testing. The `tools/docker-test` target is intended for CI and local reproducible test runs. For quick local tests, building the native binary + running the Python validator is the fastest path.


### One-step run script

There's a convenience script at `scripts/run-host-test.sh` which builds the host-native simulator, starts a local Mosquitto container (using the repo config), runs the simulator, and invokes the Python validator. Usage:

```bash
scripts/run-host-test.sh
```

The script creates a `tools/venv` virtual environment for the validator and cleans up the simulator process and Docker container on exit.

If you'd like, I can also add a `Makefile` target that wraps this script.
