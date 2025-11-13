#ifndef MQTT_DISCOVERY_HPP
#define MQTT_DISCOVERY_HPP

#include <cstdio>
#include <string>

// Forward declaration to avoid header dependency
namespace OpenTherm
{
    namespace HomeAssistant
    {
        struct Config;
    }
}

namespace OpenTherm
{
    namespace Discovery
    {
        // NOTE: simulator-specific discovery helper removed — use publishDiscoveryConfigs

        // Build topics based on Home Assistant config
        std::string buildStateTopic(const OpenTherm::HomeAssistant::Config &cfg, const char *suffix);
        std::string buildCommandTopic(const OpenTherm::HomeAssistant::Config &cfg, const char *suffix);
        std::string buildDiscoveryTopic(const OpenTherm::HomeAssistant::Config &cfg, const char *component, const char *object_id);

        // Publish a single discovery config (uses publishWithRetry)
        bool publishDiscoveryConfig(const OpenTherm::HomeAssistant::Config &cfg,
                        const char *component, const char *object_id,
                        const char *name, const char *state_topic,
                        const char *device_class = nullptr,
                        const char *unit = nullptr,
                        const char *icon = nullptr,
                        const char *command_topic = nullptr,
                        const char *value_template = nullptr,
                        float min_value = 0.0f,
                        float max_value = 100.0f,
                        float step = 1.0f);

        // Publish all discovery configs for a Home Assistant `Config`
        bool publishDiscoveryConfigs(const OpenTherm::HomeAssistant::Config &cfg);

        // Simple publish helpers that use the shared MQTT wrapper
        void publishSensor(const OpenTherm::HomeAssistant::Config &cfg, const char *topic_suffix, float value);
        void publishSensor(const OpenTherm::HomeAssistant::Config &cfg, const char *topic_suffix, int value);
        void publishSensor(const OpenTherm::HomeAssistant::Config &cfg, const char *topic_suffix, const char *value);
        void publishBinarySensor(const OpenTherm::HomeAssistant::Config &cfg, const char *topic_suffix, bool value);

        /**
         * Publish a single discovery message with retry logic and exponential backoff
         *
         * @param topic The MQTT topic to publish to
         * @param config The JSON configuration payload
         * @param max_retries Maximum number of retry attempts (default: 5)
         * @return true if publish succeeded, false if all retries failed
         */
        bool publishWithRetry(const char *topic, const char *config, int max_retries = 5);

    } // namespace Discovery
} // namespace OpenTherm

#endif // MQTT_DISCOVERY_HPP
