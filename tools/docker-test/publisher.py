#!/usr/bin/env python3
"""
publisher.py

Publisher used by the docker test-runner to emit discovery configs and state
messages so the validator can verify them.
"""
import time
import json
import sys
import argparse
import paho.mqtt.client as mqtt


def make_discovery_payload(device_id, component, object_id, state_topic, command_topic=None, device_name="OpenTherm Gateway"):
    payload = {
        "name": f"{device_name} {object_id}",
        "unique_id": f"{device_id}_{object_id}",
        "state_topic": state_topic,
        "device": {
            "identifiers": [device_id],
            "name": device_name,
            "model": "OpenTherm Gateway",
            "manufacturer": "PicoOpenTherm"
        }
    }
    if command_topic:
        payload["command_topic"] = command_topic
    return json.dumps(payload)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--host', default='mosquitto', help='MQTT broker host')
    parser.add_argument('--port', type=int, default=1883)
    parser.add_argument('--device', default='opentherm_gw')
    args = parser.parse_args()

    client = mqtt.Client()
    client.connect(args.host, args.port, 60)
    client.loop_start()

    device = args.device
    base_state = f"opentherm/state/{device}"
    base_cmd = f"opentherm/opentherm_gw/cmd/{device}"

    expected_map = {
        'binary_sensor': ['fault', 'ch_mode', 'dhw_mode', 'flame', 'cooling', 'diagnostic', 'dhw_present', 'cooling_supported', 'ch2_present'],
        'switch': ['ch_enable', 'dhw_enable'],
        'sensor': ['boiler_temp', 'dhw_temp', 'return_temp', 'outside_temp', 'room_temp', 'exhaust_temp', 'modulation', 'max_modulation', 'pressure', 'dhw_flow', 'burner_starts', 'ch_pump_starts', 'dhw_pump_starts', 'burner_hours', 'ch_pump_hours', 'dhw_pump_hours', 'fault_code', 'diagnostic_code', 'opentherm_version'],
        'number': ['control_setpoint', 'room_setpoint', 'dhw_setpoint', 'max_ch_setpoint', 'opentherm_tx_pin', 'opentherm_rx_pin'],
        'text': ['device_name', 'device_id']
    }

    # Publish discovery messages (retained)
    for comp, ids in expected_map.items():
        for oid in ids:
            state_topic = f"{base_state}/{oid}"
            cmd_topic = f"{base_cmd}/{oid}" if comp in ('switch', 'number', 'text') else None
            topic = f"homeassistant/{comp}/{device}/{oid}/config"
            payload = make_discovery_payload(device, comp, oid, state_topic, cmd_topic)
            print(f"Publishing discovery {topic} -> {payload}")
            client.publish(topic, payload, retain=True)

    # Publish initial state messages (retained)
    sample_values = {
        'boiler_temp': '55.2',
        'dhw_temp': '45.0',
        'return_temp': '40.0',
        'outside_temp': '7.5',
        'room_temp': '21.0',
        'modulation': '35',
        'pressure': '1.2',
        'flame': 'ON',
        'fault': 'OFF',
        'room_setpoint': '21.5'
    }

    # Small delay to ensure broker processed retained messages
    time.sleep(1)

    for comp, ids in expected_map.items():
        for oid in ids:
            topic = f"{base_state}/{oid}"
            value = sample_values.get(oid, '0')
            print(f"Publishing state {topic} -> {value}")
            client.publish(topic, value, retain=True)

    # Give broker a moment to process
    time.sleep(1)
    client.loop_stop()
    client.disconnect()


if __name__ == '__main__':
    main()
