#include "mqtt_common.hpp"
#include "led_blink.hpp"
#include "lwip/altcp.h"
#include "lwip/apps/mqtt_priv.h"
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
        // Note: Static lifetime is intentional - failure counter persists across
        // reconnects to detect chronic issues. Reset on successful connection.
        static int consecutive_publish_failures = 0;
        static constexpr int PUBLISH_FAILURE_THRESHOLD = 5;

        // Track publish statistics for long-term monitoring
        uint32_t g_total_publish_attempts = 0;
        uint32_t g_total_publish_failures = 0;
        uint32_t g_mqtt_reconnect_count = 0;

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
        // Note: These are reset with empty string assignment to prevent capacity leak
        static std::string current_topic;
        static std::string current_payload;

        // Maximum pending messages to prevent unbounded memory growth
        constexpr size_t MAX_PENDING_MESSAGES = 10;

        void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
        {
            printf("Incoming publish at topic %s with total length %u\n", topic, (unsigned int)tot_len);
            // Store the topic for the incoming data callback
            current_topic = topic;
            // Force deallocation by replacing with empty string to prevent capacity leak
            current_payload = std::string();
            current_payload.reserve(tot_len); // Pre-allocate exact size needed
        }

        void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
        {
            current_payload.append((const char *)data, len);

            if (flags & MQTT_DATA_FLAG_LAST)
            {
                printf("Received: %s = %s\n", current_topic.c_str(), current_payload.c_str());

                // Check if pending messages queue is full (prevents unbounded growth)
                if (g_pending_messages.size() >= MAX_PENDING_MESSAGES)
                {
                    // Drop oldest message (FIFO)
                    auto oldest = g_pending_messages.begin();
                    printf("WARNING: Pending message queue full (%zu msgs), dropping oldest: %s\n",
                           g_pending_messages.size(), oldest->first.c_str());
                    g_pending_messages.erase(oldest);
                }

                g_pending_messages[current_topic] = current_payload;

                // Force deallocation to prevent capacity leak from large messages
                current_payload = std::string();
                current_topic = std::string();
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

            // Track publish attempt
            g_total_publish_attempts++;

            // Check TCP send buffer availability before publishing
            // This prevents the pcb->snd_queuelen assertion failure by ensuring
            // we don't overwhelm the TCP stack with too much pending data
            size_t payload_len = strlen(payload);
            // Add 50% safety margin to ensure we have plenty of buffer space
            // This conservative approach prevents edge cases where exact size isn't enough
            size_t estimated_size = (payload_len + strlen(topic) + 20) * 3 / 2;

            // Wait for sufficient TCP buffer space to be available
            // The MQTT library uses altcp (abstraction layer) which wraps TCP
            if (g_mqtt_client && g_mqtt_client->conn)
            {
                uint32_t wait_start = to_ms_since_boot(get_absolute_time());
                const uint32_t max_wait_ms = 5000; // Wait up to 5 seconds for buffer space (increased from 2s)

                // Get available send buffer space from altcp connection
                u16_t snd_buf = altcp_sndbuf(g_mqtt_client->conn);

                // Log when buffer is getting tight (less than 25% of typical send buffer)
                if (snd_buf < 7300) // 25% of typical 29,200 byte buffer (20*TCP_MSS)
                {
                    printf("TCP buffer low: %u bytes available (need %zu)\n", snd_buf, estimated_size);
                }

                while (snd_buf < estimated_size)
                {
                    uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - wait_start;
                    if (elapsed >= max_wait_ms)
                    {
                        printf("TCP send buffer timeout (%u < %zu bytes after %lums)\n",
                               snd_buf, estimated_size, elapsed);
                        break;
                    }
                    // Poll network to allow ACKs to return and free buffers
                    aggressive_network_poll(50);
                    snd_buf = altcp_sndbuf(g_mqtt_client->conn);
                }
            }

            // Retry logic for memory errors - TCP buffers may be temporarily full
            const int max_retries = 3;
            err_t err = ERR_OK;

            for (int retry = 0; retry < max_retries; retry++)
            {
                err = mqtt_publish(g_mqtt_client, topic, payload, payload_len,
                                   qos, retain_flag, nullptr, nullptr);

                if (err == ERR_OK)
                {
                    break; // Success!
                }

                // If memory error, wait for TCP buffers to clear and retry
                if (err == ERR_MEM || err == ERR_BUF)
                {
                    // Get current buffer stats for diagnosis
                    u16_t current_snd_buf = 0;
                    if (g_mqtt_client && g_mqtt_client->conn)
                    {
                        current_snd_buf = altcp_sndbuf(g_mqtt_client->conn);
                    }

                    if (retry < max_retries - 1)
                    {
                        // Wait progressively longer: 200ms, 400ms, 600ms
                        // Longer waits allow more time for TCP ACKs to free buffers
                        // Increased from 100/200/300ms due to TCP buffer exhaustion at 90s
                        uint32_t wait_ms = 200 * (retry + 1);
                        printf("MQTT publish ERR_MEM (TCP snd_buf=%u, need=%zu), waiting %lums before retry %d/%d...\n",
                               current_snd_buf, estimated_size, wait_ms, retry + 1, max_retries);
                        aggressive_network_poll(wait_ms);
                        continue;
                    }
                    else
                    {
                        // Final retry failed - log detailed info
                        printf("MQTT publish ERR_MEM FAILED after %d retries (TCP snd_buf=%u, need=%zu, topic=%s, payload_len=%zu)\n",
                               max_retries, current_snd_buf, estimated_size, topic, payload_len);
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
                // Track failure
                g_total_publish_failures++;

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

            // Delay to allow TCP ACKs to return and free PBUFs
            // Even with TCP buffer space available, PBUFs must be freed by ACKs.
            // 50ms provides time for ACKs without excessive throttling (was 150ms, tried 20ms).
            aggressive_network_poll(50); // 50ms - balance between throughput and PBUF availability
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
                g_mqtt_reconnect_count++; // Track reconnection
                mqtt_disconnect(g_mqtt_client);

                // Clear pending messages before cleanup to prevent stale messages
                if (!g_pending_messages.empty())
                {
                    printf("Clearing %zu pending messages before reconnect\n", g_pending_messages.size());
                    g_pending_messages.clear();
                }

                mqtt_client_free(g_mqtt_client);
                g_mqtt_client = nullptr;
                g_mqtt_connected = false;

                // Increased delay for lwIP cleanup (was 100ms, now 500ms)
                // Allows lwIP to properly clean up TCP buffers and timers
                sleep_ms(500);
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
