#include "opentherm_ha.hpp"
#include "config.hpp"
#include "mqtt_discovery.hpp"
#include "mqtt_topics.hpp"
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
            using namespace OpenTherm::MQTTTopics;
            std::string base_cmd = config_.command_topic_base;
            mqtt_.subscribe((base_cmd + "/" + CH_ENABLE).c_str());
            mqtt_.subscribe((base_cmd + "/" + DHW_ENABLE).c_str());
            mqtt_.subscribe((base_cmd + "/" + CONTROL_SETPOINT).c_str());
            mqtt_.subscribe((base_cmd + "/" + ROOM_SETPOINT).c_str());
            mqtt_.subscribe((base_cmd + "/" + DHW_SETPOINT).c_str());
            mqtt_.subscribe((base_cmd + "/" + MAX_CH_SETPOINT).c_str());
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
            uint32_t request = opentherm_read_max_rel_mod();
            uint32_t response;
            if (ot_.sendAndReceive(request, &response))
            {
                float max_mod = opentherm_get_f8_8(response);
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

    } // namespace HomeAssistant
} // namespace OpenTherm
