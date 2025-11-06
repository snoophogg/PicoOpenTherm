#include <cstdio>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "opentherm.hpp"
#include "opentherm_ha.hpp"
#include "lwip/apps/mqtt.h"
#include <string>
#include <map>

// TODO : Read these from a secrets file
// WiFi credentials - update these!
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// TODO : Read these from a secrets file
// MQTT broker settings - update these!
#define MQTT_SERVER_IP "192.168.1.100" // Your MQTT broker IP
#define MQTT_SERVER_PORT 1883

// OpenTherm pins
#define OPENTHERM_TX_PIN 16
#define OPENTHERM_RX_PIN 17

// Connection retry settings
// TODO : Expose these as HA configuration options
#define WIFI_CONNECT_TIMEOUT_MS 30000
#define WIFI_RETRY_DELAY_MS 5000
#define MQTT_RETRY_DELAY_MS 3000
#define CONNECTION_CHECK_DELAY_MS 5000

// LED blink patterns
#define LED_BLINK_DURATION 200       // Duration of each blink (on/off cycle)
#define LED_BLINK_PAUSE 1000         // Pause between blink patterns
#define LED_BLINK_WIFI_ERROR_COUNT 2 // 2 blinks for WiFi error
#define LED_BLINK_MQTT_ERROR_COUNT 3 // 3 blinks for MQTT error
#define LED_BLINK_NORMAL_COUNT 1     // 1 blink for normal operation

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
                                           CYW43_AUTH_WPA2_AES_PSK, WIFI_CONNECT_TIMEOUT_MS))
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

// Blink LED with specified count (e.g., 2 blinks for WiFi error, 3 for MQTT error)
void blink_led_pattern(uint8_t blink_count)
{
    for (uint8_t i = 0; i < blink_count; i++)
    {
        // LED on
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(LED_BLINK_DURATION);

        // LED off
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(LED_BLINK_DURATION);
    }

    // Pause between patterns
    sleep_ms(LED_BLINK_PAUSE);
}

// Connect to WiFi and MQTT with retry logic and LED indication
bool connect_with_retry()
{
    // First, connect to WiFi
    int wifi_attempt = 1;
    while (true)
    {
        printf("WiFi connection attempt %d\n", wifi_attempt);

        if (connect_wifi())
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            // TODO : Add connection counter and timestamp, and expose to HA
            break; // WiFi connected, proceed to MQTT
        }

        // Failed - blink WiFi error pattern (2 blinks) periodically while waiting
        printf("WiFi connection failed, retrying in %d seconds...\n", WIFI_RETRY_DELAY_MS / 1000);

        // Blink error pattern for retry delay duration
        // TODO : Move LED blinking logic to separate function
        uint32_t delay_start = to_ms_since_boot(get_absolute_time());
        while ((to_ms_since_boot(get_absolute_time()) - delay_start) < WIFI_RETRY_DELAY_MS)
        {
            blink_led_pattern(LED_BLINK_WIFI_ERROR_COUNT);
        }

        wifi_attempt++;
    }

    // Now connect to MQTT
    // Reset MQTT connection flag
    mqtt_connected = false;

    // Clean up any existing client
    if (mqtt_client)
    {
        mqtt_client_free(mqtt_client);
        mqtt_client = nullptr;
    }

    int mqtt_attempt = 1;
    while (true)
    {
        printf("MQTT connection attempt %d\n", mqtt_attempt);

        if (connect_mqtt())
        {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            // TODO : Add connection counter and timestamp, and expose to HA
            return true; // Both WiFi and MQTT connected successfully
        }

        // Failed - blink MQTT error pattern (3 blinks) periodically while waiting
        printf("MQTT connection failed, retrying in %d seconds...\n", MQTT_RETRY_DELAY_MS / 1000);

        // Blink error pattern for retry delay duration
        // TODO : Move LED blinking logic to separate function
        uint32_t delay_start = to_ms_since_boot(get_absolute_time());
        while ((to_ms_since_boot(get_absolute_time()) - delay_start) < MQTT_RETRY_DELAY_MS)
        {
            blink_led_pattern(LED_BLINK_MQTT_ERROR_COUNT);
        }

        mqtt_attempt++;
    }
}

// Check and maintain connections
bool check_and_reconnect()
{
    // Check WiFi connection
    int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
    if (link_status != CYW43_LINK_UP)
    {
        // TODO : Put OT into failsafe mode
        printf("WiFi connection lost! Reconnecting...\n");
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

        // Reconnect WiFi and MQTT
        connect_with_retry();

        // Trigger discovery again after reconnection
        if (ha_interface)
        {
            ha_interface->publishDiscoveryConfigs();
        }

        return true;
    }

    // TODO : Check wifi connection strength
    // TODO : Expose wifi signal strength to HA

    // Check MQTT connection
    if (!mqtt_connected)
    {
        // TODO : Put OT into failsafe mode
        printf("MQTT connection lost! Reconnecting...\n");

        // Reconnect MQTT (WiFi already up)
        // Reset MQTT connection flag
        mqtt_connected = false;

        // Clean up any existing client
        if (mqtt_client)
        {
            mqtt_client_free(mqtt_client);
            mqtt_client = nullptr;
        }

        int mqtt_attempt = 1;
        while (true)
        {
            printf("MQTT reconnection attempt %d\n", mqtt_attempt);

            if (connect_mqtt())
            {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);

                // Trigger discovery again after reconnection
                if (ha_interface)
                {
                    ha_interface->publishDiscoveryConfigs();
                }

                return true;
            }

            // Failed - blink MQTT error pattern (3 blinks) periodically while waiting
            printf("MQTT reconnection failed, retrying in %d seconds...\n", MQTT_RETRY_DELAY_MS / 1000);

            // TODO : use common blinking function
            // Blink error pattern for retry delay duration
            uint32_t delay_start = to_ms_since_boot(get_absolute_time());
            while ((to_ms_since_boot(get_absolute_time()) - delay_start) < MQTT_RETRY_DELAY_MS)
            {
                blink_led_pattern(LED_BLINK_MQTT_ERROR_COUNT);
            }

            mqtt_attempt++;
        }
    }

    return true; // Both connections OK
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

    // Connect to WiFi and MQTT with retry logic
    connect_with_retry(); // Will retry until both are connected

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
    uint32_t last_connection_check = 0;

    // Main loop
    while (true)
    {
        // Check connections periodically
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now - last_connection_check >= CONNECTION_CHECK_DELAY_MS)
        {
            check_and_reconnect(); // Will retry until reconnected
            last_connection_check = now;
        }

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
        ha.update();

        // TODO : Move LED blinking logic to separate function
        // Blink LED to show normal activity (1 blink pattern with pause)
        if (now - last_led_toggle >= (LED_BLINK_DURATION * 2 + LED_BLINK_PAUSE))
        {
            // Single blink for normal operation
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_ms(LED_BLINK_DURATION);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            last_led_toggle = now;
        }

        // Small delay
        sleep_ms(100);
    }

    return 0;
}
