#include "mqtt_common.hpp"
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
                return false;
            }

            err_t err = mqtt_client_connect(g_mqtt_client, &mqtt_server, port,
                                            mqtt_connection_cb, nullptr, &ci);

            if (err != ERR_OK)
            {
                printf("MQTT connect failed: %d\n", err);
                return false;
            }

            // Wait for connection
            for (int i = 0; i < 50 && !g_mqtt_connected; i++)
            {
                sleep_ms(100);
            }

            return g_mqtt_connected;
        }

        // LED blink functions
        void blink_led_pattern(uint8_t blink_count)
        {
            for (uint8_t i = 0; i < blink_count; i++)
            {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                sleep_ms(LED_BLINK_DURATION);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                sleep_ms(LED_BLINK_DURATION);
            }
            sleep_ms(LED_BLINK_PAUSE);
        }

        uint32_t blink_check(uint8_t blink_count, uint32_t last_led_toggle)
        {
            uint32_t now = to_ms_since_boot(get_absolute_time());
            if (now - last_led_toggle >= LED_BLINK_PAUSE)
            {
                blink_led_pattern(blink_count);
                return now;
            }
            return last_led_toggle;
        }

        [[noreturn]] void blink_error_fatal()
        {
            printf("Fatal error - blinking continuously\n");
            while (true)
            {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                sleep_ms(100);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                sleep_ms(100);
            }
        }

        bool connect_with_retry(const char *ssid, const char *password,
                                const char *server_ip, uint16_t port, const char *client_id)
        {
            uint32_t last_led_toggle = 0;

            // Connect to WiFi
            int wifi_attempt = 1;
            while (true)
            {
                printf("WiFi connection attempt %d\n", wifi_attempt);

                if (connect_wifi(ssid, password))
                {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                    break;
                }

                printf("WiFi connection failed, retrying in %d seconds...\n", WIFI_RETRY_DELAY_MS / 1000);

                uint32_t delay_start = to_ms_since_boot(get_absolute_time());
                while ((to_ms_since_boot(get_absolute_time()) - delay_start) < WIFI_RETRY_DELAY_MS)
                {
                    last_led_toggle = blink_check(LED_BLINK_WIFI_ERROR_COUNT, last_led_toggle);
                }

                wifi_attempt++;
            }

            // Connect to MQTT
            g_mqtt_connected = false;

            if (g_mqtt_client)
            {
                mqtt_client_free(g_mqtt_client);
                g_mqtt_client = nullptr;
            }

            int mqtt_attempt = 1;
            while (true)
            {
                printf("MQTT connection attempt %d\n", mqtt_attempt);

                if (connect_mqtt(server_ip, port, client_id))
                {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                    return true;
                }

                printf("MQTT connection failed, retrying in %d seconds...\n", MQTT_RETRY_DELAY_MS / 1000);

                uint32_t delay_start = to_ms_since_boot(get_absolute_time());
                while ((to_ms_since_boot(get_absolute_time()) - delay_start) < MQTT_RETRY_DELAY_MS)
                {
                    last_led_toggle = blink_check(LED_BLINK_MQTT_ERROR_COUNT, last_led_toggle);
                }

                mqtt_attempt++;
            }
        }

        bool check_and_reconnect(const char *ssid, const char *password,
                                 const char *server_ip, uint16_t port, const char *client_id,
                                 void (*on_reconnect_callback)())
        {
            // Check WiFi connection
            int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
            if (link_status != CYW43_LINK_UP)
            {
                printf("WiFi connection lost! Reconnecting...\n");
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

                connect_with_retry(ssid, password, server_ip, port, client_id);

                if (on_reconnect_callback)
                {
                    on_reconnect_callback();
                }

                return true;
            }

            // Check MQTT connection
            if (!g_mqtt_connected)
            {
                printf("MQTT connection lost! Reconnecting...\n");

                g_mqtt_connected = false;

                if (g_mqtt_client)
                {
                    mqtt_client_free(g_mqtt_client);
                    g_mqtt_client = nullptr;
                }

                int mqtt_attempt = 1;
                while (true)
                {
                    printf("MQTT reconnection attempt %d\n", mqtt_attempt);

                    if (connect_mqtt(server_ip, port, client_id))
                    {
                        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

                        if (on_reconnect_callback)
                        {
                            on_reconnect_callback();
                        }

                        return true;
                    }

                    printf("MQTT reconnection failed, retrying in %d seconds...\n", MQTT_RETRY_DELAY_MS / 1000);

                    uint32_t delay_start = to_ms_since_boot(get_absolute_time());
                    while ((to_ms_since_boot(get_absolute_time()) - delay_start) < MQTT_RETRY_DELAY_MS)
                    {
                        blink_led_pattern(LED_BLINK_MQTT_ERROR_COUNT);
                    }

                    mqtt_attempt++;
                }
            }

            return true;
        }

    } // namespace Common
} // namespace OpenTherm
