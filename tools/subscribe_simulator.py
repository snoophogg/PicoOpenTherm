#!/usr/bin/env python3
"""
subscribe_simulator.py

Simple MQTT validator for the PicoOpenTherm simulator:
- Subscribes to Home Assistant discovery topics and simulator state topics
- Prints messages with timestamps
- Optionally publishes a test setpoint command to exercise the simulator

Usage:
    python3 tools/subscribe_simulator.py --host <broker> [--port 1883] [--test]

Requires: paho-mqtt
"""

import argparse
import sys
import time
from datetime import datetime

import paho.mqtt.client as mqtt


def now():
    return datetime.now().isoformat(timespec='seconds')


def on_connect(client, userdata, flags, rc):
    print(f"[{now()}] Connected to broker, rc={rc}")
    # Subscribe to discovery and state topics
    client.subscribe('homeassistant/#')
    client.subscribe('opentherm/state/#')


def on_message(client, userdata, msg):
    print(f"[{now()}] {msg.topic} -> {msg.payload.decode(errors='replace')}")


def main():
    parser = argparse.ArgumentParser(description='Subscribe to simulator discovery/state topics')
    parser.add_argument('--host', default='localhost', help='MQTT broker host')
    parser.add_argument('--port', type=int, default=1883, help='MQTT broker port')
    parser.add_argument('--test', action='store_true', help='Publish a test room_setpoint command')
    parser.add_argument('--device', default='opentherm_gw', help='Device ID used by simulator (default: opentherm_gw)')
    args = parser.parse_args()

    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message

    try:
        client.connect(args.host, args.port, 60)
    except Exception as e:
        print(f"Failed to connect to {args.host}:{args.port} -> {e}")
        sys.exit(1)

    client.loop_start()

    # Wait for a bit to receive discovery messages
    try:
        print(f"[{now()}] Listening for discovery/state messages. Press Ctrl-C to quit.")
        time.sleep(5)

        if args.test:
            topic = f"opentherm/cmd/{args.device}/room_setpoint"
            payload = "21.5"
            print(f"[{now()}] Publishing test command {topic} -> {payload}")
            client.publish(topic, payload)

        # Keep running until interrupted
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print('\nInterrupted, exiting')
    finally:
        client.loop_stop()
        client.disconnect()


if __name__ == '__main__':
    main()
