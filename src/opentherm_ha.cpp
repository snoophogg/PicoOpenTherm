#include "opentherm_ha.hpp"
#include "config.hpp"
#include "mqtt_discovery.hpp"
#include "pico/cyw43_arch.h"
#include <cstdio>
#include <cstring>
#include <sstream>
#include "pico/stdlib.h"
#include "hardware/watchdog.h"

namespace OpenTherm
{
    namespace HomeAssistant
    {

        HAInterface::HAInterface(OpenTherm::Interface &ot_interface, const Config &config)
            : ot_(ot_interface), config_(config), last_update_(0), status_valid_(false)
        {
            memset(&last_status_, 0, sizeof(last_status_));
        }

        void HAInterface::begin(const MQTTCallbacks &callbacks)
        {
            mqtt_ = callbacks;

            if (config_.auto_discovery)
            {
                publishDiscoveryConfigs();
            }

            // Subscribe to command topics
            std::string base_cmd = config_.command_topic_base;
            mqtt_.subscribe((base_cmd + "/ch_enable").c_str());
            mqtt_.subscribe((base_cmd + "/dhw_enable").c_str());
            mqtt_.subscribe((base_cmd + "/control_setpoint").c_str());
            mqtt_.subscribe((base_cmd + "/room_setpoint").c_str());
            mqtt_.subscribe((base_cmd + "/dhw_setpoint").c_str());
            mqtt_.subscribe((base_cmd + "/max_ch_setpoint").c_str());
        }

        std::string HAInterface::buildStateTopic(const char *suffix)
        {
            return std::string(config_.state_topic_base) + "/" + suffix;
        }

        std::string HAInterface::buildCommandTopic(const char *suffix)
        {
            return std::string(config_.command_topic_base) + "/" + suffix;
        }

        std::string HAInterface::buildDiscoveryTopic(const char *component, const char *object_id)
        {
            return std::string(config_.mqtt_prefix) + "/" + component + "/" +
                   config_.device_id + "/" + object_id + "/config";
        }

        void HAInterface::publishDiscoveryConfig(const char *component, const char *object_id,
                                                 const char *name, const char *state_topic,
                                                 const char *device_class, const char *unit,
                                                 const char *icon, const char *command_topic,
                                                 const char *value_template,
                                                 float min_value, float max_value, float step)
        {
            std::ostringstream payload;
            payload << "{";
            payload << "\"name\":\"" << name << "\",";
            payload << "\"unique_id\":\"" << config_.device_id << "_" << object_id << "\",";
            payload << "\"state_topic\":\"" << state_topic << "\",";

            if (command_topic)
            {
                payload << "\"command_topic\":\"" << command_topic << "\",";
            }

            if (device_class)
            {
                payload << "\"device_class\":\"" << device_class << "\",";
            }

            if (unit)
            {
                payload << "\"unit_of_measurement\":\"" << unit << "\",";
            }

            if (icon)
            {
                payload << "\"icon\":\"" << icon << "\",";
            }

            if (value_template)
            {
                payload << "\"value_template\":\"" << value_template << "\",";
            }

            // For number entities
            if (strcmp(component, "number") == 0)
            {
                payload << "\"min\":" << min_value << ",";
                payload << "\"max\":" << max_value << ",";
                payload << "\"step\":" << step << ",";
                payload << "\"mode\":\"box\",";
            }

            // Device info
            payload << "\"device\":{";
            payload << "\"identifiers\":[\"" << config_.device_id << "\"],";
            payload << "\"name\":\"" << config_.device_name << "\",";
            payload << "\"model\":\"OpenTherm Gateway\",";
            payload << "\"manufacturer\":\"PicoOpenTherm\"";
            payload << "}";

            payload << "}";

            std::string topic = buildDiscoveryTopic(component, object_id);
            // Use retry logic from discovery library for reliable publishing
            if (!Discovery::publishWithRetry(topic.c_str(), payload.str().c_str()))
            {
                printf("WARNING: Failed to publish discovery config for %s/%s\n", component, object_id);
            }
        }

        void HAInterface::publishDiscoveryConfigs()
        {
            // Wait for MQTT client to be ready for large discovery messages
            // Poll network actively to process any pending packets
            // Extended wait (5s) to allow MQTT handshake and buffers to fully stabilize
            printf("Waiting for MQTT client to be ready for discovery (5 seconds, polling network)...\n");
            for (int i = 0; i < 500; i++)
            {
                cyw43_arch_poll();
                sleep_ms(10);
            }

            printf("Publishing Home Assistant MQTT discovery configs...\n");

            // Status flags - Binary Sensors
            publishDiscoveryConfig("binary_sensor", "fault", "Fault",
                                   buildStateTopic("fault").c_str(), "problem", nullptr, "mdi:alert-circle");
            publishDiscoveryConfig("binary_sensor", "ch_mode", "Central Heating Mode",
                                   buildStateTopic("ch_mode").c_str(), "heat", nullptr, "mdi:radiator");
            publishDiscoveryConfig("binary_sensor", "dhw_mode", "Hot Water Mode",
                                   buildStateTopic("dhw_mode").c_str(), "heat", nullptr, "mdi:water-boiler");
            publishDiscoveryConfig("binary_sensor", "flame", "Flame Status",
                                   buildStateTopic("flame").c_str(), "heat", nullptr, "mdi:fire");
            publishDiscoveryConfig("binary_sensor", "cooling", "Cooling Active",
                                   buildStateTopic("cooling").c_str(), "cold", nullptr, "mdi:snowflake");
            publishDiscoveryConfig("binary_sensor", "diagnostic", "Diagnostic Mode",
                                   buildStateTopic("diagnostic").c_str(), nullptr, nullptr, "mdi:wrench");

            // Control switches
            publishDiscoveryConfig("switch", "ch_enable", "Central Heating Enable",
                                   buildStateTopic("ch_enable").c_str(), "switch", nullptr, "mdi:radiator",
                                   buildCommandTopic("ch_enable").c_str());
            publishDiscoveryConfig("switch", "dhw_enable", "Hot Water Enable",
                                   buildStateTopic("dhw_enable").c_str(), "switch", nullptr, "mdi:water-boiler",
                                   buildCommandTopic("dhw_enable").c_str());

            // Temperature sensors
            publishDiscoveryConfig("sensor", "boiler_temp", "Boiler Temperature",
                                   buildStateTopic("boiler_temp").c_str(), "temperature", "°C", "mdi:thermometer");
            publishDiscoveryConfig("sensor", "dhw_temp", "Hot Water Temperature",
                                   buildStateTopic("dhw_temp").c_str(), "temperature", "°C", "mdi:thermometer");
            publishDiscoveryConfig("sensor", "return_temp", "Return Water Temperature",
                                   buildStateTopic("return_temp").c_str(), "temperature", "°C", "mdi:thermometer");
            publishDiscoveryConfig("sensor", "outside_temp", "Outside Temperature",
                                   buildStateTopic("outside_temp").c_str(), "temperature", "°C", "mdi:thermometer");
            publishDiscoveryConfig("sensor", "room_temp", "Room Temperature",
                                   buildStateTopic("room_temp").c_str(), "temperature", "°C", "mdi:home-thermometer");
            publishDiscoveryConfig("sensor", "exhaust_temp", "Exhaust Temperature",
                                   buildStateTopic("exhaust_temp").c_str(), "temperature", "°C", "mdi:thermometer");

            // Setpoint numbers
            publishDiscoveryConfig("number", "control_setpoint", "Control Setpoint",
                                   buildStateTopic("control_setpoint").c_str(), nullptr, "°C", "mdi:thermometer-lines",
                                   buildCommandTopic("control_setpoint").c_str(), nullptr, 0.0f, 100.0f, 0.5f);
            publishDiscoveryConfig("number", "room_setpoint", "Room Setpoint",
                                   buildStateTopic("room_setpoint").c_str(), nullptr, "°C", "mdi:home-thermometer-outline",
                                   buildCommandTopic("room_setpoint").c_str(), nullptr, 5.0f, 30.0f, 0.5f);
            publishDiscoveryConfig("number", "dhw_setpoint", "Hot Water Setpoint",
                                   buildStateTopic("dhw_setpoint").c_str(), nullptr, "°C", "mdi:water-thermometer-outline",
                                   buildCommandTopic("dhw_setpoint").c_str(), nullptr, 30.0f, 90.0f, 1.0f);
            publishDiscoveryConfig("number", "max_ch_setpoint", "Max CH Setpoint",
                                   buildStateTopic("max_ch_setpoint").c_str(), nullptr, "°C", "mdi:thermometer-high",
                                   buildCommandTopic("max_ch_setpoint").c_str(), nullptr, 30.0f, 90.0f, 1.0f);

            // Modulation sensor
            publishDiscoveryConfig("sensor", "modulation", "Modulation Level",
                                   buildStateTopic("modulation").c_str(), nullptr, "%", "mdi:percent");
            publishDiscoveryConfig("sensor", "max_modulation", "Max Modulation Level",
                                   buildStateTopic("max_modulation").c_str(), nullptr, "%", "mdi:percent");

            // Pressure and flow sensors
            publishDiscoveryConfig("sensor", "pressure", "CH Water Pressure",
                                   buildStateTopic("pressure").c_str(), "pressure", "bar", "mdi:gauge");
            publishDiscoveryConfig("sensor", "dhw_flow", "Hot Water Flow Rate",
                                   buildStateTopic("dhw_flow").c_str(), nullptr, "l/min", "mdi:water-pump");

            // Counter sensors
            publishDiscoveryConfig("sensor", "burner_starts", "Burner Starts",
                                   buildStateTopic("burner_starts").c_str(), nullptr, "starts", "mdi:counter");
            publishDiscoveryConfig("sensor", "ch_pump_starts", "CH Pump Starts",
                                   buildStateTopic("ch_pump_starts").c_str(), nullptr, "starts", "mdi:counter");
            publishDiscoveryConfig("sensor", "dhw_pump_starts", "DHW Pump Starts",
                                   buildStateTopic("dhw_pump_starts").c_str(), nullptr, "starts", "mdi:counter");
            publishDiscoveryConfig("sensor", "burner_hours", "Burner Operating Hours",
                                   buildStateTopic("burner_hours").c_str(), "duration", "h", "mdi:clock-outline");
            publishDiscoveryConfig("sensor", "ch_pump_hours", "CH Pump Operating Hours",
                                   buildStateTopic("ch_pump_hours").c_str(), "duration", "h", "mdi:clock-outline");
            publishDiscoveryConfig("sensor", "dhw_pump_hours", "DHW Pump Operating Hours",
                                   buildStateTopic("dhw_pump_hours").c_str(), "duration", "h", "mdi:clock-outline");

            // Fault information
            publishDiscoveryConfig("sensor", "fault_code", "Fault Code",
                                   buildStateTopic("fault_code").c_str(), nullptr, nullptr, "mdi:alert-octagon");

            // Configuration sensors
            publishDiscoveryConfig("binary_sensor", "dhw_present", "DHW Present",
                                   buildStateTopic("dhw_present").c_str(), nullptr, nullptr, "mdi:water-boiler");
            publishDiscoveryConfig("binary_sensor", "cooling_supported", "Cooling Supported",
                                   buildStateTopic("cooling_supported").c_str(), nullptr, nullptr, "mdi:snowflake");
            publishDiscoveryConfig("binary_sensor", "ch2_present", "CH2 Present",
                                   buildStateTopic("ch2_present").c_str(), nullptr, nullptr, "mdi:radiator");

            // Version info
            publishDiscoveryConfig("sensor", "opentherm_version", "OpenTherm Version",
                                   buildStateTopic("opentherm_version").c_str(), nullptr, nullptr, "mdi:information");

            // Device configuration (text entities)
            publishDiscoveryConfig("text", "device_name", "Device Name",
                                   buildStateTopic("device_name").c_str(), nullptr, nullptr, "mdi:tag-text",
                                   buildCommandTopic("device_name").c_str());
            publishDiscoveryConfig("text", "device_id", "Device ID",
                                   buildStateTopic("device_id").c_str(), nullptr, nullptr, "mdi:identifier",
                                   buildCommandTopic("device_id").c_str());

            // OpenTherm GPIO configuration (number entities)
            publishDiscoveryConfig("number", "opentherm_tx_pin", "OpenTherm TX Pin",
                                   buildStateTopic("opentherm_tx_pin").c_str(), nullptr, nullptr, "mdi:pin",
                                   buildCommandTopic("opentherm_tx_pin").c_str(), nullptr, 0.0f, 28.0f, 1.0f);
            publishDiscoveryConfig("number", "opentherm_rx_pin", "OpenTherm RX Pin",
                                   buildStateTopic("opentherm_rx_pin").c_str(), nullptr, nullptr, "mdi:pin",
                                   buildCommandTopic("opentherm_rx_pin").c_str(), nullptr, 0.0f, 28.0f, 1.0f);

            printf("Discovery configs published!\n");
        }

        void HAInterface::publishSensor(const char *topic_suffix, float value)
        {
            char payload[32];
            snprintf(payload, sizeof(payload), "%.2f", value);
            std::string topic = buildStateTopic(topic_suffix);
            mqtt_.publish(topic.c_str(), payload, false);
        }

        void HAInterface::publishSensor(const char *topic_suffix, int value)
        {
            char payload[32];
            snprintf(payload, sizeof(payload), "%d", value);
            std::string topic = buildStateTopic(topic_suffix);
            mqtt_.publish(topic.c_str(), payload, false);
        }

        void HAInterface::publishSensor(const char *topic_suffix, const char *value)
        {
            std::string topic = buildStateTopic(topic_suffix);
            mqtt_.publish(topic.c_str(), value, false);
        }

        void HAInterface::publishBinarySensor(const char *topic_suffix, bool value)
        {
            std::string topic = buildStateTopic(topic_suffix);
            mqtt_.publish(topic.c_str(), value ? "ON" : "OFF", false);
        }

        void HAInterface::publishStatus()
        {
            opentherm_status_t status;
            if (ot_.readStatus(&status))
            {
                last_status_ = status;
                status_valid_ = true;

                // Binary sensors
                publishBinarySensor("fault", status.fault);
                publishBinarySensor("ch_mode", status.ch_mode);
                publishBinarySensor("dhw_mode", status.dhw_mode);
                publishBinarySensor("flame", status.flame);
                publishBinarySensor("cooling", status.cooling);
                publishBinarySensor("ch2_mode", status.ch2_mode);
                publishBinarySensor("diagnostic", status.diagnostic);

                // Switches (current state)
                publishBinarySensor("ch_enable", status.ch_enable);
                publishBinarySensor("dhw_enable", status.dhw_enable);
            }
        }

        void HAInterface::publishTemperatures()
        {
            float temp;

            if (ot_.readBoilerTemperature(&temp))
            {
                publishSensor("boiler_temp", temp);
            }

            if (ot_.readDHWTemperature(&temp))
            {
                publishSensor("dhw_temp", temp);
            }

            if (ot_.readReturnWaterTemperature(&temp))
            {
                publishSensor("return_temp", temp);
            }

            if (ot_.readOutsideTemperature(&temp))
            {
                publishSensor("outside_temp", temp);
            }

            if (ot_.readRoomTemperature(&temp))
            {
                publishSensor("room_temp", temp);
            }

            int16_t exhaust_temp;
            if (ot_.readExhaustTemperature(&exhaust_temp))
            {
                publishSensor("exhaust_temp", (int)exhaust_temp);
            }

            // Read setpoints
            if (ot_.readControlSetpoint(&temp))
            {
                publishSensor("control_setpoint", temp);
            }

            if (ot_.readDHWSetpoint(&temp))
            {
                publishSensor("dhw_setpoint", temp);
            }

            if (ot_.readMaxCHSetpoint(&temp))
            {
                publishSensor("max_ch_setpoint", temp);
            }
        }

        void HAInterface::publishPressureFlow()
        {
            float value;

            if (ot_.readCHWaterPressure(&value))
            {
                publishSensor("pressure", value);
            }

            if (ot_.readDHWFlowRate(&value))
            {
                publishSensor("dhw_flow", value);
            }
        }

        void HAInterface::publishModulation()
        {
            float level;

            if (ot_.readModulationLevel(&level))
            {
                publishSensor("modulation", level);
            }

            // Read max modulation
            uint32_t request = opentherm_read_max_rel_mod();
            uint32_t response;
            if (ot_.sendAndReceive(request, &response))
            {
                float max_mod = opentherm_get_f8_8(response);
                publishSensor("max_modulation", max_mod);
            }
        }

        void HAInterface::publishCounters()
        {
            uint16_t count;

            if (ot_.readBurnerStarts(&count))
            {
                publishSensor("burner_starts", (int)count);
            }

            if (ot_.readCHPumpStarts(&count))
            {
                publishSensor("ch_pump_starts", (int)count);
            }

            if (ot_.readDHWPumpStarts(&count))
            {
                publishSensor("dhw_pump_starts", (int)count);
            }

            if (ot_.readBurnerHours(&count))
            {
                publishSensor("burner_hours", (int)count);
            }

            if (ot_.readCHPumpHours(&count))
            {
                publishSensor("ch_pump_hours", (int)count);
            }

            if (ot_.readDHWPumpHours(&count))
            {
                publishSensor("dhw_pump_hours", (int)count);
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
                publishSensor("opentherm_version", version);
            }
        }

        void HAInterface::publishFaults()
        {
            opentherm_fault_t fault;
            if (ot_.readFaultFlags(&fault))
            {
                publishSensor("fault_code", (int)fault.code);
            }
        }

        void HAInterface::publishDeviceConfiguration()
        {
            char buffer[128];

            // Publish device name
            if (::Config::getDeviceName(buffer, sizeof(buffer)))
            {
                publishSensor("device_name", buffer);
            }

            // Publish device ID
            if (::Config::getDeviceID(buffer, sizeof(buffer)))
            {
                publishSensor("device_id", buffer);
            }

            // Publish OpenTherm GPIO pins
            publishSensor("opentherm_tx_pin", (int)::Config::getOpenThermTxPin());
            publishSensor("opentherm_rx_pin", (int)::Config::getOpenThermRxPin());
        }

        void HAInterface::update()
        {
            // TODO : Add failsafe mode
            uint32_t now = to_ms_since_boot(get_absolute_time());

            if (now - last_update_ >= config_.update_interval_ms)
            {
                last_update_ = now;

                // Update all sensors
                publishStatus();
                publishTemperatures();
                publishPressureFlow();
                publishModulation();
                publishCounters();
                publishConfiguration();
                publishFaults();
                publishDeviceConfiguration();
            }
        }

        void HAInterface::handleMessage(const char *topic, const char *payload)
        {
            std::string cmd_base = config_.command_topic_base;

            // CH Enable switch
            if (strcmp(topic, (cmd_base + "/ch_enable").c_str()) == 0)
            {
                bool enable = (strcmp(payload, "ON") == 0);
                setCHEnable(enable);
            }
            // DHW Enable switch
            else if (strcmp(topic, (cmd_base + "/dhw_enable").c_str()) == 0)
            {
                bool enable = (strcmp(payload, "ON") == 0);
                setDHWEnable(enable);
            }
            // Control setpoint
            else if (strcmp(topic, (cmd_base + "/control_setpoint").c_str()) == 0)
            {
                float temp = atof(payload);
                setControlSetpoint(temp);
            }
            // Room setpoint
            else if (strcmp(topic, (cmd_base + "/room_setpoint").c_str()) == 0)
            {
                float temp = atof(payload);
                setRoomSetpoint(temp);
            }
            // DHW setpoint
            else if (strcmp(topic, (cmd_base + "/dhw_setpoint").c_str()) == 0)
            {
                float temp = atof(payload);
                setDHWSetpoint(temp);
            }
            // Max CH setpoint
            else if (strcmp(topic, (cmd_base + "/max_ch_setpoint").c_str()) == 0)
            {
                float temp = atof(payload);
                setMaxCHSetpoint(temp);
            }
            // Device name
            else if (strcmp(topic, (cmd_base + "/device_name").c_str()) == 0)
            {
                setDeviceName(payload);
            }
            // Device ID
            else if (strcmp(topic, (cmd_base + "/device_id").c_str()) == 0)
            {
                setDeviceID(payload);
            }
            // OpenTherm TX pin
            else if (strcmp(topic, (cmd_base + "/opentherm_tx_pin").c_str()) == 0)
            {
                uint8_t pin = (uint8_t)atoi(payload);
                setOpenThermTxPin(pin);
            }
            // OpenTherm RX pin
            else if (strcmp(topic, (cmd_base + "/opentherm_rx_pin").c_str()) == 0)
            {
                uint8_t pin = (uint8_t)atoi(payload);
                setOpenThermRxPin(pin);
            }
        }

        bool HAInterface::setControlSetpoint(float temperature)
        {
            if (ot_.writeControlSetpoint(temperature))
            {
                publishSensor("control_setpoint", temperature);
                return true;
            }
            return false;
        }

        bool HAInterface::setRoomSetpoint(float temperature)
        {
            if (ot_.writeRoomSetpoint(temperature))
            {
                publishSensor("room_setpoint", temperature);
                return true;
            }
            return false;
        }

        bool HAInterface::setDHWSetpoint(float temperature)
        {
            if (ot_.writeDHWSetpoint(temperature))
            {
                publishSensor("dhw_setpoint", temperature);
                return true;
            }
            return false;
        }

        bool HAInterface::setMaxCHSetpoint(float temperature)
        {
            if (ot_.writeMaxCHSetpoint(temperature))
            {
                publishSensor("max_ch_setpoint", temperature);
                return true;
            }
            return false;
        }

        bool HAInterface::setCHEnable(bool enable)
        {
            if (!status_valid_)
            {
                return false;
            }

            // Build status frame with updated CH enable flag
            last_status_.ch_enable = enable;
            uint16_t status_value = opentherm_encode_status(&last_status_);

            uint32_t request = opentherm_build_write_request(OT_DATA_ID_STATUS, status_value);
            uint32_t response;
            if (ot_.sendAndReceive(request, &response))
            {
                publishBinarySensor("ch_enable", enable);
                return true;
            }
            return false;
        }

        bool HAInterface::setDHWEnable(bool enable)
        {
            if (!status_valid_)
            {
                return false;
            }

            // Build status frame with updated DHW enable flag
            last_status_.dhw_enable = enable;
            uint16_t status_value = opentherm_encode_status(&last_status_);

            uint32_t request = opentherm_build_write_request(OT_DATA_ID_STATUS, status_value);
            uint32_t response;
            if (ot_.sendAndReceive(request, &response))
            {
                publishBinarySensor("dhw_enable", enable);
                return true;
            }
            return false;
        }

        bool HAInterface::setDeviceName(const char *name)
        {
            if (::Config::setDeviceName(name))
            {
                publishSensor("device_name", name);
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
                publishSensor("device_id", id);
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
                publishSensor("opentherm_tx_pin", (int)pin);
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
                publishSensor("opentherm_rx_pin", (int)pin);
                printf("OpenTherm RX pin updated to: GPIO%u - restarting in 2 seconds...\n", pin);
                sleep_ms(2000);
                watchdog_reboot(0, 0, 0);
                return true;
            }
            return false;
        }

    } // namespace HomeAssistant
} // namespace OpenTherm
