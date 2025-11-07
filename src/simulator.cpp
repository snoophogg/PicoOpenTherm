#include <cstdio>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "simulated_opentherm.hpp"
#include "opentherm_ha.hpp"
#include "config.hpp"
#include "mqtt_common.hpp"
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
    sleep_ms(3000);

    printf("\n=== PicoOpenTherm SIMULATOR Mode ===\n");
    printf("This firmware simulates OpenTherm data without hardware\n\n");

    if (cyw43_arch_init())
    {
        printf("Failed to initialize cyw43\n");
        while (true)
        {
            sleep_ms(1000);
        }
    }

    printf("Initializing configuration...\n");
    if (!Config::init())
    {
        printf("Failed to initialize configuration system\n");
        OpenTherm::Common::blink_error_fatal();
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

    // Connect to WiFi and MQTT
    cyw43_arch_enable_sta_mode();
    OpenTherm::Common::connect_with_retry(wifi_ssid, wifi_password, mqtt_server_ip, mqtt_server_port, mqtt_client_id);

    // Create simulated OpenTherm interface
    printf("Initializing OpenTherm Simulator...\n");
    OpenTherm::Simulator::SimulatedInterface sim_ot;

    // Configure Home Assistant interface
    OpenTherm::HomeAssistant::Config ha_config = {
        .device_name = device_name,
        .device_id = device_id,
        .mqtt_prefix = "homeassistant",
        .state_topic_base = "opentherm/state",
        .command_topic_base = "opentherm/cmd",
        .auto_discovery = true,
        .update_interval_ms = 10000};

    // Note: We can't use HAInterface directly as it expects OpenTherm::Interface
    // We need to create a wrapper or modify the interface
    // For now, we'll publish data manually following the HA pattern

    printf("Simulator ready! Publishing simulated data to Home Assistant...\n");

    uint32_t last_led_toggle = 0;
    uint32_t last_connection_check = 0;
    uint32_t last_update = 0;

    // Main loop
    while (true)
    {
        uint32_t now = to_ms_since_boot(get_absolute_time());

        // Blink LED for normal activity
        last_led_toggle = OpenTherm::Common::blink_check(OpenTherm::Common::LED_BLINK_NORMAL_COUNT, last_led_toggle);

        // Check connections
        if (now - last_connection_check >= OpenTherm::Common::CONNECTION_CHECK_DELAY_MS)
        {
            OpenTherm::Common::check_and_reconnect(wifi_ssid, wifi_password, mqtt_server_ip, mqtt_server_port,
                                                   mqtt_client_id, nullptr);
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
