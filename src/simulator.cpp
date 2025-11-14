#include <cstdio>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "simulated_opentherm.hpp"
#include "opentherm_ha.hpp"
#include "config.hpp"
#include "mqtt_common.hpp"
#include "mqtt_discovery.hpp"
#include "mqtt_topics.hpp"
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

    // Publish Home Assistant discovery configurations using common library
    printf("Publishing Home Assistant discovery configurations...\n");

    // Build a temporary HomeAssistant::Config for discovery
    OpenTherm::HomeAssistant::Config ha_cfg;
    ha_cfg.device_name = device_name;
    ha_cfg.device_id = device_id;
    ha_cfg.mqtt_prefix = "homeassistant";
    // Simulator uses device-specific state topics like "opentherm/state/<device_id>/..."
    // so include device_id in the base state topic to preserve behavior.
    std::string state_base = std::string("opentherm/state/") + device_id;
    std::string cmd_base = std::string("opentherm/cmd/") + device_id;
    ha_cfg.state_topic_base = state_base.c_str();
    ha_cfg.command_topic_base = cmd_base.c_str();
    ha_cfg.auto_discovery = true;
    ha_cfg.update_interval_ms = 60000;

    if (!OpenTherm::Discovery::publishDiscoveryConfigs(ha_cfg))
    {
        printf("FATAL ERROR: Failed to publish discovery configurations after all retries\n");
        printf("Cannot continue without Home Assistant discovery - halting execution\n");
        OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_CONFIG_ERROR);
        while (true)
        {
            sleep_ms(1000); // Halt execution
        }
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

            using namespace OpenTherm::Discovery;
            using namespace OpenTherm::MQTTTopics;

            // Temperatures
            publishSensor(ha_cfg, ROOM_TEMP, sim_ot.readRoomTemperature());
            publishSensor(ha_cfg, BOILER_TEMP, sim_ot.readBoilerTemperature());
            publishSensor(ha_cfg, DHW_TEMP, sim_ot.readDHWTemperature());
            publishSensor(ha_cfg, RETURN_TEMP, sim_ot.readReturnWaterTemperature());
            publishSensor(ha_cfg, OUTSIDE_TEMP, sim_ot.readOutsideTemperature());

            // Modulation / pressure
            publishSensor(ha_cfg, MODULATION, sim_ot.readModulationLevel());
            publishSensor(ha_cfg, MAX_MODULATION, sim_ot.readMaxModulationLevel());
            publishSensor(ha_cfg, PRESSURE, sim_ot.readCHWaterPressure());

            // Binary/status sensors
            publishBinarySensor(ha_cfg, FLAME, sim_ot.readFlameStatus());
            publishBinarySensor(ha_cfg, CH_MODE, sim_ot.readCHActive());
            publishBinarySensor(ha_cfg, DHW_MODE, sim_ot.readDHWActive());
            publishBinarySensor(ha_cfg, CH_ENABLE, sim_ot.readCHEnabled());
            publishBinarySensor(ha_cfg, DHW_ENABLE, sim_ot.readDHWEnabled());
            publishBinarySensor(ha_cfg, COOLING, sim_ot.readCoolingEnabled());

            // Setpoints
            publishSensor(ha_cfg, CONTROL_SETPOINT, sim_ot.readRoomSetpoint());
            publishSensor(ha_cfg, ROOM_SETPOINT, sim_ot.readRoomSetpoint());
            publishSensor(ha_cfg, DHW_SETPOINT, sim_ot.readDHWSetpoint());
            publishSensor(ha_cfg, MAX_CH_SETPOINT, 90.0f);

            // Counters / statistics
            publishSensor(ha_cfg, BURNER_STARTS, (int)sim_ot.readBurnerStarts());
            publishSensor(ha_cfg, BURNER_HOURS, (int)sim_ot.readBurnerHours());
            publishSensor(ha_cfg, CH_PUMP_STARTS, 0);
            publishSensor(ha_cfg, DHW_PUMP_STARTS, 0);
            publishSensor(ha_cfg, CH_PUMP_HOURS, (int)sim_ot.readCHPumpHours());
            publishSensor(ha_cfg, DHW_PUMP_HOURS, (int)sim_ot.readDHWPumpHours());

            // Fault / diagnostic codes
            publishSensor(ha_cfg, FAULT_CODE, (int)sim_ot.readOEMFaultCode());
            publishSensor(ha_cfg, DIAGNOSTIC_CODE, (int)sim_ot.readOEMDiagnosticCode());

            // Device configuration and metadata
            publishSensor(ha_cfg, DEVICE_NAME, device_name);
            publishSensor(ha_cfg, DEVICE_ID, device_id);

            // OpenTherm version - simulator placeholder
            publishSensor(ha_cfg, OPENTHERM_VERSION, "1.0");

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
