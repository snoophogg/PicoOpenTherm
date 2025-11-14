#!/usr/bin/env bash
set -euo pipefail

# Wait for mosquitto to be available
host=mosquitto
port=1883
echo "Waiting for MQTT broker ${host}:${port}..."
for i in {1..30}; do
  if nc -z ${host} ${port}; then
    echo "Broker is up"
    break
  fi
  sleep 1
done

echo "Publishing discovery/state messages..."
python3 /app/publisher.py --host ${host} --port ${port}

echo "Running validator..."
python3 /app/subscribe_simulator.py --host ${host} --port ${port} --verify --test --timeout 15

EXIT_CODE=$?
echo "Validator exited with ${EXIT_CODE}"
exit ${EXIT_CODE}
