#include <cstdio>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "opentherm.hpp"
#include "opentherm_ha.hpp"
#include "config.hpp"
#include "mqtt_common.hpp"
#include "led_blink.hpp"
#include <string>

// Global configuration buffers
static char wifi_ssid[64];
static char wifi_password[64];
static char mqtt_server_ip[64];
static uint16_t mqtt_server_port;
static char mqtt_client_id[64];
static char device_name[64];
static char device_id[64];
static uint8_t opentherm_tx_pin;
static uint8_t opentherm_rx_pin;

static OpenTherm::HomeAssistant::HAInterface *ha_interface = nullptr;

// Callback for reconnection - republish discovery
static void on_reconnect()
{
    if (ha_interface)
    {
        ha_interface->publishDiscoveryConfigs();
    }
}

int main()
{
    stdio_init_all();
    sleep_ms(3000); // Wait for USB serial

    printf("\n=== PicoOpenTherm Home Assistant Gateway ===\n");

    // Initialize WiFi and enable station mode
    printf("Initializing WiFi...\n");
    if (cyw43_arch_init())
    {
        printf("Failed to initialize cyw43\n");
        // Can't blink LED without cyw43, just hang
        while (true)
        {
            sleep_ms(1000);
        }
    }

    // Enable station mode before using LED
    cyw43_arch_enable_sta_mode();
    sleep_ms(100); // Give WiFi chip time to stabilize

    // Initialize LED state machine after WiFi is ready
    printf("Initializing LED state machine...\n");
    if (!OpenTherm::LED::init())
    {
        printf("Warning: Failed to initialize LED state machine\n");
    }

    // Initialize configuration system
    OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_CONFIG_ERROR);
    printf("Initializing configuration...\n");
    if (!Config::init())
    {
        printf("Failed to initialize configuration system\n");
        while (true)
        {
            sleep_ms(1000);
        }
    }

    // Load configuration from flash
    Config::getWiFiSSID(wifi_ssid, sizeof(wifi_ssid));
    Config::getWiFiPassword(wifi_password, sizeof(wifi_password));
    Config::getMQTTServerIP(mqtt_server_ip, sizeof(mqtt_server_ip));
    mqtt_server_port = Config::getMQTTServerPort();
    Config::getMQTTClientID(mqtt_client_id, sizeof(mqtt_client_id));
    Config::getDeviceName(device_name, sizeof(device_name));
    Config::getDeviceID(device_id, sizeof(device_id));
    opentherm_tx_pin = Config::getOpenThermTxPin();
    opentherm_rx_pin = Config::getOpenThermRxPin();

    // Print current configuration (hide password)
    Config::printConfig();

    // Connect to WiFi and MQTT with retry logic
    OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_WIFI_ERROR);
    OpenTherm::Common::connect_with_retry(wifi_ssid, wifi_password, mqtt_server_ip, mqtt_server_port, mqtt_client_id);

    // Set normal blink pattern after successful connection
    OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_NORMAL);

    // Initialize OpenTherm with configured pins
    printf("Initializing OpenTherm...\n");
    OpenTherm::Interface ot(opentherm_tx_pin, opentherm_rx_pin);

    // Configure Home Assistant interface using loaded configuration
    OpenTherm::HomeAssistant::Config ha_config = {
        .device_name = device_name,
        .device_id = device_id,
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
        .publish = OpenTherm::Common::mqtt_publish_wrapper,
        .subscribe = OpenTherm::Common::mqtt_subscribe_wrapper};

    // Initialize Home Assistant interface
    ha.begin(mqtt_callbacks);

    printf("System ready! Publishing to Home Assistant via MQTT...\n");

    uint32_t last_connection_check = 0;

    // Main loop
    while (true)
    {
        // Check connections periodically
        uint32_t now = to_ms_since_boot(get_absolute_time());

        if (now - last_connection_check >= OpenTherm::Common::CONNECTION_CHECK_DELAY_MS)
        {
            OpenTherm::Common::check_and_reconnect(wifi_ssid, wifi_password, mqtt_server_ip, mqtt_server_port,
                                                   mqtt_client_id, on_reconnect);
            last_connection_check = now;
        }

        // Process any pending MQTT messages
        if (!OpenTherm::Common::g_pending_messages.empty())
        {
            for (auto &msg : OpenTherm::Common::g_pending_messages)
            {
                ha.handleMessage(msg.first.c_str(), msg.second.c_str());
            }
            OpenTherm::Common::g_pending_messages.clear();
        }

        // Update Home Assistant (reads sensors and publishes to MQTT)
        ha.update();

        // Small delay
        sleep_ms(100);
    }

    return 0;
}
