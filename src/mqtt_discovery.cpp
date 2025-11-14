#include "mqtt_discovery.hpp"
#include "mqtt_common.hpp"
#include "opentherm_ha.hpp"
#include "mqtt_topics.hpp"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "mqtt_publish.hpp"
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <unordered_map>

namespace OpenTherm
{
    namespace Discovery
    {
        bool publishWithRetry(const char *topic, const char *config, int max_retries)
        {
            for (int attempt = 0; attempt < max_retries; attempt++)
            {
                // Publish with retain flag so configs survive HA restarts
                if (OpenTherm::Common::mqtt_publish_wrapper(topic, config, true))
                {
                    // Success - give buffers time to clear before next publish
                    // Poll network aggressively to process ACKs (500ms)
                    for (int i = 0; i < 50; i++)
                    {
                        cyw43_arch_poll();
                        sleep_ms(10);
                    }
                    return true;
                }

                // Exponential backoff with active network polling
                // This allows lwIP to process ACKs and free buffers
                uint32_t delay_ms = 500 * (1 << attempt);
                printf("  Retry %d/%d in %lu ms (polling network)...\n", attempt + 1, max_retries, delay_ms);

                // Poll network actively during delay to process packets and free buffers
                uint32_t iterations = delay_ms / 10;
                for (uint32_t i = 0; i < iterations; i++)
                {
                    cyw43_arch_poll();
                    sleep_ms(10);
                }
            }

            printf("  ERROR: Failed to publish after %d attempts\n", max_retries);
            return false;
        }

        // Simulator-specific helper removed; merged into publishDiscoveryConfigs

        std::string buildStateTopic(const OpenTherm::HomeAssistant::Config &cfg, const char *suffix)
        {
            return std::string(cfg.state_topic_base) + "/" + suffix;
        }

        std::string buildCommandTopic(const OpenTherm::HomeAssistant::Config &cfg, const char *suffix)
        {
            return std::string(cfg.command_topic_base) + "/" + suffix;
        }

        std::string buildDiscoveryTopic(const OpenTherm::HomeAssistant::Config &cfg, const char *component, const char *object_id)
        {
            return std::string(cfg.mqtt_prefix) + "/" + component + "/" + cfg.device_id + "/" + object_id + "/config";
        }

        bool publishDiscoveryConfig(const OpenTherm::HomeAssistant::Config &cfg,
                                    const char *component, const char *object_id,
                                    const char *name, const char *state_topic,
                                    const char *device_class, const char *unit,
                                    const char *icon, const char *command_topic,
                                    const char *value_template,
                                    float min_value, float max_value, float step)
        {
            std::ostringstream payload;
            payload << "{";
            payload << "\"name\":\"" << name << "\",";
            payload << "\"unique_id\":\"" << cfg.device_id << "_" << object_id << "\",";
            payload << "\"state_topic\":\"" << state_topic << "\",";

            if (command_topic)
            {
                payload << "\"command_topic\":\"" << command_topic << "\",";
            }

            if (device_class)
            {
                payload << "\"device_class\":\"" << device_class << "\",";
            }

            if (unit)
            {
                payload << "\"unit_of_measurement\":\"" << unit << "\",";
            }

            if (icon)
            {
                payload << "\"icon\":\"" << icon << "\",";
            }

            if (value_template)
            {
                payload << "\"value_template\":\"" << value_template << "\",";
            }

            if (strcmp(component, "number") == 0)
            {
                payload << "\"min\":" << min_value << ",";
                payload << "\"max\":" << max_value << ",";
                payload << "\"step\":" << step << ",";
                payload << "\"mode\":\"box\",";
            }

            payload << "\"device\":{";
            payload << "\"identifiers\":[\"" << cfg.device_id << "\"],";
            payload << "\"name\":\"" << cfg.device_name << "\",";
            payload << "\"model\":\"OpenTherm Gateway\",";
            payload << "\"manufacturer\":\"PicoOpenTherm\"";
            payload << "}";

            payload << "}";

            std::string topic = buildDiscoveryTopic(cfg, component, object_id);
            if (!publishWithRetry(topic.c_str(), payload.str().c_str()))
            {
                printf("WARNING: Failed to publish discovery config for %s/%s\n", component, object_id);
                return false;
            }
            return true;
        }

        bool publishDiscoveryConfigs(const OpenTherm::HomeAssistant::Config &cfg)
        {
            // Wait for MQTT client to be ready for large discovery messages
            printf("Waiting for MQTT client to be ready for discovery (5 seconds, polling network)...\n");
            for (int i = 0; i < 500; i++)
            {
                cyw43_arch_poll();
                sleep_ms(10);
            }

            printf("Publishing Home Assistant MQTT discovery configs...\n");

            using namespace OpenTherm::MQTTTopics;
            // Binary Sensors
            publishDiscoveryConfig(cfg, "binary_sensor", FAULT, "Fault",
                                   buildStateTopic(cfg, FAULT).c_str(), "problem", nullptr, "mdi:alert-circle");
            publishDiscoveryConfig(cfg, "binary_sensor", CH_MODE, "Central Heating Mode",
                                   buildStateTopic(cfg, CH_MODE).c_str(), "heat", nullptr, "mdi:radiator");
            publishDiscoveryConfig(cfg, "binary_sensor", DHW_MODE, "Hot Water Mode",
                                   buildStateTopic(cfg, DHW_MODE).c_str(), "heat", nullptr, "mdi:water-boiler");
            publishDiscoveryConfig(cfg, "binary_sensor", FLAME, "Flame Status",
                                   buildStateTopic(cfg, FLAME).c_str(), "heat", nullptr, "mdi:fire");
            publishDiscoveryConfig(cfg, "binary_sensor", COOLING, "Cooling Active",
                                   buildStateTopic(cfg, COOLING).c_str(), "cold", nullptr, "mdi:snowflake");
            publishDiscoveryConfig(cfg, "binary_sensor", DIAGNOSTIC, "Diagnostic Mode",
                                   buildStateTopic(cfg, DIAGNOSTIC).c_str(), nullptr, nullptr, "mdi:wrench");

            // Switches
            publishDiscoveryConfig(cfg, "switch", CH_ENABLE, "Central Heating Enable",
                                   buildStateTopic(cfg, CH_ENABLE).c_str(), "switch", nullptr, "mdi:radiator",
                                   buildCommandTopic(cfg, CH_ENABLE).c_str());
            publishDiscoveryConfig(cfg, "switch", DHW_ENABLE, "Hot Water Enable",
                                   buildStateTopic(cfg, DHW_ENABLE).c_str(), "switch", nullptr, "mdi:water-boiler",
                                   buildCommandTopic(cfg, DHW_ENABLE).c_str());

            // Temperature sensors
            publishDiscoveryConfig(cfg, "sensor", BOILER_TEMP, "Boiler Temperature",
                                   buildStateTopic(cfg, BOILER_TEMP).c_str(), "temperature", "°C", "mdi:thermometer");
            publishDiscoveryConfig(cfg, "sensor", DHW_TEMP, "Hot Water Temperature",
                                   buildStateTopic(cfg, DHW_TEMP).c_str(), "temperature", "°C", "mdi:thermometer");
            publishDiscoveryConfig(cfg, "sensor", RETURN_TEMP, "Return Water Temperature",
                                   buildStateTopic(cfg, RETURN_TEMP).c_str(), "temperature", "°C", "mdi:thermometer");
            publishDiscoveryConfig(cfg, "sensor", OUTSIDE_TEMP, "Outside Temperature",
                                   buildStateTopic(cfg, OUTSIDE_TEMP).c_str(), "temperature", "°C", "mdi:thermometer");
            publishDiscoveryConfig(cfg, "sensor", ROOM_TEMP, "Room Temperature",
                                   buildStateTopic(cfg, ROOM_TEMP).c_str(), "temperature", "°C", "mdi:home-thermometer");
            publishDiscoveryConfig(cfg, "sensor", EXHAUST_TEMP, "Exhaust Temperature",
                                   buildStateTopic(cfg, EXHAUST_TEMP).c_str(), "temperature", "°C", "mdi:thermometer");

            // Numbers (setpoints)
            publishDiscoveryConfig(cfg, "number", CONTROL_SETPOINT, "Control Setpoint",
                                   buildStateTopic(cfg, CONTROL_SETPOINT).c_str(), nullptr, "°C", "mdi:thermometer-lines",
                                   buildCommandTopic(cfg, CONTROL_SETPOINT).c_str(), nullptr, 0.0f, 100.0f, 0.5f);
            publishDiscoveryConfig(cfg, "number", ROOM_SETPOINT, "Room Setpoint",
                                   buildStateTopic(cfg, ROOM_SETPOINT).c_str(), nullptr, "°C", "mdi:home-thermometer-outline",
                                   buildCommandTopic(cfg, ROOM_SETPOINT).c_str(), nullptr, 5.0f, 30.0f, 0.5f);
            publishDiscoveryConfig(cfg, "number", DHW_SETPOINT, "Hot Water Setpoint",
                                   buildStateTopic(cfg, DHW_SETPOINT).c_str(), nullptr, "°C", "mdi:water-thermometer-outline",
                                   buildCommandTopic(cfg, DHW_SETPOINT).c_str(), nullptr, 30.0f, 90.0f, 1.0f);
            publishDiscoveryConfig(cfg, "number", MAX_CH_SETPOINT, "Max CH Setpoint",
                                   buildStateTopic(cfg, MAX_CH_SETPOINT).c_str(), nullptr, "°C", "mdi:thermometer-high",
                                   buildCommandTopic(cfg, MAX_CH_SETPOINT).c_str(), nullptr, 30.0f, 90.0f, 1.0f);

            // Modulation sensors
            publishDiscoveryConfig(cfg, "sensor", MODULATION, "Modulation Level",
                                   buildStateTopic(cfg, MODULATION).c_str(), nullptr, "%", "mdi:percent");
            publishDiscoveryConfig(cfg, "sensor", MAX_MODULATION, "Max Modulation Level",
                                   buildStateTopic(cfg, MAX_MODULATION).c_str(), nullptr, "%", "mdi:percent");

            // Pressure & flow
            publishDiscoveryConfig(cfg, "sensor", PRESSURE, "CH Water Pressure",
                                   buildStateTopic(cfg, PRESSURE).c_str(), "pressure", "bar", "mdi:gauge");
            publishDiscoveryConfig(cfg, "sensor", DHW_FLOW, "Hot Water Flow Rate",
                                   buildStateTopic(cfg, DHW_FLOW).c_str(), nullptr, "l/min", "mdi:water-pump");

            // Counters
            publishDiscoveryConfig(cfg, "sensor", BURNER_STARTS, "Burner Starts",
                                   buildStateTopic(cfg, BURNER_STARTS).c_str(), nullptr, "starts", "mdi:counter");
            publishDiscoveryConfig(cfg, "sensor", CH_PUMP_STARTS, "CH Pump Starts",
                                   buildStateTopic(cfg, CH_PUMP_STARTS).c_str(), nullptr, "starts", "mdi:counter");
            publishDiscoveryConfig(cfg, "sensor", DHW_PUMP_STARTS, "DHW Pump Starts",
                                   buildStateTopic(cfg, DHW_PUMP_STARTS).c_str(), nullptr, "starts", "mdi:counter");
            publishDiscoveryConfig(cfg, "sensor", BURNER_HOURS, "Burner Operating Hours",
                                   buildStateTopic(cfg, BURNER_HOURS).c_str(), "duration", "h", "mdi:clock-outline");
            publishDiscoveryConfig(cfg, "sensor", CH_PUMP_HOURS, "CH Pump Operating Hours",
                                   buildStateTopic(cfg, CH_PUMP_HOURS).c_str(), "duration", "h", "mdi:clock-outline");
            publishDiscoveryConfig(cfg, "sensor", DHW_PUMP_HOURS, "DHW Pump Operating Hours",
                                   buildStateTopic(cfg, DHW_PUMP_HOURS).c_str(), "duration", "h", "mdi:clock-outline");

            // Faults and config
            publishDiscoveryConfig(cfg, "sensor", FAULT_CODE, "Fault Code",
                                   buildStateTopic(cfg, FAULT_CODE).c_str(), nullptr, nullptr, "mdi:alert-octagon");

            // Diagnostic code
            publishDiscoveryConfig(cfg, "sensor", DIAGNOSTIC_CODE, "Diagnostic Code",
                                   buildStateTopic(cfg, DIAGNOSTIC_CODE).c_str(), nullptr, nullptr, "mdi:alert-circle");

            publishDiscoveryConfig(cfg, "binary_sensor", DHW_PRESENT, "DHW Present",
                                   buildStateTopic(cfg, DHW_PRESENT).c_str(), nullptr, nullptr, "mdi:water-boiler");
            publishDiscoveryConfig(cfg, "binary_sensor", COOLING_SUPPORTED, "Cooling Supported",
                                   buildStateTopic(cfg, COOLING_SUPPORTED).c_str(), nullptr, nullptr, "mdi:snowflake");
            publishDiscoveryConfig(cfg, "binary_sensor", CH2_PRESENT, "CH2 Present",
                                   buildStateTopic(cfg, CH2_PRESENT).c_str(), nullptr, nullptr, "mdi:radiator");

            publishDiscoveryConfig(cfg, "sensor", OPENTHERM_VERSION, "OpenTherm Version",
                                   buildStateTopic(cfg, OPENTHERM_VERSION).c_str(), nullptr, nullptr, "mdi:information");

            // Text / device config
            publishDiscoveryConfig(cfg, "text", DEVICE_NAME, "Device Name",
                                   buildStateTopic(cfg, DEVICE_NAME).c_str(), nullptr, nullptr, "mdi:tag-text",
                                   buildCommandTopic(cfg, DEVICE_NAME).c_str());
            publishDiscoveryConfig(cfg, "text", DEVICE_ID, "Device ID",
                                   buildStateTopic(cfg, DEVICE_ID).c_str(), nullptr, nullptr, "mdi:identifier",
                                   buildCommandTopic(cfg, DEVICE_ID).c_str());

            // GPIO numbers
            publishDiscoveryConfig(cfg, "number", OPENTHERM_TX_PIN, "OpenTherm TX Pin",
                                   buildStateTopic(cfg, OPENTHERM_TX_PIN).c_str(), nullptr, nullptr, "mdi:pin",
                                   buildCommandTopic(cfg, OPENTHERM_TX_PIN).c_str(), nullptr, 0.0f, 28.0f, 1.0f);
            publishDiscoveryConfig(cfg, "number", OPENTHERM_RX_PIN, "OpenTherm RX Pin",
                                   buildStateTopic(cfg, OPENTHERM_RX_PIN).c_str(), nullptr, nullptr, "mdi:pin",
                                   buildCommandTopic(cfg, OPENTHERM_RX_PIN).c_str(), nullptr, 0.0f, 28.0f, 1.0f);

            printf("Discovery configs published!\n");
            return true;
        }

        void publishSensor(const OpenTherm::HomeAssistant::Config &cfg, const char *topic_suffix, float value)
        {
            std::string topic = buildStateTopic(cfg, topic_suffix);
            OpenTherm::Publish::publishFloatIfChanged(topic, value, 2, false);
        }

        void publishSensor(const OpenTherm::HomeAssistant::Config &cfg, const char *topic_suffix, int value)
        {
            std::string topic = buildStateTopic(cfg, topic_suffix);
            OpenTherm::Publish::publishIntIfChanged(topic, value, false);
        }

        void publishSensor(const OpenTherm::HomeAssistant::Config &cfg, const char *topic_suffix, const char *value)
        {
            std::string topic = buildStateTopic(cfg, topic_suffix);
            OpenTherm::Publish::publishStringIfChanged(topic, value ? std::string(value) : std::string(), false);
        }

        void publishBinarySensor(const OpenTherm::HomeAssistant::Config &cfg, const char *topic_suffix, bool value)
        {
            std::string topic = buildStateTopic(cfg, topic_suffix);
            OpenTherm::Publish::publishBinaryIfChanged(topic, value, false);
        }

    } // namespace Discovery
} // namespace OpenTherm
