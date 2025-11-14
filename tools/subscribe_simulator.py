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
    # Track discovery/state messages for verification
    if userdata is None:
        return
    if msg.topic.startswith('homeassistant/'):
        parts = msg.topic.split('/')
        if len(parts) >= 5:
            comp = parts[1]
            device = parts[2]
            object_id = parts[3]
            userdata.setdefault('discovered', set()).add((comp, device, object_id))
    elif msg.topic.startswith('opentherm/state/'):
        parts = msg.topic.split('/')
        if len(parts) >= 4:
            device = parts[2]
            suffix = parts[3]
            userdata.setdefault('state_received', set()).add((device, suffix, msg.payload.decode(errors='replace')))


def main():
    parser = argparse.ArgumentParser(description='Subscribe to simulator discovery/state topics')
    parser.add_argument('--host', default='localhost', help='MQTT broker host')
    parser.add_argument('--port', type=int, default=1883, help='MQTT broker port')
    parser.add_argument('--test', action='store_true', help='Publish a test room_setpoint command')
    parser.add_argument('--verify', action='store_true', help='Automatically verify discovery topics are published')
    parser.add_argument('--timeout', type=int, default=15, help='Timeout seconds for verification (default: 15)')
    parser.add_argument('--device', default='opentherm_gw', help='Device ID used by simulator (default: opentherm_gw)')
    args = parser.parse_args()

    client = mqtt.Client()
    client.on_connect = on_connect
    client.on_message = on_message
    # userdata for verification
    client.user_data_set({'discovered': set(), 'state_received': set()})

    try:
        client.connect(args.host, args.port, 60)
    except Exception as e:
        print(f"Failed to connect to {args.host}:{args.port} -> {e}")
        sys.exit(1)

    client.loop_start()

    # Wait for a bit to receive discovery messages
    try:
        print(f"[{now()}] Listening for discovery/state messages. Press Ctrl-C to quit.")

        if args.verify:
            # Build expected discovery tuples based on firmware lists
            device = args.device
            expected = []
            expected_map = {
                'binary_sensor': ['fault', 'ch_mode', 'dhw_mode', 'flame', 'cooling', 'diagnostic', 'dhw_present', 'cooling_supported', 'ch2_present'],
                'switch': ['ch_enable', 'dhw_enable'],
                'sensor': ['boiler_temp', 'dhw_temp', 'return_temp', 'outside_temp', 'room_temp', 'exhaust_temp', 'modulation', 'max_modulation', 'pressure', 'dhw_flow', 'burner_starts', 'ch_pump_starts', 'dhw_pump_starts', 'burner_hours', 'ch_pump_hours', 'dhw_pump_hours', 'fault_code', 'diagnostic_code', 'opentherm_version'],
                'number': ['control_setpoint', 'room_setpoint', 'dhw_setpoint', 'max_ch_setpoint', 'opentherm_tx_pin', 'opentherm_rx_pin'],
                'text': ['device_name', 'device_id']
            }
            for comp, ids in expected_map.items():
                for oid in ids:
                    expected.append((comp, device, oid))

            deadline = time.time() + args.timeout
            print(f"[{now()}] Verifying {len(expected)} discovery topics within {args.timeout}s...")
            while time.time() < deadline:
                received = client._userdata_get() if hasattr(client, '_userdata_get') else client._userdata if hasattr(client, '_userdata') else client._user_data
                # paho stores userdata internally; retrieve via _userdata if necessary
                received_set = None
                try:
                    received_set = client._userdata['discovered']
                except Exception:
                    try:
                        received_set = client._userdata_get()['discovered']
                    except Exception:
                        received_set = client._userdata['discovered'] if hasattr(client, 'userdata') else set()

                missing = [e for e in expected if e not in received_set]
                if not missing:
                    print(f"[{now()}] All discovery topics received ({len(expected)})")
                    break
                time.sleep(0.5)

            # Final check
            try:
                discovered = client._userdata['discovered']
            except Exception:
                discovered = set()
            missing = [e for e in expected if e not in discovered]
            if missing:
                print(f"[{now()}] Verification FAILED - missing {len(missing)} discovery topics:")
                for comp, dev, oid in missing:
                    print(f"  homeassistant/{comp}/{dev}/{oid}/config")
                client.loop_stop()
                client.disconnect()
                sys.exit(2)
            else:
                print(f"[{now()}] Discovery verification PASSED")

        # Small delay to collect messages
        time.sleep(1)

        if args.test:
            topic = f"opentherm/cmd/{args.device}/room_setpoint"
            payload = "21.5"
            print(f"[{now()}] Publishing test command {topic} -> {payload}")
            client.publish(topic, payload)

            # Wait for state update on room_setpoint
            expected_state_topic = f"opentherm/state/{args.device}/room_setpoint"
            deadline = time.time() + args.timeout
            seen = False
            while time.time() < deadline:
                try:
                    states = client._userdata['state_received']
                except Exception:
                    states = set()
                for dev, suffix, payload in states:
                    t = f"opentherm/state/{dev}/{suffix}"
                    if t == expected_state_topic:
                        print(f"[{now()}] Received state {t} -> {payload}")
                        seen = True
                        break
                if seen:
                    break
                time.sleep(0.5)
            if not seen:
                print(f"[{now()}] Test command verification FAILED - did not observe {expected_state_topic}")
                client.loop_stop()
                client.disconnect()
                sys.exit(3)

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
