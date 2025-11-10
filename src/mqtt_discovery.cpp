#include "mqtt_discovery.hpp"
#include "mqtt_common.hpp"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include <cstdio>
#include <cstring>

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

        bool publishSimulatorDiscovery(const char *device_name, const char *device_id)
        {
            // Wait for MQTT client to be ready for large discovery messages
            // Poll network actively to process any pending packets
            // Extended wait (5s) to allow MQTT handshake and buffers to fully stabilize
            printf("Waiting for MQTT client to be ready for discovery (5 seconds, polling network)...\n");
            for (int i = 0; i < 500; i++)
            {
                cyw43_arch_poll();
                sleep_ms(10);
            }

            printf("Publishing Home Assistant discovery configurations...\n");

            char topic[256];
            char config[512];

            // Room temperature sensor
            printf("Publishing room temperature sensor...\n");
            snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/room_temperature/config", device_id);
            snprintf(config, sizeof(config),
                     "{\"name\":\"%s Room Temperature\",\"unique_id\":\"%s_room_temp\","
                     "\"default_entity_id\":\"sensor.%s_room_temp\","
                     "\"state_topic\":\"opentherm/state/%s/room_temperature\","
                     "\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\","
                     "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\",\"manufacturer\":\"PicoOpenTherm\",\"model\":\"Simulator\"}}",
                     device_name, device_id, device_id, device_id, device_id, device_name);
            if (!publishWithRetry(topic, config))
            {
                return false;
            }

            // Boiler temperature sensor
            printf("Publishing boiler temperature sensor...\n");
            snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/boiler_temperature/config", device_id);
            snprintf(config, sizeof(config),
                     "{\"name\":\"%s Boiler Temperature\",\"unique_id\":\"%s_boiler_temp\","
                     "\"default_entity_id\":\"sensor.%s_boiler_temp\","
                     "\"state_topic\":\"opentherm/state/%s/boiler_temperature\","
                     "\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\","
                     "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\",\"manufacturer\":\"PicoOpenTherm\",\"model\":\"Simulator\"}}",
                     device_name, device_id, device_id, device_id, device_id, device_name);
            if (!publishWithRetry(topic, config))
            {
                return false;
            }

            // DHW temperature sensor
            printf("Publishing DHW temperature sensor...\n");
            snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/dhw_temperature/config", device_id);
            snprintf(config, sizeof(config),
                     "{\"name\":\"%s DHW Temperature\",\"unique_id\":\"%s_dhw_temp\","
                     "\"default_entity_id\":\"sensor.%s_dhw_temp\","
                     "\"state_topic\":\"opentherm/state/%s/dhw_temperature\","
                     "\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\","
                     "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\",\"manufacturer\":\"PicoOpenTherm\",\"model\":\"Simulator\"}}",
                     device_name, device_id, device_id, device_id, device_id, device_name);
            if (!publishWithRetry(topic, config))
            {
                return false;
            }

            // Modulation sensor
            printf("Publishing modulation sensor...\n");
            snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/modulation/config", device_id);
            snprintf(config, sizeof(config),
                     "{\"name\":\"%s Modulation\",\"unique_id\":\"%s_modulation\","
                     "\"default_entity_id\":\"sensor.%s_modulation\","
                     "\"state_topic\":\"opentherm/state/%s/modulation\","
                     "\"unit_of_measurement\":\"%%\","
                     "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\",\"manufacturer\":\"PicoOpenTherm\",\"model\":\"Simulator\"}}",
                     device_name, device_id, device_id, device_id, device_id, device_name);
            if (!publishWithRetry(topic, config))
            {
                return false;
            }

            // Pressure sensor
            printf("Publishing pressure sensor...\n");
            snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/pressure/config", device_id);
            snprintf(config, sizeof(config),
                     "{\"name\":\"%s CH Pressure\",\"unique_id\":\"%s_pressure\","
                     "\"default_entity_id\":\"sensor.%s_pressure\","
                     "\"state_topic\":\"opentherm/state/%s/pressure\","
                     "\"unit_of_measurement\":\"bar\",\"device_class\":\"pressure\","
                     "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\",\"manufacturer\":\"PicoOpenTherm\",\"model\":\"Simulator\"}}",
                     device_name, device_id, device_id, device_id, device_id, device_name);
            if (!publishWithRetry(topic, config))
            {
                return false;
            }

            // Flame status binary sensor
            printf("Publishing flame status sensor...\n");
            snprintf(topic, sizeof(topic), "homeassistant/binary_sensor/%s/flame/config", device_id);
            snprintf(config, sizeof(config),
                     "{\"name\":\"%s Flame Status\",\"unique_id\":\"%s_flame\","
                     "\"default_entity_id\":\"binary_sensor.%s_flame\","
                     "\"state_topic\":\"opentherm/state/%s/flame\","
                     "\"payload_on\":\"ON\",\"payload_off\":\"OFF\","
                     "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\",\"manufacturer\":\"PicoOpenTherm\",\"model\":\"Simulator\"}}",
                     device_name, device_id, device_id, device_id, device_id, device_name);
            if (!publishWithRetry(topic, config))
            {
                return false;
            }

            printf("Discovery configuration complete!\n");
            return true;
        }

    } // namespace Discovery
} // namespace OpenTherm
