#ifndef MQTT_DISCOVERY_HPP
#define MQTT_DISCOVERY_HPP

#include <cstdio>

namespace OpenTherm
{
    namespace Discovery
    {
        /**
         * Publish Home Assistant MQTT discovery configurations for simulator sensors
         *
         * @param device_name The friendly name of the device
         * @param device_id The unique ID of the device (used in topics and entity IDs)
         * @return true if all discovery messages published successfully, false otherwise
         */
        bool publishSimulatorDiscovery(const char *device_name, const char *device_id);

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
