#include <cstdio>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "opentherm.hpp"
#include "opentherm_ha.hpp"
#include "lwip/apps/mqtt.h"
#include <string>
#include <map>

// WiFi credentials - update these!
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// MQTT broker settings - update these!
#define MQTT_SERVER_IP "192.168.1.100" // Your MQTT broker IP
#define MQTT_SERVER_PORT 1883

// OpenTherm pins
#define OPENTHERM_TX_PIN 16
#define OPENTHERM_RX_PIN 17

// Global MQTT client
static mqtt_client_t *mqtt_client = nullptr;
static bool mqtt_connected = false;
static OpenTherm::HomeAssistant::HAInterface *ha_interface = nullptr;

// Message queue for incoming MQTT messages
struct MQTTMessage
{
    std::string topic;
    std::string payload;
};
static std::map<std::string, std::string> pending_messages;

// MQTT callbacks
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    if (status == MQTT_CONNECT_ACCEPTED)
    {
        printf("MQTT connected!\n");
        mqtt_connected = true;

        // Trigger Home Assistant discovery
        if (ha_interface)
        {
            ha_interface->publishDiscoveryConfigs();
        }
    }
    else
    {
        printf("MQTT connection failed: %d\n", status);
        mqtt_connected = false;
    }
}

static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    printf("Incoming publish at topic %s with total length %u\n", topic, (unsigned int)tot_len);

    // Store topic for data callback
    static std::string current_topic;
    current_topic = topic;
}

static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    // Get topic from previous callback
    static std::string current_topic;
    static std::string current_payload;

    // Accumulate payload
    current_payload.append((const char *)data, len);

    // If this is the last chunk
    if (flags & MQTT_DATA_FLAG_LAST)
    {
        printf("Received: %s = %s\n", current_topic.c_str(), current_payload.c_str());

        // Store message for processing in main loop
        pending_messages[current_topic] = current_payload;

        current_payload.clear();
    }
}

static void mqtt_sub_request_cb(void *arg, err_t result)
{
    printf("Subscribe result: %d\n", result);
}

// MQTT publish function for Home Assistant interface
static bool mqtt_publish_wrapper(const char *topic, const char *payload, bool retain)
{
    if (!mqtt_connected || !mqtt_client)
    {
        return false;
    }

    u8_t qos = 0;
    u8_t retain_flag = retain ? 1 : 0;

    err_t err = mqtt_publish(mqtt_client, topic, payload, strlen(payload),
                             qos, retain_flag, nullptr, nullptr);

    if (err != ERR_OK)
    {
        printf("MQTT publish failed: %d\n", err);
        return false;
    }

    return true;
}

// MQTT subscribe function for Home Assistant interface
static bool mqtt_subscribe_wrapper(const char *topic)
{
    if (!mqtt_connected || !mqtt_client)
    {
        return false;
    }

    err_t err = mqtt_subscribe(mqtt_client, topic, 0, mqtt_sub_request_cb, nullptr);

    if (err != ERR_OK)
    {
        printf("MQTT subscribe failed: %d\n", err);
        return false;
    }

    printf("Subscribed to: %s\n", topic);
    return true;
}

// Connect to WiFi
bool connect_wifi()
{
    printf("Connecting to WiFi...\n");

    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD,
                                           CYW43_AUTH_WPA2_AES_PSK, 30000))
    {
        printf("Failed to connect to WiFi\n");
        return false;
    }

    printf("Connected to WiFi!\n");
    printf("IP Address: %s\n", ip4addr_ntoa(netif_ip4_addr(netif_list)));
    return true;
}

// Connect to MQTT broker
bool connect_mqtt()
{
    printf("Connecting to MQTT broker...\n");

    mqtt_client = mqtt_client_new();
    if (!mqtt_client)
    {
        printf("Failed to create MQTT client\n");
        return false;
    }

    struct mqtt_connect_client_info_t ci = {0};
    ci.client_id = "pico_opentherm";
    ci.keep_alive = 60;

    mqtt_set_inpub_callback(mqtt_client, mqtt_incoming_publish_cb,
                            mqtt_incoming_data_cb, nullptr);

    ip_addr_t mqtt_server;
    if (!ipaddr_aton(MQTT_SERVER_IP, &mqtt_server))
    {
        printf("Invalid MQTT server IP\n");
        return false;
    }

    err_t err = mqtt_client_connect(mqtt_client, &mqtt_server, MQTT_SERVER_PORT,
                                    mqtt_connection_cb, nullptr, &ci);

    if (err != ERR_OK)
    {
        printf("MQTT connect failed: %d\n", err);
        return false;
    }

    // Wait for connection
    for (int i = 0; i < 50 && !mqtt_connected; i++)
    {
        sleep_ms(100);
    }

    return mqtt_connected;
}

int main()
{
    stdio_init_all();
    sleep_ms(3000); // Wait for USB serial

    printf("\n=== PicoOpenTherm Home Assistant Gateway ===\n");

    // Initialize WiFi
    if (cyw43_arch_init())
    {
        printf("Failed to initialize cyw43\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    // Connect to WiFi
    if (!connect_wifi())
    {
        return 1; // TODO retry logic and blink error LED
    }

    // Connect to MQTT
    if (!connect_mqtt())
    {
        return 1; // TODO retry logic and blink error LED
    }

    // Initialize OpenTherm
    printf("Initializing OpenTherm...\n");
    OpenTherm::Interface ot(OPENTHERM_TX_PIN, OPENTHERM_RX_PIN);

    // Configure Home Assistant interface
    OpenTherm::HomeAssistant::Config ha_config = {
        .device_name = "OpenTherm Gateway",
        .device_id = "opentherm_gw",
        .mqtt_prefix = "homeassistant",
        .state_topic_base = "opentherm/state",
        .command_topic_base = "opentherm/cmd",
        .auto_discovery = true,
        .update_interval_ms = 10000 // Update every 10 seconds
    };

    OpenTherm::HomeAssistant::HAInterface ha(ot, ha_config);
    ha_interface = &ha;

    // Set up MQTT callbacks
    OpenTherm::HomeAssistant::MQTTCallbacks mqtt_callbacks = {
        .publish = mqtt_publish_wrapper,
        .subscribe = mqtt_subscribe_wrapper};

    // Initialize Home Assistant interface
    ha.begin(mqtt_callbacks);

    printf("System ready! Publishing to Home Assistant via MQTT...\n");

    uint32_t last_led_toggle = 0;
    bool led_state = false;

    // Main loop
    while (true)
    {
        // Process any pending MQTT messages
        if (!pending_messages.empty())
        {
            for (auto &msg : pending_messages)
            {
                ha.handleMessage(msg.first.c_str(), msg.second.c_str());
            }
            pending_messages.clear();
        }

        // Update Home Assistant (reads sensors and publishes to MQTT)
        ha.update(); // TODO errorhandling and restarting WiFi/MQTT if needed

        // Toggle LED to show activity
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_led_toggle >= 1000)
        {
            led_state = !led_state;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, led_state);
            last_led_toggle = now;
        }

        // Small delay
        sleep_ms(100);
    }

    return 0;
}
