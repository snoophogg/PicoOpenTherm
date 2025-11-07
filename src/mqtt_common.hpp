#ifndef MQTT_COMMON_HPP
#define MQTT_COMMON_HPP

#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/apps/mqtt.h"
#include <string>
#include <map>

namespace OpenTherm
{
    namespace Common
    {
        // Connection retry settings
        constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 30000;
        constexpr uint32_t WIFI_RETRY_DELAY_MS = 5000;
        constexpr uint32_t MQTT_RETRY_DELAY_MS = 3000;
        constexpr uint32_t CONNECTION_CHECK_DELAY_MS = 5000;

        // Global MQTT state
        extern mqtt_client_t *g_mqtt_client;
        extern bool g_mqtt_connected;
        extern std::map<std::string, std::string> g_pending_messages;

        // MQTT callback functions
        void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status);
        void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len);
        void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags);
        void mqtt_sub_request_cb(void *arg, err_t result);

        // MQTT wrapper functions
        bool mqtt_publish_wrapper(const char *topic, const char *payload, bool retain);
        bool mqtt_subscribe_wrapper(const char *topic);

        // Connection functions
        bool connect_wifi(const char *ssid, const char *password);
        bool connect_mqtt(const char *server_ip, uint16_t port, const char *client_id);
        bool connect_with_retry(const char *ssid, const char *password,
                                const char *server_ip, uint16_t port, const char *client_id);
        bool check_and_reconnect(const char *ssid, const char *password,
                                 const char *server_ip, uint16_t port, const char *client_id,
                                 void (*on_reconnect_callback)());

    } // namespace Common
} // namespace OpenTherm

#endif // MQTT_COMMON_HPP
