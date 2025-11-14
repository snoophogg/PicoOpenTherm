#!/usr/bin/env bash
set -euo pipefail

# scripts/run-host-test.sh
# Build the host-native simulator, start a local Mosquitto broker (Docker),
# run the simulator, and execute the Python validator (tools/subscribe_simulator.py).

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="$ROOT_DIR/build-host"
MOSQ_CONTAINER_NAME="picoop-mosquitto"
MOSQ_IMAGE="eclipse-mosquitto:2.0.18"
DEVICE_ID="test_gw"

function cleanup() {
  echo "Cleaning up..."
  if [[ -n "${SIM_PID:-}" ]]; then
    kill "${SIM_PID}" 2>/dev/null || true
    wait "${SIM_PID}" 2>/dev/null || true
  fi
  docker rm -f "$MOSQ_CONTAINER_NAME" >/dev/null 2>&1 || true
}

trap cleanup EXIT INT TERM

echo "Building host-native simulator in $BUILD_DIR"
mkdir -p "$BUILD_DIR"
pushd "$BUILD_DIR" >/dev/null
cmake -DBUILD_HOST_SIMULATOR=ON ..
cmake --build . --target host_simulator -j
popd >/dev/null

MOSQ_CONFIG_DIR="$ROOT_DIR/docker/mosquitto/config"
echo "Starting Mosquitto broker (container: $MOSQ_CONTAINER_NAME)"
docker rm -f "$MOSQ_CONTAINER_NAME" >/dev/null 2>&1 || true
docker run -d --rm --name "$MOSQ_CONTAINER_NAME" -p 1883:1883 -v "$MOSQ_CONFIG_DIR":/mosquitto/config "$MOSQ_IMAGE"

SIM_BIN="$BUILD_DIR/host-external/host_simulator"
if [[ ! -x "$SIM_BIN" ]]; then
  echo "Simulator binary not found or not executable: $SIM_BIN"
  exit 1
fi

echo "Starting host simulator: $SIM_BIN localhost 1883 $DEVICE_ID"
"$SIM_BIN" localhost 1883 "$DEVICE_ID" &
SIM_PID=$!
sleep 1

echo "Preparing Python venv and running validator"
PY_VENV="$ROOT_DIR/tools/venv"
python3 -m venv "$PY_VENV"
"$PY_VENV/bin/pip" install -q -r "$ROOT_DIR/tools/requirements.txt"

"$PY_VENV/bin/python" "$ROOT_DIR/tools/subscribe_simulator.py" --host localhost --port 1883 --verify --test --device "$DEVICE_ID"

echo "Validator finished"

exit 0
