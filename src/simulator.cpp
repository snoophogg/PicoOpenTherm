#include <cstdio>
#include <ctime>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/watchdog.h"
#include "simulated_opentherm.hpp"
#include "opentherm_ha.hpp"
#include "config.hpp"
#include "mqtt_common.hpp"
#include "mqtt_discovery.hpp"
#include "mqtt_topics.hpp"
#include "led_blink.hpp"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
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

    // Enable watchdog so the LED code can feed it; if the simulator gets stuck
    // in a continuous-blink fault mode, LED will stop feeding and allow reset.
    watchdog_enable(8000, false);

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
        if (now - last_update >= ha_cfg.update_interval_ms)
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

            // Time/Date - simulate current system time
            auto time_now = time(nullptr);
            struct tm *tm_now = localtime(&time_now);
            if (tm_now)
            {
                // Day of week (1=Monday, 7=Sunday)
                const char *day_names[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
                publishSensor(ha_cfg, DAY_OF_WEEK, day_names[tm_now->tm_wday]);

                // Time of day (HH:MM)
                snprintf(payload, sizeof(payload), "%02d:%02d", tm_now->tm_hour, tm_now->tm_min);
                publishSensor(ha_cfg, TIME_OF_DAY, payload);

                // Date (MM/DD)
                snprintf(payload, sizeof(payload), "%02d/%02d", tm_now->tm_mon + 1, tm_now->tm_mday);
                publishSensor(ha_cfg, DATE, payload);

                // Year
                publishSensor(ha_cfg, YEAR, tm_now->tm_year + 1900);
            }

            // Temperature bounds - simulate typical boiler limits
            publishSensor(ha_cfg, DHW_SETPOINT_MIN, 40);  // 40°C min DHW
            publishSensor(ha_cfg, DHW_SETPOINT_MAX, 65);  // 65°C max DHW
            publishSensor(ha_cfg, CH_SETPOINT_MIN, 30);   // 30°C min CH
            publishSensor(ha_cfg, CH_SETPOINT_MAX, 90);   // 90°C max CH

            // WiFi statistics - real data from CYW43 chip
            int32_t rssi = 0;
            if (cyw43_wifi_get_rssi(&cyw43_state, &rssi) == 0)
            {
                publishSensor(ha_cfg, WIFI_RSSI, (int)rssi);
            }

            // WiFi link status
            int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
            const char *status_str = "unknown";
            switch (link_status)
            {
            case CYW43_LINK_DOWN:
                status_str = "down";
                break;
            case CYW43_LINK_JOIN:
                status_str = "joining";
                break;
            case CYW43_LINK_NOIP:
                status_str = "no_ip";
                break;
            case CYW43_LINK_UP:
                status_str = "connected";
                break;
            case CYW43_LINK_FAIL:
                status_str = "failed";
                break;
            case CYW43_LINK_NONET:
                status_str = "no_network";
                break;
            case CYW43_LINK_BADAUTH:
                status_str = "bad_auth";
                break;
            }
            publishSensor(ha_cfg, WIFI_LINK_STATUS, status_str);

            // IP address
            if (netif_list)
            {
                const char *ip_addr = ip4addr_ntoa(netif_ip4_addr(netif_list));
                publishSensor(ha_cfg, IP_ADDRESS, ip_addr);
            }

            // WiFi SSID
            publishSensor(ha_cfg, WIFI_SSID, wifi_ssid);

            // Uptime in seconds
            uint64_t uptime_us = time_us_64();
            uint32_t uptime_seconds = (uint32_t)(uptime_us / 1000000ULL);
            publishSensor(ha_cfg, UPTIME, (int)uptime_seconds);

            // Free heap memory (placeholder)
            publishSensor(ha_cfg, FREE_HEAP, 0);

            printf("[SIM] T_room=%.1f T_boiler=%.1f Mod=%.0f%% Flame=%s RSSI=%ddBm\n",
                   sim_ot.readRoomTemperature(),
                   sim_ot.readBoilerTemperature(),
                   sim_ot.readModulationLevel(),
                   sim_ot.readFlameStatus() ? "ON" : "OFF",
                   (int)rssi);

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
