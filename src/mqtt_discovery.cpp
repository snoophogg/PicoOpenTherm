#include "mqtt_discovery.hpp"
#include "mqtt_common.hpp"
#include "opentherm_ha.hpp"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>

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

            // Binary Sensors
            publishDiscoveryConfig(cfg, "binary_sensor", "fault", "Fault",
                                   buildStateTopic(cfg, "fault").c_str(), "problem", nullptr, "mdi:alert-circle");
            publishDiscoveryConfig(cfg, "binary_sensor", "ch_mode", "Central Heating Mode",
                                   buildStateTopic(cfg, "ch_mode").c_str(), "heat", nullptr, "mdi:radiator");
            publishDiscoveryConfig(cfg, "binary_sensor", "dhw_mode", "Hot Water Mode",
                                   buildStateTopic(cfg, "dhw_mode").c_str(), "heat", nullptr, "mdi:water-boiler");
            publishDiscoveryConfig(cfg, "binary_sensor", "flame", "Flame Status",
                                   buildStateTopic(cfg, "flame").c_str(), "heat", nullptr, "mdi:fire");
            publishDiscoveryConfig(cfg, "binary_sensor", "cooling", "Cooling Active",
                                   buildStateTopic(cfg, "cooling").c_str(), "cold", nullptr, "mdi:snowflake");
            publishDiscoveryConfig(cfg, "binary_sensor", "diagnostic", "Diagnostic Mode",
                                   buildStateTopic(cfg, "diagnostic").c_str(), nullptr, nullptr, "mdi:wrench");

            // Switches
            publishDiscoveryConfig(cfg, "switch", "ch_enable", "Central Heating Enable",
                                   buildStateTopic(cfg, "ch_enable").c_str(), "switch", nullptr, "mdi:radiator",
                                   buildCommandTopic(cfg, "ch_enable").c_str());
            publishDiscoveryConfig(cfg, "switch", "dhw_enable", "Hot Water Enable",
                                   buildStateTopic(cfg, "dhw_enable").c_str(), "switch", nullptr, "mdi:water-boiler",
                                   buildCommandTopic(cfg, "dhw_enable").c_str());

            // Temperature sensors
            publishDiscoveryConfig(cfg, "sensor", "boiler_temp", "Boiler Temperature",
                                   buildStateTopic(cfg, "boiler_temp").c_str(), "temperature", "°C", "mdi:thermometer");
            publishDiscoveryConfig(cfg, "sensor", "dhw_temp", "Hot Water Temperature",
                                   buildStateTopic(cfg, "dhw_temp").c_str(), "temperature", "°C", "mdi:thermometer");
            publishDiscoveryConfig(cfg, "sensor", "return_temp", "Return Water Temperature",
                                   buildStateTopic(cfg, "return_temp").c_str(), "temperature", "°C", "mdi:thermometer");
            publishDiscoveryConfig(cfg, "sensor", "outside_temp", "Outside Temperature",
                                   buildStateTopic(cfg, "outside_temp").c_str(), "temperature", "°C", "mdi:thermometer");
            publishDiscoveryConfig(cfg, "sensor", "room_temp", "Room Temperature",
                                   buildStateTopic(cfg, "room_temp").c_str(), "temperature", "°C", "mdi:home-thermometer");
            publishDiscoveryConfig(cfg, "sensor", "exhaust_temp", "Exhaust Temperature",
                                   buildStateTopic(cfg, "exhaust_temp").c_str(), "temperature", "°C", "mdi:thermometer");

            // Numbers (setpoints)
            publishDiscoveryConfig(cfg, "number", "control_setpoint", "Control Setpoint",
                                   buildStateTopic(cfg, "control_setpoint").c_str(), nullptr, "°C", "mdi:thermometer-lines",
                                   buildCommandTopic(cfg, "control_setpoint").c_str(), nullptr, 0.0f, 100.0f, 0.5f);
            publishDiscoveryConfig(cfg, "number", "room_setpoint", "Room Setpoint",
                                   buildStateTopic(cfg, "room_setpoint").c_str(), nullptr, "°C", "mdi:home-thermometer-outline",
                                   buildCommandTopic(cfg, "room_setpoint").c_str(), nullptr, 5.0f, 30.0f, 0.5f);
            publishDiscoveryConfig(cfg, "number", "dhw_setpoint", "Hot Water Setpoint",
                                   buildStateTopic(cfg, "dhw_setpoint").c_str(), nullptr, "°C", "mdi:water-thermometer-outline",
                                   buildCommandTopic(cfg, "dhw_setpoint").c_str(), nullptr, 30.0f, 90.0f, 1.0f);
            publishDiscoveryConfig(cfg, "number", "max_ch_setpoint", "Max CH Setpoint",
                                   buildStateTopic(cfg, "max_ch_setpoint").c_str(), nullptr, "°C", "mdi:thermometer-high",
                                   buildCommandTopic(cfg, "max_ch_setpoint").c_str(), nullptr, 30.0f, 90.0f, 1.0f);

            // Modulation sensors
            publishDiscoveryConfig(cfg, "sensor", "modulation", "Modulation Level",
                                   buildStateTopic(cfg, "modulation").c_str(), nullptr, "%", "mdi:percent");
            publishDiscoveryConfig(cfg, "sensor", "max_modulation", "Max Modulation Level",
                                   buildStateTopic(cfg, "max_modulation").c_str(), nullptr, "%", "mdi:percent");

            // Pressure & flow
            publishDiscoveryConfig(cfg, "sensor", "pressure", "CH Water Pressure",
                                   buildStateTopic(cfg, "pressure").c_str(), "pressure", "bar", "mdi:gauge");
            publishDiscoveryConfig(cfg, "sensor", "dhw_flow", "Hot Water Flow Rate",
                                   buildStateTopic(cfg, "dhw_flow").c_str(), nullptr, "l/min", "mdi:water-pump");

            // Counters
            publishDiscoveryConfig(cfg, "sensor", "burner_starts", "Burner Starts",
                                   buildStateTopic(cfg, "burner_starts").c_str(), nullptr, "starts", "mdi:counter");
            publishDiscoveryConfig(cfg, "sensor", "ch_pump_starts", "CH Pump Starts",
                                   buildStateTopic(cfg, "ch_pump_starts").c_str(), nullptr, "starts", "mdi:counter");
            publishDiscoveryConfig(cfg, "sensor", "dhw_pump_starts", "DHW Pump Starts",
                                   buildStateTopic(cfg, "dhw_pump_starts").c_str(), nullptr, "starts", "mdi:counter");
            publishDiscoveryConfig(cfg, "sensor", "burner_hours", "Burner Operating Hours",
                                   buildStateTopic(cfg, "burner_hours").c_str(), "duration", "h", "mdi:clock-outline");
            publishDiscoveryConfig(cfg, "sensor", "ch_pump_hours", "CH Pump Operating Hours",
                                   buildStateTopic(cfg, "ch_pump_hours").c_str(), "duration", "h", "mdi:clock-outline");
            publishDiscoveryConfig(cfg, "sensor", "dhw_pump_hours", "DHW Pump Operating Hours",
                                   buildStateTopic(cfg, "dhw_pump_hours").c_str(), "duration", "h", "mdi:clock-outline");

            // Faults and config
            publishDiscoveryConfig(cfg, "sensor", "fault_code", "Fault Code",
                                   buildStateTopic(cfg, "fault_code").c_str(), nullptr, nullptr, "mdi:alert-octagon");

            publishDiscoveryConfig(cfg, "binary_sensor", "dhw_present", "DHW Present",
                                   buildStateTopic(cfg, "dhw_present").c_str(), nullptr, nullptr, "mdi:water-boiler");
            publishDiscoveryConfig(cfg, "binary_sensor", "cooling_supported", "Cooling Supported",
                                   buildStateTopic(cfg, "cooling_supported").c_str(), nullptr, nullptr, "mdi:snowflake");
            publishDiscoveryConfig(cfg, "binary_sensor", "ch2_present", "CH2 Present",
                                   buildStateTopic(cfg, "ch2_present").c_str(), nullptr, nullptr, "mdi:radiator");

            publishDiscoveryConfig(cfg, "sensor", "opentherm_version", "OpenTherm Version",
                                   buildStateTopic(cfg, "opentherm_version").c_str(), nullptr, nullptr, "mdi:information");

            // Text / device config
            publishDiscoveryConfig(cfg, "text", "device_name", "Device Name",
                                   buildStateTopic(cfg, "device_name").c_str(), nullptr, nullptr, "mdi:tag-text",
                                   buildCommandTopic(cfg, "device_name").c_str());
            publishDiscoveryConfig(cfg, "text", "device_id", "Device ID",
                                   buildStateTopic(cfg, "device_id").c_str(), nullptr, nullptr, "mdi:identifier",
                                   buildCommandTopic(cfg, "device_id").c_str());

            // GPIO numbers
            publishDiscoveryConfig(cfg, "number", "opentherm_tx_pin", "OpenTherm TX Pin",
                                   buildStateTopic(cfg, "opentherm_tx_pin").c_str(), nullptr, nullptr, "mdi:pin",
                                   buildCommandTopic(cfg, "opentherm_tx_pin").c_str(), nullptr, 0.0f, 28.0f, 1.0f);
            publishDiscoveryConfig(cfg, "number", "opentherm_rx_pin", "OpenTherm RX Pin",
                                   buildStateTopic(cfg, "opentherm_rx_pin").c_str(), nullptr, nullptr, "mdi:pin",
                                   buildCommandTopic(cfg, "opentherm_rx_pin").c_str(), nullptr, 0.0f, 28.0f, 1.0f);

            printf("Discovery configs published!\n");
            return true;
        }

        void publishSensor(const OpenTherm::HomeAssistant::Config &cfg, const char *topic_suffix, float value)
        {
            char payload[32];
            snprintf(payload, sizeof(payload), "%.2f", value);
            std::string topic = buildStateTopic(cfg, topic_suffix);
            OpenTherm::Common::mqtt_publish_wrapper(topic.c_str(), payload, false);
        }

        void publishSensor(const OpenTherm::HomeAssistant::Config &cfg, const char *topic_suffix, int value)
        {
            char payload[32];
            snprintf(payload, sizeof(payload), "%d", value);
            std::string topic = buildStateTopic(cfg, topic_suffix);
            OpenTherm::Common::mqtt_publish_wrapper(topic.c_str(), payload, false);
        }

        void publishSensor(const OpenTherm::HomeAssistant::Config &cfg, const char *topic_suffix, const char *value)
        {
            std::string topic = buildStateTopic(cfg, topic_suffix);
            OpenTherm::Common::mqtt_publish_wrapper(topic.c_str(), value, false);
        }

        void publishBinarySensor(const OpenTherm::HomeAssistant::Config &cfg, const char *topic_suffix, bool value)
        {
            std::string topic = buildStateTopic(cfg, topic_suffix);
            OpenTherm::Common::mqtt_publish_wrapper(topic.c_str(), value ? "ON" : "OFF", false);
        }

    } // namespace Discovery
} // namespace OpenTherm
