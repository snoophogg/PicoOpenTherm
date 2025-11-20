#include "mqtt_common.hpp"
#include "led_blink.hpp"
#include <cstdio>

namespace OpenTherm
{
    namespace Common
    {
        // Global MQTT state
        mqtt_client_t *g_mqtt_client = nullptr;
        bool g_mqtt_connected = false;
        std::map<std::string, std::string> g_pending_messages;

        // Track consecutive publish failures for LED indication
        static int consecutive_publish_failures = 0;
        static constexpr int PUBLISH_FAILURE_THRESHOLD = 5;

        // Network polling helper to prevent TCP buffer exhaustion
        // Note: With Core 1 dedicated to network polling, this is now much lighter
        // Core 1 continuously polls cyw43_arch_poll(), so we only need brief yields
        void aggressive_network_poll(int duration_ms)
        {
            // With dual-core: Core 1 handles continuous polling in parallel
            // We just yield briefly to allow any pending TCP operations to complete
            // This is MUCH faster than the previous implementation
            if (duration_ms > 0)
            {
                sleep_ms(duration_ms);
            }
        }

        // MQTT callbacks
        void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
        {
            if (status == MQTT_CONNECT_ACCEPTED)
            {
                printf("MQTT connected!\n");
                g_mqtt_connected = true;
                consecutive_publish_failures = 0; // Reset failure counter on successful connection
            }
            else
            {
                printf("MQTT connection failed: %d\n", status);
                g_mqtt_connected = false;
            }
        }

        // Static variables to track incoming MQTT message state
        static std::string current_topic;
        static std::string current_payload;

        void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
        {
            printf("Incoming publish at topic %s with total length %u\n", topic, (unsigned int)tot_len);
            // Store the topic for the incoming data callback
            current_topic = topic;
            current_payload.clear();
        }

        void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
        {
            current_payload.append((const char *)data, len);

            if (flags & MQTT_DATA_FLAG_LAST)
            {
                printf("Received: %s = %s\n", current_topic.c_str(), current_payload.c_str());
                g_pending_messages[current_topic] = current_payload;
                current_payload.clear();
            }
        }

        void mqtt_sub_request_cb(void *arg, err_t result)
        {
            printf("Subscribe result: %d\n", result);
        }

        // MQTT wrapper functions
        bool mqtt_publish_wrapper(const char *topic, const char *payload, bool retain)
        {
            if (!g_mqtt_connected || !g_mqtt_client)
            {
                return false;
            }

            // Check if MQTT client is actually ready to publish
            if (!mqtt_client_is_connected(g_mqtt_client))
            {
                printf("MQTT publish failed: client not connected\n");
                g_mqtt_connected = false;
                return false;
            }

            u8_t qos = 0;
            u8_t retain_flag = retain ? 1 : 0;

            // Retry logic for memory errors - TCP buffers may be temporarily full
            const int max_retries = 3;
            err_t err = ERR_OK;

            for (int retry = 0; retry < max_retries; retry++)
            {
                err = mqtt_publish(g_mqtt_client, topic, payload, strlen(payload),
                                   qos, retain_flag, nullptr, nullptr);

                if (err == ERR_OK)
                {
                    break; // Success!
                }

                // If memory error, wait for TCP buffers to clear and retry
                if (err == ERR_MEM || err == ERR_BUF)
                {
                    if (retry < max_retries - 1)
                    {
                        // Wait progressively longer: 100ms, 200ms, 300ms
                        // Longer waits allow more time for TCP ACKs to free buffers
                        uint32_t wait_ms = 100 * (retry + 1);
                        printf("MQTT publish ERR_MEM, waiting %lums before retry %d/%d...\n",
                               wait_ms, retry + 1, max_retries);
                        aggressive_network_poll(wait_ms);
                        continue;
                    }
                }
                else
                {
                    // Non-memory error, don't retry
                    break;
                }
            }

            if (err != ERR_OK)
            {
                const char *err_str = "unknown";
                switch (err)
                {
                case ERR_MEM:
                    err_str = "out of memory (ERR_MEM)";
                    break;
                case ERR_BUF:
                    err_str = "buffer error (ERR_BUF)";
                    break;
                case ERR_TIMEOUT:
                    err_str = "timeout (ERR_TIMEOUT)";
                    break;
                case ERR_RTE:
                    err_str = "routing problem (ERR_RTE)";
                    break;
                case ERR_CONN:
                    err_str = "not connected (ERR_CONN)";
                    g_mqtt_connected = false;
                    break;
                case ERR_CLSD:
                    err_str = "connection closed (ERR_CLSD)";
                    g_mqtt_connected = false;
                    break;
                default:
                    break;
                }
                printf("MQTT publish failed after %d attempts: %s (%d) - topic: %s\n",
                       max_retries, err_str, err, topic);

                // Track consecutive failures and update LED pattern
                consecutive_publish_failures++;
                if (consecutive_publish_failures >= PUBLISH_FAILURE_THRESHOLD)
                {
                    printf("Multiple consecutive publish failures detected - setting MQTT error LED\n");
                    OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_MQTT_ERROR);
                }

                // Add delay on error to prevent flooding
                aggressive_network_poll(100);
                return false;
            }

            // Successful publish - reset failure counter
            consecutive_publish_failures = 0;

            // Delay between publishes to allow lwIP buffers to be freed
            // CRITICAL: Even with Core 1 processing network continuously, TCP ACKs
            // take time to return over network. Insufficient delay causes long-term
            // buffer accumulation leading to panic after 40+ minutes of operation.
            // 50ms allows reliable ACK processing and prevents TCP queue corruption.
            aggressive_network_poll(50); // 50ms - prevents long-term buffer leak
            return true;
        }

        bool mqtt_subscribe_wrapper(const char *topic)
        {
            if (!g_mqtt_connected || !g_mqtt_client)
            {
                printf("MQTT subscribe failed: not connected\n");
                return false;
            }

            err_t err = mqtt_subscribe(g_mqtt_client, topic, 0, mqtt_sub_request_cb, nullptr);

            if (err != ERR_OK)
            {
                const char *err_str = "unknown";
                switch (err)
                {
                case ERR_MEM:
                    err_str = "out of memory (ERR_MEM)";
                    break;
                case ERR_BUF:
                    err_str = "buffer error (ERR_BUF)";
                    break;
                case ERR_CONN:
                    err_str = "not connected (ERR_CONN)";
                    break;
                default:
                    break;
                }
                printf("MQTT subscribe failed: %s (%d) - topic: %s\n", err_str, err, topic);
                return false;
            }

            printf("Subscribed to: %s\n", topic);

            // Brief delay to allow subscription to be processed
            aggressive_network_poll(10);
            return true;
        }

        // Connection functions
        bool connect_wifi(const char *ssid, const char *password)
        {
            printf("Connecting to WiFi...\n");

            if (cyw43_arch_wifi_connect_timeout_ms(ssid, password,
                                                   CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIMEOUT_MS))
            {
                printf("Failed to connect to WiFi\n");
                return false;
            }

            printf("Connected to WiFi!\n");
            printf("IP Address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
            return true;
        }

        bool connect_mqtt(const char *server_ip, uint16_t port, const char *client_id)
        {
            printf("Connecting to MQTT broker...\n");

            // Clean up any existing client first
            if (g_mqtt_client)
            {
                printf("Cleaning up existing MQTT client...\n");
                mqtt_disconnect(g_mqtt_client);
                mqtt_client_free(g_mqtt_client);
                g_mqtt_client = nullptr;
                g_mqtt_connected = false;
                sleep_ms(100); // Give lwIP time to clean up timers
            }

            g_mqtt_client = mqtt_client_new();
            if (!g_mqtt_client)
            {
                printf("Failed to create MQTT client\n");
                return false;
            }

            struct mqtt_connect_client_info_t ci = {0};
            ci.client_id = client_id;
            ci.keep_alive = 60;

            mqtt_set_inpub_callback(g_mqtt_client, mqtt_incoming_publish_cb,
                                    mqtt_incoming_data_cb, nullptr);

            ip_addr_t mqtt_server;
            if (!ipaddr_aton(server_ip, &mqtt_server))
            {
                printf("Invalid MQTT server IP\n");
                mqtt_client_free(g_mqtt_client);
                g_mqtt_client = nullptr;
                return false;
            }

            err_t err = mqtt_client_connect(g_mqtt_client, &mqtt_server, port,
                                            mqtt_connection_cb, nullptr, &ci);

            if (err != ERR_OK)
            {
                printf("MQTT connect failed: %d\n", err);
                mqtt_client_free(g_mqtt_client);
                g_mqtt_client = nullptr;
                return false;
            }

            // Wait for connection
            for (int i = 0; i < 50 && !g_mqtt_connected; i++)
            {
                sleep_ms(100);
            }

            if (!g_mqtt_connected)
            {
                printf("MQTT connection timeout\n");
                mqtt_disconnect(g_mqtt_client);
                mqtt_client_free(g_mqtt_client);
                g_mqtt_client = nullptr;
            }
            else
            {
                // Give MQTT client time to stabilize before publishing
                printf("MQTT connection established, waiting for client to stabilize...\n");
                sleep_ms(500);
            }

            return g_mqtt_connected;
        }

        bool connect_with_retry(const char *ssid, const char *password,
                                const char *server_ip, uint16_t port, const char *client_id)
        {
            // Connect to WiFi
            int wifi_attempt = 1;
            while (true)
            {
                printf("WiFi connection attempt %d\n", wifi_attempt);

                // Set LED pattern to WiFi error
                OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_WIFI_ERROR);

                if (connect_wifi(ssid, password))
                {
                    printf("WiFi connected!\n");
                    break;
                }

                printf("WiFi connection failed, retrying in %d seconds...\n", WIFI_RETRY_DELAY_MS / 1000);
                sleep_ms(WIFI_RETRY_DELAY_MS);

                wifi_attempt++;
            }

            // Connect to MQTT
            g_mqtt_connected = false;

            int mqtt_attempt = 1;
            while (true)
            {
                printf("MQTT connection attempt %d\n", mqtt_attempt);

                // Set LED pattern to MQTT error
                OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_MQTT_ERROR);

                if (connect_mqtt(server_ip, port, client_id))
                {
                    printf("MQTT connected!\n");
                    return true;
                }

                printf("MQTT connection failed, retrying in %d seconds...\n", MQTT_RETRY_DELAY_MS / 1000);
                sleep_ms(MQTT_RETRY_DELAY_MS);

                mqtt_attempt++;
            }
        }

        bool check_and_reconnect(const char *ssid, const char *password,
                                 const char *server_ip, uint16_t port, const char *client_id,
                                 void (*on_reconnect_callback)())
        {
            // Check MQTT connection first (more reliable than WiFi link status)
            // Note: WiFi link status can be momentarily down during normal operation,
            // so we only check WiFi if MQTT is actually failing.
            if (!g_mqtt_connected)
            {
                printf("MQTT connection lost! Reconnecting...\n");
                OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_MQTT_ERROR);

                g_mqtt_connected = false;

                int mqtt_attempt = 1;
                while (true)
                {
                    printf("MQTT reconnection attempt %d\n", mqtt_attempt);

                    // Check WiFi before attempting MQTT reconnect
                    int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
                    if (link_status != CYW43_LINK_UP)
                    {
                        printf("WiFi connection lost during MQTT reconnect! Reconnecting WiFi...\n");
                        OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_WIFI_ERROR);

                        // Reconnect both WiFi and MQTT
                        connect_with_retry(ssid, password, server_ip, port, client_id);

                        if (on_reconnect_callback)
                        {
                            on_reconnect_callback();
                        }

                        return true;
                    }

                    if (connect_mqtt(server_ip, port, client_id))
                    {
                        printf("MQTT reconnected!\n");

                        if (on_reconnect_callback)
                        {
                            on_reconnect_callback();
                        }

                        return true;
                    }

                    printf("MQTT reconnection failed, retrying in %d seconds...\n", MQTT_RETRY_DELAY_MS / 1000);
                    sleep_ms(MQTT_RETRY_DELAY_MS);

                    mqtt_attempt++;
                }
            }

            return true;
        }

    } // namespace Common
} // namespace OpenTherm
