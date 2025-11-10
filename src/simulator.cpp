#include <cstdio>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "simulated_opentherm.hpp"
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

int main()
{
    stdio_init_all();
    sleep_ms(3000); // Wait for USB serial

    // Longer pause to allow UART connection for debugging
    printf("\n");
    printf("Waiting for UART connection...\n");
    for (int i = 5; i > 0; i--)
    {
        printf("%d...\n", i);
        sleep_ms(1000);
    }
    printf("\n");

    printf("\n=== PicoOpenTherm SIMULATOR Mode ===\n");
    printf("This firmware simulates OpenTherm data without hardware\n\n");

    // Initialize WiFi and enable station mode
    printf("Initializing WiFi chip...\n");
    if (cyw43_arch_init())
    {
        printf("Failed to initialize cyw43\n");
        while (true)
        {
            sleep_ms(1000);
        }
    }

    // Enable station mode and give chip extra time to stabilize
    printf("Enabling WiFi station mode...\n");
    cyw43_arch_enable_sta_mode();
    sleep_ms(500); // Increased delay to allow CYW43 chip to fully stabilize

    // Initialize LED state machine after WiFi is ready
    printf("Starting LED blink timer...\n");
    if (!OpenTherm::LED::init())
    {
        printf("Warning: Failed to initialize LED blink timer\n");
    }

    printf("Initializing configuration...\n");
    OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_CONFIG_ERROR);
    if (!Config::init())
    {
        printf("Failed to initialize configuration system\n");
        while (true)
        {
            sleep_ms(1000);
        }
    }

    // Load configuration
    Config::getWiFiSSID(wifi_ssid, sizeof(wifi_ssid));
    Config::getWiFiPassword(wifi_password, sizeof(wifi_password));
    Config::getMQTTServerIP(mqtt_server_ip, sizeof(mqtt_server_ip));
    mqtt_server_port = Config::getMQTTServerPort();
    Config::getMQTTClientID(mqtt_client_id, sizeof(mqtt_client_id));
    Config::getDeviceName(device_name, sizeof(device_name));
    Config::getDeviceID(device_id, sizeof(device_id));

    Config::printConfig();

    printf("\n=== Connection Settings ===\n");
    printf("WiFi SSID:        %s\n", wifi_ssid);
    printf("WiFi Password:    %s\n", strlen(wifi_password) > 0 ? "********" : "(not set)");
    printf("MQTT Server:      %s:%d\n", mqtt_server_ip, mqtt_server_port);
    printf("MQTT Client ID:   %s\n", mqtt_client_id);
    printf("Device Name:      %s\n", device_name);
    printf("Device ID:        %s\n", device_id);
    printf("===========================\n\n");

    // Connect to WiFi and MQTT (station mode already enabled)
    // Set pattern to WiFi error while attempting connection
    OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_WIFI_ERROR);

    OpenTherm::Common::connect_with_retry(wifi_ssid, wifi_password, mqtt_server_ip, mqtt_server_port, mqtt_client_id);

    // Connection successful - set to normal blink pattern
    OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_NORMAL);

    // Create simulated OpenTherm interface
    printf("Initializing OpenTherm Simulator...\n");
    OpenTherm::Simulator::SimulatedInterface sim_ot;

    // Wait additional time before publishing discovery configs
    // Discovery messages are large and need extra buffer availability
    printf("Waiting for MQTT client to be ready for discovery...\n");
    sleep_ms(1000);

    printf("Publishing Home Assistant discovery configurations...\n");
    
    // Publish discovery configs for simulated sensors
    char topic[256];
    char config[512];
    
    // Room temperature sensor
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/room_temperature/config", device_id);
    snprintf(config, sizeof(config), 
        "{\"name\":\"%s Room Temperature\",\"unique_id\":\"%s_room_temp\","
        "\"state_topic\":\"opentherm/state/%s/room_temperature\","
        "\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\","
        "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\",\"manufacturer\":\"PicoOpenTherm\",\"model\":\"Simulator\"}}",
        device_name, device_id, device_id, device_id, device_name);
    if (!OpenTherm::Common::mqtt_publish_wrapper(topic, config, true))
    {
        printf("Failed to publish room temperature discovery - retrying in 1s...\n");
        sleep_ms(1000);
        OpenTherm::Common::mqtt_publish_wrapper(topic, config, true);
    }
    
    // Boiler temperature sensor
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/boiler_temperature/config", device_id);
    snprintf(config, sizeof(config),
        "{\"name\":\"%s Boiler Temperature\",\"unique_id\":\"%s_boiler_temp\","
        "\"state_topic\":\"opentherm/state/%s/boiler_temperature\","
        "\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\","
        "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\",\"manufacturer\":\"PicoOpenTherm\",\"model\":\"Simulator\"}}",
        device_name, device_id, device_id, device_id, device_name);
    if (!OpenTherm::Common::mqtt_publish_wrapper(topic, config, true))
    {
        printf("Failed to publish boiler temperature discovery - retrying in 1s...\n");
        sleep_ms(1000);
        OpenTherm::Common::mqtt_publish_wrapper(topic, config, true);
    }
    
    // DHW temperature sensor
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/dhw_temperature/config", device_id);
    snprintf(config, sizeof(config),
        "{\"name\":\"%s DHW Temperature\",\"unique_id\":\"%s_dhw_temp\","
        "\"state_topic\":\"opentherm/state/%s/dhw_temperature\","
        "\"unit_of_measurement\":\"°C\",\"device_class\":\"temperature\","
        "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\",\"manufacturer\":\"PicoOpenTherm\",\"model\":\"Simulator\"}}",
        device_name, device_id, device_id, device_id, device_name);
    if (!OpenTherm::Common::mqtt_publish_wrapper(topic, config, true))
    {
        printf("Failed to publish DHW temperature discovery - retrying in 1s...\n");
        sleep_ms(1000);
        OpenTherm::Common::mqtt_publish_wrapper(topic, config, true);
    }
    
    // Modulation sensor
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/modulation/config", device_id);
    snprintf(config, sizeof(config),
        "{\"name\":\"%s Modulation\",\"unique_id\":\"%s_modulation\","
        "\"state_topic\":\"opentherm/state/%s/modulation\","
        "\"unit_of_measurement\":\"%%\","
        "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\",\"manufacturer\":\"PicoOpenTherm\",\"model\":\"Simulator\"}}",
        device_name, device_id, device_id, device_id, device_name);
    if (!OpenTherm::Common::mqtt_publish_wrapper(topic, config, true))
    {
        printf("Failed to publish modulation discovery - retrying in 1s...\n");
        sleep_ms(1000);
        OpenTherm::Common::mqtt_publish_wrapper(topic, config, true);
    }
    
    // Pressure sensor
    snprintf(topic, sizeof(topic), "homeassistant/sensor/%s/pressure/config", device_id);
    snprintf(config, sizeof(config),
        "{\"name\":\"%s CH Pressure\",\"unique_id\":\"%s_pressure\","
        "\"state_topic\":\"opentherm/state/%s/pressure\","
        "\"unit_of_measurement\":\"bar\",\"device_class\":\"pressure\","
        "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\",\"manufacturer\":\"PicoOpenTherm\",\"model\":\"Simulator\"}}",
        device_name, device_id, device_id, device_id, device_name);
    if (!OpenTherm::Common::mqtt_publish_wrapper(topic, config, true))
    {
        printf("Failed to publish pressure discovery - retrying in 1s...\n");
        sleep_ms(1000);
        OpenTherm::Common::mqtt_publish_wrapper(topic, config, true);
    }
    
    // Flame status binary sensor
    snprintf(topic, sizeof(topic), "homeassistant/binary_sensor/%s/flame/config", device_id);
    snprintf(config, sizeof(config),
        "{\"name\":\"%s Flame Status\",\"unique_id\":\"%s_flame\","
        "\"state_topic\":\"opentherm/state/%s/flame\","
        "\"payload_on\":\"ON\",\"payload_off\":\"OFF\","
        "\"device\":{\"identifiers\":[\"%s\"],\"name\":\"%s\",\"manufacturer\":\"PicoOpenTherm\",\"model\":\"Simulator\"}}",
        device_name, device_id, device_id, device_id, device_name);
    if (!OpenTherm::Common::mqtt_publish_wrapper(topic, config, true))
    {
        printf("Failed to publish flame discovery - retrying in 1s...\n");
        sleep_ms(1000);
        OpenTherm::Common::mqtt_publish_wrapper(topic, config, true);
    }

    printf("Discovery configuration complete!\n");
    printf("Simulator ready! Publishing simulated data to Home Assistant...\n");

    uint32_t last_connection_check = 0;
    uint32_t last_update = 0;

    // Main loop
    while (true)
    {
        uint32_t now = to_ms_since_boot(get_absolute_time());

        // LED blinking is now handled asynchronously by timer

        // Check connections
        if (now - last_connection_check >= OpenTherm::Common::CONNECTION_CHECK_DELAY_MS)
        {
            bool was_connected = OpenTherm::Common::g_mqtt_connected;

            OpenTherm::Common::check_and_reconnect(wifi_ssid, wifi_password, mqtt_server_ip, mqtt_server_port,
                                                   mqtt_client_id, nullptr);

            // Update LED pattern based on connection status
            if (OpenTherm::Common::g_mqtt_connected && !was_connected)
            {
                // Just reconnected - set to normal pattern
                OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_NORMAL);
            }

            last_connection_check = now;
        }

        // Update simulator and publish data
        if (now - last_update >= 10000) // Every 10 seconds
        {
            sim_ot.update();

            // Publish simulated sensor data
            char topic[128];
            char payload[64];

            // Room temperature
            snprintf(topic, sizeof(topic), "opentherm/state/%s/room_temperature", device_id);
            snprintf(payload, sizeof(payload), "%.2f", sim_ot.readRoomTemperature());
            OpenTherm::Common::mqtt_publish_wrapper(topic, payload, false);

            // Boiler temperature
            snprintf(topic, sizeof(topic), "opentherm/state/%s/boiler_temperature", device_id);
            snprintf(payload, sizeof(payload), "%.2f", sim_ot.readBoilerTemperature());
            OpenTherm::Common::mqtt_publish_wrapper(topic, payload, false);

            // DHW temperature
            snprintf(topic, sizeof(topic), "opentherm/state/%s/dhw_temperature", device_id);
            snprintf(payload, sizeof(payload), "%.2f", sim_ot.readDHWTemperature());
            OpenTherm::Common::mqtt_publish_wrapper(topic, payload, false);

            // Modulation
            snprintf(topic, sizeof(topic), "opentherm/state/%s/modulation", device_id);
            snprintf(payload, sizeof(payload), "%.1f", sim_ot.readModulationLevel());
            OpenTherm::Common::mqtt_publish_wrapper(topic, payload, false);

            // Pressure
            snprintf(topic, sizeof(topic), "opentherm/state/%s/pressure", device_id);
            snprintf(payload, sizeof(payload), "%.2f", sim_ot.readCHWaterPressure());
            OpenTherm::Common::mqtt_publish_wrapper(topic, payload, false);

            // Flame status
            snprintf(topic, sizeof(topic), "opentherm/state/%s/flame", device_id);
            snprintf(payload, sizeof(payload), "%s", sim_ot.readFlameStatus() ? "ON" : "OFF");
            OpenTherm::Common::mqtt_publish_wrapper(topic, payload, false);

            printf("[SIM] T_room=%.1f T_boiler=%.1f Mod=%.0f%% Flame=%s\n",
                   sim_ot.readRoomTemperature(),
                   sim_ot.readBoilerTemperature(),
                   sim_ot.readModulationLevel(),
                   sim_ot.readFlameStatus() ? "ON" : "OFF");

            last_update = now;
        }

        // Process incoming MQTT messages
        if (!OpenTherm::Common::g_pending_messages.empty())
        {
            for (auto &msg : OpenTherm::Common::g_pending_messages)
            {
                // Handle setpoint changes
                if (msg.first.find("/room_setpoint") != std::string::npos)
                {
                    float setpoint = std::stof(msg.second);
                    sim_ot.writeRoomSetpoint(setpoint);
                }
                else if (msg.first.find("/dhw_setpoint") != std::string::npos)
                {
                    float setpoint = std::stof(msg.second);
                    sim_ot.writeDHWSetpoint(setpoint);
                }
            }
            OpenTherm::Common::g_pending_messages.clear();
        }

        sleep_ms(100);
    }

    return 0;
}
