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
            return std::string(cfg.mqtt_prefix) + "/" + component + "/" + cfg.device_id + "/" + object_id + OpenTherm::MQTTDiscovery::CONFIG_SUFFIX;
        }

        bool publishDiscoveryConfig(const OpenTherm::HomeAssistant::Config &cfg,
                                    const char *component, const char *object_id,
                                    const char *name, const char *state_topic,
                                    const char *device_class, const char *unit,
                                    const char *icon, const char *command_topic,
                                    const char *value_template,
                                    float min_value, float max_value, float step)
        {
            using namespace OpenTherm::MQTTDiscovery;

            std::ostringstream payload;
            payload << "{";
            payload << "\"" << JSON_NAME << "\":\"" << name << "\",";
            payload << "\"" << JSON_OBJECT_ID << "\":\"" << object_id << "\",";
            payload << "\"" << JSON_UNIQUE_ID << "\":\"" << cfg.device_id << "_" << object_id << "\",";
            payload << "\"" << JSON_STATE_TOPIC << "\":\"" << state_topic << "\",";

            if (command_topic)
            {
                payload << "\"" << JSON_COMMAND_TOPIC << "\":\"" << command_topic << "\",";
            }

            if (device_class)
            {
                payload << "\"" << JSON_DEVICE_CLASS << "\":\"" << device_class << "\",";
            }

            if (unit)
            {
                payload << "\"" << JSON_UNIT_OF_MEASUREMENT << "\":\"" << unit << "\",";
            }

            if (icon)
            {
                payload << "\"" << JSON_ICON << "\":\"" << icon << "\",";
            }

            if (value_template)
            {
                payload << "\"" << JSON_VALUE_TEMPLATE << "\":\"" << value_template << "\",";
            }

            if (strcmp(component, COMPONENT_NUMBER) == 0)
            {
                payload << "\"" << JSON_MIN << "\":" << min_value << ",";
                payload << "\"" << JSON_MAX << "\":" << max_value << ",";
                payload << "\"" << JSON_STEP << "\":" << step << ",";
                payload << "\"" << JSON_MODE << "\":\"" << MODE_BOX << "\",";
            }

            payload << "\"" << JSON_DEVICE << "\":{";
            payload << "\"" << JSON_IDENTIFIERS << "\":[\"" << cfg.device_id << "\"],";
            payload << "\"" << JSON_NAME << "\":\"" << cfg.device_name << "\",";
            payload << "\"" << JSON_MODEL << "\":\"" << DEVICE_MODEL << "\",";
            payload << "\"" << JSON_MANUFACTURER << "\":\"" << DEVICE_MANUFACTURER << "\"";
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
            using namespace OpenTherm::MQTTDiscovery;

            // Binary Sensors
            publishDiscoveryConfig(cfg, COMPONENT_BINARY_SENSOR, FAULT, NAME_FAULT,
                                   buildStateTopic(cfg, FAULT).c_str(), DEVICE_CLASS_PROBLEM, nullptr, ICON_ALERT_CIRCLE);
            publishDiscoveryConfig(cfg, COMPONENT_BINARY_SENSOR, CH_MODE, NAME_CH_MODE,
                                   buildStateTopic(cfg, CH_MODE).c_str(), DEVICE_CLASS_HEAT, nullptr, ICON_RADIATOR);
            publishDiscoveryConfig(cfg, COMPONENT_BINARY_SENSOR, DHW_MODE, NAME_DHW_MODE,
                                   buildStateTopic(cfg, DHW_MODE).c_str(), DEVICE_CLASS_HEAT, nullptr, ICON_WATER_BOILER);
            publishDiscoveryConfig(cfg, COMPONENT_BINARY_SENSOR, FLAME, NAME_FLAME,
                                   buildStateTopic(cfg, FLAME).c_str(), DEVICE_CLASS_HEAT, nullptr, ICON_FIRE);
            publishDiscoveryConfig(cfg, COMPONENT_BINARY_SENSOR, COOLING, NAME_COOLING,
                                   buildStateTopic(cfg, COOLING).c_str(), DEVICE_CLASS_COLD, nullptr, ICON_SNOWFLAKE);
            publishDiscoveryConfig(cfg, COMPONENT_BINARY_SENSOR, DIAGNOSTIC, NAME_DIAGNOSTIC,
                                   buildStateTopic(cfg, DIAGNOSTIC).c_str(), nullptr, nullptr, ICON_WRENCH);

            // Switches
            publishDiscoveryConfig(cfg, COMPONENT_SWITCH, CH_ENABLE, NAME_CH_ENABLE,
                                   buildStateTopic(cfg, CH_ENABLE).c_str(), DEVICE_CLASS_SWITCH, nullptr, ICON_RADIATOR,
                                   buildCommandTopic(cfg, CH_ENABLE).c_str());
            publishDiscoveryConfig(cfg, COMPONENT_SWITCH, DHW_ENABLE, NAME_DHW_ENABLE,
                                   buildStateTopic(cfg, DHW_ENABLE).c_str(), DEVICE_CLASS_SWITCH, nullptr, ICON_WATER_BOILER,
                                   buildCommandTopic(cfg, DHW_ENABLE).c_str());

            // Temperature sensors
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, BOILER_TEMP, NAME_BOILER_TEMP,
                                   buildStateTopic(cfg, BOILER_TEMP).c_str(), DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS, ICON_THERMOMETER);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, DHW_TEMP, NAME_DHW_TEMP,
                                   buildStateTopic(cfg, DHW_TEMP).c_str(), DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS, ICON_THERMOMETER);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, RETURN_TEMP, NAME_RETURN_TEMP,
                                   buildStateTopic(cfg, RETURN_TEMP).c_str(), DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS, ICON_THERMOMETER);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, OUTSIDE_TEMP, NAME_OUTSIDE_TEMP,
                                   buildStateTopic(cfg, OUTSIDE_TEMP).c_str(), DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS, ICON_THERMOMETER);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, ROOM_TEMP, NAME_ROOM_TEMP,
                                   buildStateTopic(cfg, ROOM_TEMP).c_str(), DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS, ICON_HOME_THERMOMETER);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, EXHAUST_TEMP, NAME_EXHAUST_TEMP,
                                   buildStateTopic(cfg, EXHAUST_TEMP).c_str(), DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS, ICON_THERMOMETER);

            // Numbers (setpoints)
            publishDiscoveryConfig(cfg, COMPONENT_NUMBER, CONTROL_SETPOINT, NAME_CONTROL_SETPOINT,
                                   buildStateTopic(cfg, CONTROL_SETPOINT).c_str(), nullptr, UNIT_CELSIUS, ICON_THERMOMETER_LINES,
                                   buildCommandTopic(cfg, CONTROL_SETPOINT).c_str(), nullptr, 0.0f, 100.0f, 0.5f);
            publishDiscoveryConfig(cfg, COMPONENT_NUMBER, ROOM_SETPOINT, NAME_ROOM_SETPOINT,
                                   buildStateTopic(cfg, ROOM_SETPOINT).c_str(), nullptr, UNIT_CELSIUS, ICON_HOME_THERMOMETER_OUTLINE,
                                   buildCommandTopic(cfg, ROOM_SETPOINT).c_str(), nullptr, 5.0f, 30.0f, 0.5f);
            publishDiscoveryConfig(cfg, COMPONENT_NUMBER, DHW_SETPOINT, NAME_DHW_SETPOINT,
                                   buildStateTopic(cfg, DHW_SETPOINT).c_str(), nullptr, UNIT_CELSIUS, ICON_WATER_THERMOMETER_OUTLINE,
                                   buildCommandTopic(cfg, DHW_SETPOINT).c_str(), nullptr, 30.0f, 90.0f, 1.0f);
            publishDiscoveryConfig(cfg, COMPONENT_NUMBER, MAX_CH_SETPOINT, NAME_MAX_CH_SETPOINT,
                                   buildStateTopic(cfg, MAX_CH_SETPOINT).c_str(), nullptr, UNIT_CELSIUS, ICON_THERMOMETER_HIGH,
                                   buildCommandTopic(cfg, MAX_CH_SETPOINT).c_str(), nullptr, 30.0f, 90.0f, 1.0f);

            // Modulation sensors
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, MODULATION, NAME_MODULATION,
                                   buildStateTopic(cfg, MODULATION).c_str(), nullptr, UNIT_PERCENT, ICON_PERCENT);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, MAX_MODULATION, NAME_MAX_MODULATION,
                                   buildStateTopic(cfg, MAX_MODULATION).c_str(), nullptr, UNIT_PERCENT, ICON_PERCENT);

            // Pressure & flow
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, PRESSURE, NAME_PRESSURE,
                                   buildStateTopic(cfg, PRESSURE).c_str(), DEVICE_CLASS_PRESSURE, UNIT_BAR, ICON_GAUGE);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, DHW_FLOW, NAME_DHW_FLOW,
                                   buildStateTopic(cfg, DHW_FLOW).c_str(), nullptr, UNIT_LITERS_PER_MIN, ICON_WATER_PUMP);

            // Counters
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, BURNER_STARTS, NAME_BURNER_STARTS,
                                   buildStateTopic(cfg, BURNER_STARTS).c_str(), nullptr, UNIT_STARTS, ICON_COUNTER);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, CH_PUMP_STARTS, NAME_CH_PUMP_STARTS,
                                   buildStateTopic(cfg, CH_PUMP_STARTS).c_str(), nullptr, UNIT_STARTS, ICON_COUNTER);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, DHW_PUMP_STARTS, NAME_DHW_PUMP_STARTS,
                                   buildStateTopic(cfg, DHW_PUMP_STARTS).c_str(), nullptr, UNIT_STARTS, ICON_COUNTER);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, BURNER_HOURS, NAME_BURNER_HOURS,
                                   buildStateTopic(cfg, BURNER_HOURS).c_str(), DEVICE_CLASS_DURATION, UNIT_HOURS, ICON_CLOCK_OUTLINE);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, CH_PUMP_HOURS, NAME_CH_PUMP_HOURS,
                                   buildStateTopic(cfg, CH_PUMP_HOURS).c_str(), DEVICE_CLASS_DURATION, UNIT_HOURS, ICON_CLOCK_OUTLINE);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, DHW_PUMP_HOURS, NAME_DHW_PUMP_HOURS,
                                   buildStateTopic(cfg, DHW_PUMP_HOURS).c_str(), DEVICE_CLASS_DURATION, UNIT_HOURS, ICON_CLOCK_OUTLINE);

            // Faults and config
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, FAULT_CODE, NAME_FAULT_CODE,
                                   buildStateTopic(cfg, FAULT_CODE).c_str(), nullptr, nullptr, ICON_ALERT_OCTAGON);

            // Diagnostic code
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, DIAGNOSTIC_CODE, NAME_DIAGNOSTIC_CODE,
                                   buildStateTopic(cfg, DIAGNOSTIC_CODE).c_str(), nullptr, nullptr, ICON_ALERT_CIRCLE);

            publishDiscoveryConfig(cfg, COMPONENT_BINARY_SENSOR, DHW_PRESENT, NAME_DHW_PRESENT,
                                   buildStateTopic(cfg, DHW_PRESENT).c_str(), nullptr, nullptr, ICON_WATER_BOILER);
            publishDiscoveryConfig(cfg, COMPONENT_BINARY_SENSOR, COOLING_SUPPORTED, NAME_COOLING_SUPPORTED,
                                   buildStateTopic(cfg, COOLING_SUPPORTED).c_str(), nullptr, nullptr, ICON_SNOWFLAKE);
            publishDiscoveryConfig(cfg, COMPONENT_BINARY_SENSOR, CH2_PRESENT, NAME_CH2_PRESENT,
                                   buildStateTopic(cfg, CH2_PRESENT).c_str(), nullptr, nullptr, ICON_RADIATOR);

            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, OPENTHERM_VERSION, NAME_OPENTHERM_VERSION,
                                   buildStateTopic(cfg, OPENTHERM_VERSION).c_str(), nullptr, nullptr, ICON_INFORMATION);

            // Text / device config
            publishDiscoveryConfig(cfg, COMPONENT_TEXT, DEVICE_NAME, NAME_DEVICE_NAME,
                                   buildStateTopic(cfg, DEVICE_NAME).c_str(), nullptr, nullptr, ICON_TAG_TEXT,
                                   buildCommandTopic(cfg, DEVICE_NAME).c_str());
            publishDiscoveryConfig(cfg, COMPONENT_TEXT, DEVICE_ID, NAME_DEVICE_ID,
                                   buildStateTopic(cfg, DEVICE_ID).c_str(), nullptr, nullptr, ICON_IDENTIFIER,
                                   buildCommandTopic(cfg, DEVICE_ID).c_str());

            // GPIO numbers
            publishDiscoveryConfig(cfg, COMPONENT_NUMBER, OPENTHERM_TX_PIN, NAME_OPENTHERM_TX_PIN,
                                   buildStateTopic(cfg, OPENTHERM_TX_PIN).c_str(), nullptr, nullptr, ICON_PIN,
                                   buildCommandTopic(cfg, OPENTHERM_TX_PIN).c_str(), nullptr, 0.0f, 28.0f, 1.0f);
            publishDiscoveryConfig(cfg, COMPONENT_NUMBER, OPENTHERM_RX_PIN, NAME_OPENTHERM_RX_PIN,
                                   buildStateTopic(cfg, OPENTHERM_RX_PIN).c_str(), nullptr, nullptr, ICON_PIN,
                                   buildCommandTopic(cfg, OPENTHERM_RX_PIN).c_str(), nullptr, 0.0f, 28.0f, 1.0f);

            // Time/Date sensors (read-only from boiler)
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, DAY_OF_WEEK, NAME_DAY_OF_WEEK,
                                   buildStateTopic(cfg, DAY_OF_WEEK).c_str(), nullptr, nullptr, ICON_CALENDAR);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, TIME_OF_DAY, NAME_TIME_OF_DAY,
                                   buildStateTopic(cfg, TIME_OF_DAY).c_str(), nullptr, nullptr, ICON_CLOCK_OUTLINE);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, DATE, NAME_DATE,
                                   buildStateTopic(cfg, DATE).c_str(), nullptr, nullptr, ICON_CALENDAR_TODAY);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, YEAR, NAME_YEAR,
                                   buildStateTopic(cfg, YEAR).c_str(), nullptr, nullptr, ICON_CALENDAR);

            // Time sync button - pressing this will sync time from HA to boiler
            publishDiscoveryConfig(cfg, COMPONENT_BUTTON, SYNC_TIME, NAME_SYNC_TIME,
                                   buildStateTopic(cfg, SYNC_TIME).c_str(), nullptr, nullptr, ICON_CLOCK_SYNC,
                                   buildCommandTopic(cfg, SYNC_TIME).c_str());

            // Temperature bounds (read-only from boiler)
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, DHW_SETPOINT_MIN, NAME_DHW_SETPOINT_MIN,
                                   buildStateTopic(cfg, DHW_SETPOINT_MIN).c_str(), DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS, ICON_THERMOMETER_LOW);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, DHW_SETPOINT_MAX, NAME_DHW_SETPOINT_MAX,
                                   buildStateTopic(cfg, DHW_SETPOINT_MAX).c_str(), DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS, ICON_THERMOMETER_HIGH);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, CH_SETPOINT_MIN, NAME_CH_SETPOINT_MIN,
                                   buildStateTopic(cfg, CH_SETPOINT_MIN).c_str(), DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS, ICON_THERMOMETER_LOW);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, CH_SETPOINT_MAX, NAME_CH_SETPOINT_MAX,
                                   buildStateTopic(cfg, CH_SETPOINT_MAX).c_str(), DEVICE_CLASS_TEMPERATURE, UNIT_CELSIUS, ICON_THERMOMETER_HIGH);

            // WiFi statistics
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, WIFI_RSSI, NAME_WIFI_RSSI,
                                   buildStateTopic(cfg, WIFI_RSSI).c_str(), DEVICE_CLASS_SIGNAL_STRENGTH, UNIT_DBM, ICON_WIFI);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, WIFI_LINK_STATUS, NAME_WIFI_LINK_STATUS,
                                   buildStateTopic(cfg, WIFI_LINK_STATUS).c_str(), nullptr, nullptr, ICON_WIFI_CHECK);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, IP_ADDRESS, NAME_IP_ADDRESS,
                                   buildStateTopic(cfg, IP_ADDRESS).c_str(), nullptr, nullptr, ICON_IP_NETWORK);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, WIFI_SSID, NAME_WIFI_SSID,
                                   buildStateTopic(cfg, WIFI_SSID).c_str(), nullptr, nullptr, ICON_WIFI_MARKER);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, UPTIME, NAME_UPTIME,
                                   buildStateTopic(cfg, UPTIME).c_str(), DEVICE_CLASS_DURATION, UNIT_SECONDS, ICON_CLOCK_START);
            publishDiscoveryConfig(cfg, COMPONENT_SENSOR, FREE_HEAP, NAME_FREE_HEAP,
                                   buildStateTopic(cfg, FREE_HEAP).c_str(), DEVICE_CLASS_DATA_SIZE, UNIT_BYTES, ICON_MEMORY);

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
