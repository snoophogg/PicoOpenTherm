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

                // Binary sensors
                Discovery::publishBinarySensor(config_, "fault", status.fault);
                Discovery::publishBinarySensor(config_, "ch_mode", status.ch_mode);
                Discovery::publishBinarySensor(config_, "dhw_mode", status.dhw_mode);
                Discovery::publishBinarySensor(config_, "flame", status.flame);
                Discovery::publishBinarySensor(config_, "cooling", status.cooling);
                Discovery::publishBinarySensor(config_, "ch2_mode", status.ch2_mode);
                Discovery::publishBinarySensor(config_, "diagnostic", status.diagnostic);

                // Switches (current state)
                Discovery::publishBinarySensor(config_, "ch_enable", status.ch_enable);
                Discovery::publishBinarySensor(config_, "dhw_enable", status.dhw_enable);
            }
        }

        void HAInterface::publishTemperatures()
        {
            float temp;

            if (ot_.readBoilerTemperature(&temp))
            {
                Discovery::publishSensor(config_, "boiler_temp", temp);
            }

            if (ot_.readDHWTemperature(&temp))
            {
                Discovery::publishSensor(config_, "dhw_temp", temp);
            }

            if (ot_.readReturnWaterTemperature(&temp))
            {
                Discovery::publishSensor(config_, "return_temp", temp);
            }

            if (ot_.readOutsideTemperature(&temp))
            {
                Discovery::publishSensor(config_, "outside_temp", temp);
            }

            if (ot_.readRoomTemperature(&temp))
            {
                Discovery::publishSensor(config_, "room_temp", temp);
            }

            int16_t exhaust_temp;
            if (ot_.readExhaustTemperature(&exhaust_temp))
            {
                Discovery::publishSensor(config_, "exhaust_temp", (int)exhaust_temp);
            }

            // Read setpoints
            if (ot_.readControlSetpoint(&temp))
            {
                Discovery::publishSensor(config_, "control_setpoint", temp);
            }

            if (ot_.readDHWSetpoint(&temp))
            {
                Discovery::publishSensor(config_, "dhw_setpoint", temp);
            }

            if (ot_.readMaxCHSetpoint(&temp))
            {
                Discovery::publishSensor(config_, "max_ch_setpoint", temp);
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
