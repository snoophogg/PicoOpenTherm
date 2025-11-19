#include "opentherm_ha.hpp"
#include "opentherm_protocol.hpp"
#include "config.hpp"
#include "mqtt_discovery.hpp"
#include "mqtt_topics.hpp"
#include "pico/cyw43_arch.h"
#include <cstdio>
#include <cstring>
#include <sstream>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"
#include "led_blink.hpp"

namespace OpenTherm
{
    namespace HomeAssistant
    {

        HAInterface::HAInterface(OpenTherm::BaseInterface &ot_interface, const Config &config)
            : ot_(ot_interface), config_(config), last_update_(0), status_valid_(false)
        {
            memset(&last_status_, 0, sizeof(last_status_));
        }

        void HAInterface::begin(const MQTTCallbacks &callbacks)
        {
            mqtt_ = callbacks;

            if (config_.auto_discovery)
            {
                if (!OpenTherm::Discovery::publishDiscoveryConfigs(config_))
                {
                    printf("FATAL ERROR: Failed to publish discovery configurations after all retries\n");
                    printf("Cannot continue without Home Assistant discovery - halting execution\n");
                    OpenTherm::LED::set_pattern(OpenTherm::LED::BLINK_CONFIG_ERROR);
                    while (true)
                    {
                        sleep_ms(1000); // Halt execution
                    }
                }
            }

            // Subscribe to command topics
            // Format: <topic_base>/<device_id>/<command_topic_base>/<suffix>
            // Example: opentherm/opentherm_gw/cmd/ch_enable
            using namespace OpenTherm::MQTTTopics;
            std::string base_cmd = std::string(config_.topic_base) + "/" + std::string(config_.device_id) + "/" + std::string(config_.command_topic_base) + "/";
            mqtt_.subscribe((base_cmd + CH_ENABLE).c_str());
            mqtt_.subscribe((base_cmd + DHW_ENABLE).c_str());
            mqtt_.subscribe((base_cmd + CONTROL_SETPOINT).c_str());
            mqtt_.subscribe((base_cmd + ROOM_SETPOINT).c_str());
            mqtt_.subscribe((base_cmd + DHW_SETPOINT).c_str());
            mqtt_.subscribe((base_cmd + MAX_CH_SETPOINT).c_str());
            mqtt_.subscribe((base_cmd + SYNC_TIME).c_str());
            mqtt_.subscribe((base_cmd + RESTART).c_str());
            mqtt_.subscribe((base_cmd + UPDATE_INTERVAL).c_str());
        }

        void HAInterface::publishDiscoveryConfigs()
        {
            // Delegate to Discovery namespace implementation
            Discovery::publishDiscoveryConfigs(config_);
        }

        void HAInterface::publishSensor(const char *topic_suffix, float value)
        {
            Discovery::publishSensor(config_, topic_suffix, value);
        }

        void HAInterface::publishSensor(const char *topic_suffix, int value)
        {
            Discovery::publishSensor(config_, topic_suffix, value);
        }

        void HAInterface::publishSensor(const char *topic_suffix, const char *value)
        {
            Discovery::publishSensor(config_, topic_suffix, value);
        }

        void HAInterface::publishBinarySensor(const char *topic_suffix, bool value)
        {
            Discovery::publishBinarySensor(config_, topic_suffix, value);
        }

        void HAInterface::publishStatus()
        {
            opentherm_status_t status;
            if (ot_.readStatus(&status))
            {
                last_status_ = status;
                status_valid_ = true;

                using namespace OpenTherm::MQTTTopics;
                // Binary sensors
                Discovery::publishBinarySensor(config_, FAULT, status.fault);
                Discovery::publishBinarySensor(config_, CH_MODE, status.ch_mode);
                Discovery::publishBinarySensor(config_, DHW_MODE, status.dhw_mode);
                Discovery::publishBinarySensor(config_, FLAME, status.flame);
                Discovery::publishBinarySensor(config_, COOLING, status.cooling);
                Discovery::publishBinarySensor(config_, CH2_PRESENT, status.ch2_mode);
                Discovery::publishBinarySensor(config_, DIAGNOSTIC, status.diagnostic);

                // Switches (current state)
                Discovery::publishBinarySensor(config_, CH_ENABLE, status.ch_enable);
                Discovery::publishBinarySensor(config_, DHW_ENABLE, status.dhw_enable);
            }
        }

        void HAInterface::publishTemperatures()
        {
            float temp;

            if (ot_.readBoilerTemperature(&temp))
            {
                Discovery::publishSensor(config_, MQTTTopics::BOILER_TEMP, temp);
            }

            if (ot_.readDHWTemperature(&temp))
            {
                Discovery::publishSensor(config_, MQTTTopics::DHW_TEMP, temp);
            }

            if (ot_.readReturnWaterTemperature(&temp))
            {
                Discovery::publishSensor(config_, MQTTTopics::RETURN_TEMP, temp);
            }

            if (ot_.readOutsideTemperature(&temp))
            {
                Discovery::publishSensor(config_, MQTTTopics::OUTSIDE_TEMP, temp);
            }

            if (ot_.readRoomTemperature(&temp))
            {
                Discovery::publishSensor(config_, MQTTTopics::ROOM_TEMP, temp);
            }

            int16_t exhaust_temp;
            if (ot_.readExhaustTemperature(&exhaust_temp))
            {
                Discovery::publishSensor(config_, MQTTTopics::EXHAUST_TEMP, (int)exhaust_temp);
            }

            // Read setpoints
            if (ot_.readControlSetpoint(&temp))
            {
                Discovery::publishSensor(config_, MQTTTopics::CONTROL_SETPOINT, temp);
            }

            if (ot_.readDHWSetpoint(&temp))
            {
                Discovery::publishSensor(config_, MQTTTopics::DHW_SETPOINT, temp);
            }

            if (ot_.readMaxCHSetpoint(&temp))
            {
                Discovery::publishSensor(config_, MQTTTopics::MAX_CH_SETPOINT, temp);
            }
        }

        void HAInterface::publishPressureFlow()
        {
            float value;

            if (ot_.readCHWaterPressure(&value))
            {
                publishSensor(MQTTTopics::PRESSURE, value);
            }

            if (ot_.readDHWFlowRate(&value))
            {
                publishSensor(MQTTTopics::DHW_FLOW, value);
            }
        }

        void HAInterface::publishModulation()
        {
            float level;

            if (ot_.readModulationLevel(&level))
            {
                publishSensor(MQTTTopics::MODULATION, level);
            }

            // Read max modulation
            float max_mod;
            if (ot_.readMaxModulationLevel(&max_mod))
            {
                publishSensor(MQTTTopics::MAX_MODULATION, max_mod);
            }
        }

        void HAInterface::publishCounters()
        {
            uint16_t count;

            if (ot_.readBurnerStarts(&count))
            {
                publishSensor(MQTTTopics::BURNER_STARTS, (int)count);
            }

            if (ot_.readCHPumpStarts(&count))
            {
                publishSensor(MQTTTopics::CH_PUMP_STARTS, (int)count);
            }

            if (ot_.readDHWPumpStarts(&count))
            {
                publishSensor(MQTTTopics::DHW_PUMP_STARTS, (int)count);
            }

            if (ot_.readBurnerHours(&count))
            {
                publishSensor(MQTTTopics::BURNER_HOURS, (int)count);
            }

            if (ot_.readCHPumpHours(&count))
            {
                publishSensor(MQTTTopics::CH_PUMP_HOURS, (int)count);
            }

            if (ot_.readDHWPumpHours(&count))
            {
                publishSensor(MQTTTopics::DHW_PUMP_HOURS, (int)count);
            }
        }

        void HAInterface::publishConfiguration()
        {
            opentherm_config_t config;
            if (ot_.readSlaveConfig(&config))
            {
                publishBinarySensor("dhw_present", config.dhw_present);
                publishBinarySensor("cooling_supported", config.cooling_config);
                publishBinarySensor("ch2_present", config.ch2_present);
            }

            // Read OpenTherm version
            float version;
            if (ot_.readOpenThermVersion(&version))
            {
                publishSensor(MQTTTopics::OPENTHERM_VERSION, version);
            }
        }

        void HAInterface::publishFaults()
        {
            opentherm_fault_t fault;
            if (ot_.readFaultFlags(&fault))
            {
                publishSensor(MQTTTopics::FAULT_CODE, (int)fault.code);
            }

            uint16_t diag_code;
            if (ot_.readOemDiagnosticCode(&diag_code))
            {
                publishSensor(MQTTTopics::DIAGNOSTIC_CODE, (int)diag_code);
            }
        }

        void HAInterface::publishTimeDate()
        {
            // Read time/date from boiler (if supported)
            uint8_t day_of_week, hours, minutes;
            if (ot_.readDayTime(&day_of_week, &hours, &minutes))
            {
                // Day of week (1=Monday, 7=Sunday, 0=unknown)
                const char *day_names[] = {"Unknown", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday"};
                if (day_of_week <= 7)
                {
                    publishSensor(MQTTTopics::DAY_OF_WEEK, day_names[day_of_week]);
                }

                // Time of day (HH:MM format)
                char time_str[16];
                snprintf(time_str, sizeof(time_str), "%02u:%02u", hours, minutes);
                publishSensor(MQTTTopics::TIME_OF_DAY, time_str);
            }

            // Date (ID 21)
            uint8_t month, day;
            if (ot_.readDate(&month, &day))
            {
                // Date (MM/DD format)
                char date_str[16];
                snprintf(date_str, sizeof(date_str), "%02u/%02u", month, day);
                publishSensor(MQTTTopics::DATE, date_str);
            }

            // Year (ID 22)
            uint16_t year;
            if (ot_.readYear(&year))
            {
                publishSensor(MQTTTopics::YEAR, (int)year);
            }
        }

        void HAInterface::publishTemperatureBounds()
        {
            // DHW bounds (ID 48)
            uint8_t dhw_min, dhw_max;
            if (ot_.readDHWBounds(&dhw_min, &dhw_max))
            {
                publishSensor(MQTTTopics::DHW_SETPOINT_MIN, (int)dhw_min);
                publishSensor(MQTTTopics::DHW_SETPOINT_MAX, (int)dhw_max);
            }

            // CH bounds (ID 49)
            uint8_t ch_min, ch_max;
            if (ot_.readCHBounds(&ch_min, &ch_max))
            {
                publishSensor(MQTTTopics::CH_SETPOINT_MIN, (int)ch_min);
                publishSensor(MQTTTopics::CH_SETPOINT_MAX, (int)ch_max);
            }
        }

        void HAInterface::publishWiFiStats()
        {
            // WiFi RSSI (signal strength in dBm)
            int32_t rssi = 0;
            if (cyw43_wifi_get_rssi(&cyw43_state, &rssi) == 0)
            {
                publishSensor(MQTTTopics::WIFI_RSSI, (int)rssi);
            }

            // WiFi link status
            int link_status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
            const char *status_str = "unknown";

            // Check if we have an IP address
            bool has_ip = false;
            const char *ip_addr = nullptr;
            if (netif_list && netif_is_up(netif_list))
            {
                ip_addr = ip4addr_ntoa(netif_ip4_addr(netif_list));
                // Check if IP is not 0.0.0.0
                has_ip = (ip_addr && strcmp(ip_addr, "0.0.0.0") != 0);
            }

            switch (link_status)
            {
            case CYW43_LINK_DOWN:
                status_str = "down";
                break;
            case CYW43_LINK_JOIN:
                // CYW43_LINK_JOIN = Connected to WiFi
                // Override based on actual IP state since driver may not update to LINK_UP
                status_str = has_ip ? "connected" : "joining";
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
            publishSensor(MQTTTopics::WIFI_LINK_STATUS, status_str);

            // IP address
            if (ip_addr)
            {
                publishSensor(MQTTTopics::IP_ADDRESS, ip_addr);
            }

            // WiFi SSID (from configuration)
            char ssid[64];
            if (::Config::getWiFiSSID(ssid, sizeof(ssid)))
            {
                publishSensor(MQTTTopics::WIFI_SSID, ssid);
            }

            // Uptime in seconds
            uint64_t uptime_us = time_us_64();
            uint32_t uptime_seconds = (uint32_t)(uptime_us / 1000000ULL);
            publishSensor(MQTTTopics::UPTIME, (int)uptime_seconds);

            // Free heap memory - using total_heap from Pico SDK
            // Note: This requires mallinfo() or custom heap tracking
            // For now, we'll publish a placeholder value of 0
            // TODO: Implement proper heap tracking using malloc_stats or custom allocator
            publishSensor(MQTTTopics::FREE_HEAP, 0);
        }

        // Parse ISO 8601 datetime string (e.g., "2025-01-17T14:30:00Z" or "2025-01-17T14:30:00+00:00")
        bool HAInterface::syncTimeToBoiler(const char *iso8601_time)
        {
            if (!iso8601_time || strlen(iso8601_time) < 19)
            {
                printf("ERROR: Invalid ISO 8601 time string\n");
                return false;
            }

            // Parse ISO 8601 format: YYYY-MM-DDTHH:MM:SS
            int year, month, day, hour, minute, second;
            int parsed = sscanf(iso8601_time, "%d-%d-%dT%d:%d:%d",
                                &year, &month, &day, &hour, &minute, &second);

            if (parsed != 6)
            {
                printf("ERROR: Failed to parse ISO 8601 time: %s\n", iso8601_time);
                return false;
            }

            printf("Syncing time to boiler: %04d-%02d-%02d %02d:%02d:%02d\n",
                   year, month, day, hour, minute, second);

            // Calculate day of week (using Zeller's congruence for Gregorian calendar)
            int adj_month = month;
            int adj_year = year;
            if (month < 3)
            {
                adj_month += 12;
                adj_year -= 1;
            }
            int century = adj_year / 100;
            int year_of_century = adj_year % 100;
            int dow_zeller = (day + (13 * (adj_month + 1)) / 5 + year_of_century +
                              year_of_century / 4 + century / 4 - 2 * century) %
                             7;
            // Convert Zeller (0=Saturday) to OpenTherm (1=Monday, 7=Sunday)
            uint8_t day_of_week = ((dow_zeller + 5) % 7) + 1;

            bool success = true;

            // Sync day/time to boiler (ID 20)
            if (!ot_.writeDayTime(day_of_week, (uint8_t)hour, (uint8_t)minute))
            {
                printf("WARNING: Failed to sync day/time to boiler\n");
                success = false;
            }
            else
            {
                printf("  Day/time synced successfully\n");
            }

            // Sync date to boiler (ID 21)
            if (!ot_.writeDate((uint8_t)month, (uint8_t)day))
            {
                printf("WARNING: Failed to sync date to boiler\n");
                success = false;
            }
            else
            {
                printf("  Date synced successfully\n");
            }

            // Sync year to boiler (ID 22)
            if (!ot_.writeYear((uint16_t)year))
            {
                printf("WARNING: Failed to sync year to boiler\n");
                success = false;
            }
            else
            {
                printf("  Year synced successfully\n");
            }

            if (success)
            {
                printf("Time synchronized to boiler successfully!\n");
            }
            else
            {
                printf("Time sync completed with warnings (boiler may not support all fields)\n");
            }

            return success;
        }

        // Parse Unix timestamp
        bool HAInterface::syncTimeToBoiler(uint32_t unix_timestamp)
        {
            // Convert Unix timestamp to datetime components
            // Unix epoch: 1970-01-01 00:00:00 UTC
            const uint32_t SECONDS_PER_DAY = 86400;
            const uint32_t SECONDS_PER_HOUR = 3600;
            const uint32_t SECONDS_PER_MINUTE = 60;

            // Days since epoch
            uint32_t days = unix_timestamp / SECONDS_PER_DAY;
            uint32_t remaining_seconds = unix_timestamp % SECONDS_PER_DAY;

            // Time components
            uint8_t hour = remaining_seconds / SECONDS_PER_HOUR;
            remaining_seconds %= SECONDS_PER_HOUR;
            uint8_t minute = remaining_seconds / SECONDS_PER_MINUTE;

            // Calculate date (simplified algorithm)
            // Start from 1970-01-01
            uint16_t year = 1970;
            uint8_t month = 1;
            uint8_t day = 1;

            // Days in each month (non-leap year)
            const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

            // Add days
            while (days > 0)
            {
                bool is_leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
                uint16_t days_in_year = is_leap ? 366 : 365;

                if (days >= days_in_year)
                {
                    days -= days_in_year;
                    year++;
                }
                else
                {
                    // Add months
                    while (days > 0)
                    {
                        uint8_t month_days = days_in_month[month - 1];
                        if (month == 2 && is_leap)
                            month_days = 29;

                        if (days >= month_days)
                        {
                            days -= month_days;
                            month++;
                            if (month > 12)
                            {
                                month = 1;
                                year++;
                            }
                        }
                        else
                        {
                            day += days;
                            days = 0;
                        }
                    }
                }
            }

            printf("Syncing time to boiler from timestamp %lu: %04d-%02d-%02d %02d:%02d\n",
                   unix_timestamp, year, month, day, hour, minute);

            // Calculate day of week (1970-01-01 was Thursday = 4)
            uint32_t total_days = (unix_timestamp / SECONDS_PER_DAY);
            uint8_t day_of_week = ((total_days + 4) % 7); // 0=Sunday
            if (day_of_week == 0)
                day_of_week = 7; // OpenTherm: 7=Sunday
            // else OpenTherm: 1=Monday matches (day_of_week from calculation)

            bool success = true;

            // Sync day/time to boiler
            if (!ot_.writeDayTime(day_of_week, hour, minute))
            {
                printf("WARNING: Failed to sync day/time to boiler\n");
                success = false;
            }

            // Sync date to boiler
            if (!ot_.writeDate(month, day))
            {
                printf("WARNING: Failed to sync date to boiler\n");
                success = false;
            }

            // Sync year to boiler
            if (!ot_.writeYear(year))
            {
                printf("WARNING: Failed to sync year to boiler\n");
                success = false;
            }

            if (success)
            {
                printf("Time synchronized to boiler successfully!\n");
            }

            return success;
        }

        void HAInterface::publishDeviceConfiguration()
        {
            char buffer[128];

            // Publish device name
            if (::Config::getDeviceName(buffer, sizeof(buffer)))
            {
                publishSensor(MQTTTopics::DEVICE_NAME, buffer);
            }

            // Publish device ID
            if (::Config::getDeviceID(buffer, sizeof(buffer)))
            {
                publishSensor(MQTTTopics::DEVICE_ID, buffer);
            }

            // Publish OpenTherm GPIO pins
            publishSensor(MQTTTopics::OPENTHERM_TX_PIN, (int)::Config::getOpenThermTxPin());
            publishSensor(MQTTTopics::OPENTHERM_RX_PIN, (int)::Config::getOpenThermRxPin());

            // Publish update interval
            publishSensor(MQTTTopics::UPDATE_INTERVAL, (int)config_.update_interval_ms);
        }

        void HAInterface::update()
        {
            // TODO : Add failsafe mode
            uint32_t now = to_ms_since_boot(get_absolute_time());

            if (now - last_update_ >= config_.update_interval_ms)
            {
                last_update_ = now;

                // Update all sensors with delays between groups to prevent lwIP TCP panic
                // Each group publishes multiple messages - need time for TCP ACKs
                // Increased to 200ms with active network polling to prevent buffer exhaustion
                publishStatus();
                for (int i = 0; i < 20; i++) { cyw43_arch_poll(); sleep_ms(10); }

                publishTemperatures();
                for (int i = 0; i < 20; i++) { cyw43_arch_poll(); sleep_ms(10); }

                publishPressureFlow();
                for (int i = 0; i < 20; i++) { cyw43_arch_poll(); sleep_ms(10); }

                publishModulation();
                for (int i = 0; i < 20; i++) { cyw43_arch_poll(); sleep_ms(10); }

                publishCounters();
                for (int i = 0; i < 20; i++) { cyw43_arch_poll(); sleep_ms(10); }

                publishConfiguration();
                for (int i = 0; i < 20; i++) { cyw43_arch_poll(); sleep_ms(10); }

                publishFaults();
                for (int i = 0; i < 20; i++) { cyw43_arch_poll(); sleep_ms(10); }

                publishTimeDate();
                for (int i = 0; i < 20; i++) { cyw43_arch_poll(); sleep_ms(10); }

                publishTemperatureBounds();
                for (int i = 0; i < 20; i++) { cyw43_arch_poll(); sleep_ms(10); }

                publishWiFiStats();
                for (int i = 0; i < 20; i++) { cyw43_arch_poll(); sleep_ms(10); }

                publishDeviceConfiguration();
            }
        }

        void HAInterface::handleMessage(const char *topic, const char *payload)
        {
            // Build command topic base: <topic_base>/<device_id>/<command_topic_base>/
            // Example: opentherm/opentherm_gw/cmd/
            std::string cmd_base = std::string(config_.topic_base) + "/" + std::string(config_.device_id) + "/" + std::string(config_.command_topic_base) + "/";

            // CH Enable switch
            if (strcmp(topic, (cmd_base + MQTTTopics::CH_ENABLE).c_str()) == 0)
            {
                bool enable = (strcmp(payload, "ON") == 0);
                setCHEnable(enable);
            }
            // DHW Enable switch
            else if (strcmp(topic, (cmd_base + MQTTTopics::DHW_ENABLE).c_str()) == 0)
            {
                bool enable = (strcmp(payload, "ON") == 0);
                setDHWEnable(enable);
            }
            // Control setpoint
            else if (strcmp(topic, (cmd_base + MQTTTopics::CONTROL_SETPOINT).c_str()) == 0)
            {
                float temp = atof(payload);
                setControlSetpoint(temp);
            }
            // Room setpoint
            else if (strcmp(topic, (cmd_base + MQTTTopics::ROOM_SETPOINT).c_str()) == 0)
            {
                float temp = atof(payload);
                setRoomSetpoint(temp);
            }
            // DHW setpoint
            else if (strcmp(topic, (cmd_base + MQTTTopics::DHW_SETPOINT).c_str()) == 0)
            {
                float temp = atof(payload);
                setDHWSetpoint(temp);
            }
            // Max CH setpoint
            else if (strcmp(topic, (cmd_base + MQTTTopics::MAX_CH_SETPOINT).c_str()) == 0)
            {
                float temp = atof(payload);
                setMaxCHSetpoint(temp);
            }
            // Device name
            else if (strcmp(topic, (cmd_base + MQTTTopics::DEVICE_NAME).c_str()) == 0)
            {
                setDeviceName(payload);
            }
            // Device ID
            else if (strcmp(topic, (cmd_base + MQTTTopics::DEVICE_ID).c_str()) == 0)
            {
                setDeviceID(payload);
            }
            // OpenTherm TX pin
            else if (strcmp(topic, (cmd_base + MQTTTopics::OPENTHERM_TX_PIN).c_str()) == 0)
            {
                uint8_t pin = (uint8_t)atoi(payload);
                setOpenThermTxPin(pin);
            }
            // OpenTherm RX pin
            else if (strcmp(topic, (cmd_base + MQTTTopics::OPENTHERM_RX_PIN).c_str()) == 0)
            {
                uint8_t pin = (uint8_t)atoi(payload);
                setOpenThermRxPin(pin);
            }
            // Time sync command
            else if (strcmp(topic, (cmd_base + MQTTTopics::SYNC_TIME).c_str()) == 0)
            {
                // Payload format can be:
                // 1. ISO 8601: "2025-01-17T14:30:00Z"
                // 2. Unix timestamp: "1737121800"
                // 3. "PRESS" from HA button (we'll use current system time if available)

                if (payload && strlen(payload) > 0)
                {
                    // Check if it's a Unix timestamp (all digits)
                    bool is_timestamp = true;
                    for (size_t i = 0; i < strlen(payload); i++)
                    {
                        if (!isdigit(payload[i]))
                        {
                            is_timestamp = false;
                            break;
                        }
                    }

                    if (is_timestamp && strlen(payload) >= 10)
                    {
                        // Unix timestamp
                        uint32_t timestamp = (uint32_t)strtoul(payload, NULL, 10);
                        printf("Received time sync request with timestamp: %lu\n", timestamp);
                        syncTimeToBoiler(timestamp);
                    }
                    else if (strchr(payload, 'T') != nullptr)
                    {
                        // ISO 8601 format
                        printf("Received time sync request with ISO 8601: %s\n", payload);
                        syncTimeToBoiler(payload);
                    }
                    else
                    {
                        printf("Time sync requested but format not recognized: %s\n", payload);
                        printf("Expected ISO 8601 (YYYY-MM-DDTHH:MM:SS) or Unix timestamp\n");
                    }
                }
            }
            // Restart command
            else if (strcmp(topic, (cmd_base + MQTTTopics::RESTART).c_str()) == 0)
            {
                printf("Restart requested via MQTT command\n");
                printf("Restarting in 2 seconds...\n");
                sleep_ms(2000); // Give time for the message to be logged and MQTT to ack
                watchdog_reboot(0, 0, 0);
                // watchdog_reboot will reset the system
            }
            // Update interval command
            else if (strcmp(topic, (cmd_base + MQTTTopics::UPDATE_INTERVAL).c_str()) == 0)
            {
                uint32_t interval_ms = (uint32_t)atoi(payload);
                setUpdateInterval(interval_ms);
            }
        }

        bool HAInterface::setControlSetpoint(float temperature)
        {
            if (ot_.writeControlSetpoint(temperature))
            {
                publishSensor(MQTTTopics::CONTROL_SETPOINT, temperature);
                return true;
            }
            return false;
        }

        bool HAInterface::setRoomSetpoint(float temperature)
        {
            if (ot_.writeRoomSetpoint(temperature))
            {
                publishSensor(MQTTTopics::ROOM_SETPOINT, temperature);
                return true;
            }
            return false;
        }

        bool HAInterface::setDHWSetpoint(float temperature)
        {
            if (ot_.writeDHWSetpoint(temperature))
            {
                publishSensor(MQTTTopics::DHW_SETPOINT, temperature);
                return true;
            }
            return false;
        }

        bool HAInterface::setMaxCHSetpoint(float temperature)
        {
            if (ot_.writeMaxCHSetpoint(temperature))
            {
                publishSensor(MQTTTopics::MAX_CH_SETPOINT, temperature);
                return true;
            }
            return false;
        }

        bool HAInterface::setCHEnable(bool enable)
        {
            if (ot_.writeCHEnable(enable))
            {
                publishBinarySensor(MQTTTopics::CH_ENABLE, enable);
                return true;
            }
            return false;
        }

        bool HAInterface::setDHWEnable(bool enable)
        {
            if (ot_.writeDHWEnable(enable))
            {
                publishBinarySensor(MQTTTopics::DHW_ENABLE, enable);
                return true;
            }
            return false;
        }

        bool HAInterface::setDeviceName(const char *name)
        {
            if (::Config::setDeviceName(name))
            {
                publishSensor(MQTTTopics::DEVICE_NAME, name);
                printf("Device name updated to: %s - restarting in 2 seconds...\n", name);
                sleep_ms(2000);
                watchdog_reboot(0, 0, 0);
                return true;
            }
            return false;
        }

        bool HAInterface::setDeviceID(const char *id)
        {
            if (::Config::setDeviceID(id))
            {
                publishSensor(MQTTTopics::DEVICE_ID, id);
                printf("Device ID updated to: %s - restarting in 2 seconds...\n", id);
                sleep_ms(2000);
                watchdog_reboot(0, 0, 0);
                return true;
            }
            return false;
        }

        bool HAInterface::setOpenThermTxPin(uint8_t pin)
        {
            if (::Config::setOpenThermTxPin(pin))
            {
                publishSensor(MQTTTopics::OPENTHERM_TX_PIN, (int)pin);
                printf("OpenTherm TX pin updated to: GPIO%u - restarting in 2 seconds...\n", pin);
                sleep_ms(2000);
                watchdog_reboot(0, 0, 0);
                return true;
            }
            return false;
        }

        bool HAInterface::setOpenThermRxPin(uint8_t pin)
        {
            if (::Config::setOpenThermRxPin(pin))
            {
                publishSensor(MQTTTopics::OPENTHERM_RX_PIN, (int)pin);
                printf("OpenTherm RX pin updated to: GPIO%u - restarting in 2 seconds...\n", pin);
                sleep_ms(2000);
                watchdog_reboot(0, 0, 0);
                return true;
            }
            return false;
        }

        bool HAInterface::setUpdateInterval(uint32_t interval_ms)
        {
            if (::Config::setUpdateIntervalMs(interval_ms))
            {
                config_.update_interval_ms = interval_ms;
                publishSensor(MQTTTopics::UPDATE_INTERVAL, (int)interval_ms);
                printf("Update interval changed to: %u ms (%.1f seconds)\n", interval_ms, interval_ms / 1000.0f);
                return true;
            }
            return false;
        }

        uint32_t HAInterface::getUpdateInterval() const
        {
            return config_.update_interval_ms;
        }

    } // namespace HomeAssistant
} // namespace OpenTherm
