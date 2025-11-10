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

        // MQTT callbacks
        void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
        {
            if (status == MQTT_CONNECT_ACCEPTED)
            {
                printf("MQTT connected!\n");
                g_mqtt_connected = true;
            }
            else
            {
                printf("MQTT connection failed: %d\n", status);
                g_mqtt_connected = false;
            }
        }

        void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
        {
            printf("Incoming publish at topic %s with total length %u\n", topic, (unsigned int)tot_len);
        }

        void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
        {
            static std::string current_topic;
            static std::string current_payload;

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

            u8_t qos = 0;
            u8_t retain_flag = retain ? 1 : 0;

            err_t err = mqtt_publish(g_mqtt_client, topic, payload, strlen(payload),
                                     qos, retain_flag, nullptr, nullptr);

            if (err != ERR_OK)
            {
                printf("MQTT publish failed: %d\n", err);
                return false;
            }

            return true;
        }

        bool mqtt_subscribe_wrapper(const char *topic)
        {
            if (!g_mqtt_connected || !g_mqtt_client)
            {
                return false;
            }

            err_t err = mqtt_subscribe(g_mqtt_client, topic, 0, mqtt_sub_request_cb, nullptr);

            if (err != ERR_OK)
            {
                printf("MQTT subscribe failed: %d\n", err);
                return false;
            }

            printf("Subscribed to: %s\n", topic);
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
