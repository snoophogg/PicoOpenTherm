#ifndef SIMULATED_OPENTHERM_ADAPTER_HPP
#define SIMULATED_OPENTHERM_ADAPTER_HPP

#include "opentherm_base.hpp"
#include "simulated_opentherm.hpp"
#include <cstring>

namespace OpenTherm
{
    namespace Simulator
    {
        /**
         * @brief Adapter to make SimulatedInterface compatible with BaseInterface
         *
         * This adapter wraps the SimulatedInterface and implements the BaseInterface
         * API, allowing the simulator to be used with HAInterface and other components
         * that expect a BaseInterface.
         */
        class SimulatedInterfaceAdapter : public BaseInterface
        {
        public:
            SimulatedInterfaceAdapter(SimulatedInterface &sim) : sim_(sim), timeout_ms_(1000) {}

            // Status and configuration reads
            bool readStatus(opentherm_status_t *status) override
            {
                if (!status)
                    return false;

                memset(status, 0, sizeof(opentherm_status_t));
                status->fault = false;
                status->ch_mode = sim_.readCHActive();
                status->dhw_mode = sim_.readDHWActive();
                status->flame = sim_.readFlameStatus();
                status->cooling = sim_.readCoolingEnabled();
                status->ch2_mode = false;
                status->diagnostic = false;
                return true;
            }

            bool readSlaveConfig(opentherm_config_t *config) override
            {
                if (!config)
                    return false;

                memset(config, 0, sizeof(opentherm_config_t));
                config->dhw_present = true;
                config->control_type = false; // Modulating
                config->cooling_config = false;
                config->dhw_config = true;
                config->master_pump_control = false;
                config->ch2_present = false;
                return true;
            }

            bool readFaultFlags(opentherm_fault_t *fault) override
            {
                if (!fault)
                    return false;

                memset(fault, 0, sizeof(opentherm_fault_t));
                // Simulator has no faults
                return true;
            }

            bool readOemDiagnosticCode(uint16_t *diag_code) override
            {
                if (!diag_code)
                    return false;

                *diag_code = sim_.readOEMDiagnosticCode();
                return true;
            }

            // Temperature sensor reads (returns temperature in °C)
            bool readBoilerTemperature(float *temp) override
            {
                if (!temp)
                    return false;

                *temp = sim_.readBoilerTemperature();
                return true;
            }

            bool readDHWTemperature(float *temp) override
            {
                if (!temp)
                    return false;

                *temp = sim_.readDHWTemperature();
                return true;
            }

            bool readOutsideTemperature(float *temp) override
            {
                if (!temp)
                    return false;

                *temp = sim_.readOutsideTemperature();
                return true;
            }

            bool readReturnWaterTemperature(float *temp) override
            {
                if (!temp)
                    return false;

                *temp = sim_.readReturnWaterTemperature();
                return true;
            }

            bool readRoomTemperature(float *temp) override
            {
                if (!temp)
                    return false;

                *temp = sim_.readRoomTemperature();
                return true;
            }

            bool readExhaustTemperature(int16_t *temp) override
            {
                if (!temp)
                    return false;

                *temp = sim_.readExhaustTemperature();
                return true;
            }

            // Pressure and flow reads
            bool readCHWaterPressure(float *pressure) override
            {
                if (!pressure)
                    return false;

                *pressure = sim_.readCHWaterPressure();
                return true;
            }

            bool readDHWFlowRate(float *flow_rate) override
            {
                if (!flow_rate)
                    return false;

                *flow_rate = sim_.readDHWFlowRate();
                return true;
            }

            // Modulation level read (percentage)
            bool readModulationLevel(float *level) override
            {
                if (!level)
                    return false;

                *level = sim_.readModulationLevel();
                return true;
            }

            // Setpoint reads
            bool readControlSetpoint(float *setpoint) override
            {
                if (!setpoint)
                    return false;

                // Use room setpoint as control setpoint in simulator
                *setpoint = sim_.readRoomSetpoint();
                return true;
            }

            bool readDHWSetpoint(float *setpoint) override
            {
                if (!setpoint)
                    return false;

                *setpoint = sim_.readDHWSetpoint();
                return true;
            }

            bool readMaxCHSetpoint(float *setpoint) override
            {
                if (!setpoint)
                    return false;

                *setpoint = sim_.readMaxCHSetpoint();
                return true;
            }

            // Counter/statistics reads
            bool readBurnerStarts(uint16_t *count) override
            {
                if (!count)
                    return false;

                *count = (uint16_t)sim_.readBurnerStarts();
                return true;
            }

            bool readCHPumpStarts(uint16_t *count) override
            {
                if (!count)
                    return false;

                *count = (uint16_t)sim_.readCHPumpStarts();
                return true;
            }

            bool readDHWPumpStarts(uint16_t *count) override
            {
                if (!count)
                    return false;

                *count = (uint16_t)sim_.readDHWPumpStarts();
                return true;
            }

            bool readBurnerHours(uint16_t *hours) override
            {
                if (!hours)
                    return false;

                *hours = (uint16_t)sim_.readBurnerHours();
                return true;
            }

            bool readCHPumpHours(uint16_t *hours) override
            {
                if (!hours)
                    return false;

                *hours = (uint16_t)sim_.readCHPumpHours();
                return true;
            }

            bool readDHWPumpHours(uint16_t *hours) override
            {
                if (!hours)
                    return false;

                *hours = (uint16_t)sim_.readDHWPumpHours();
                return true;
            }

            // Version information reads
            bool readOpenThermVersion(float *version) override
            {
                if (!version)
                    return false;

                *version = 2.2f; // Simulator reports OpenTherm 2.2
                return true;
            }

            bool readSlaveVersion(uint8_t *type, uint8_t *version) override
            {
                if (!type || !version)
                    return false;

                *type = 1;    // Simulator type
                *version = 1; // Version 1
                return true;
            }

            // Write functions
            bool writeControlSetpoint(float temperature) override
            {
                return sim_.writeRoomSetpoint(temperature);
            }

            bool writeRoomSetpoint(float temperature) override
            {
                return sim_.writeRoomSetpoint(temperature);
            }

            bool writeDHWSetpoint(float temperature) override
            {
                return sim_.writeDHWSetpoint(temperature);
            }

            bool writeMaxCHSetpoint(float temperature) override
            {
                return sim_.writeMaxCHSetpoint(temperature);
            }

            bool writeCHEnable(bool enable) override
            {
                return sim_.writeCHEnabled(enable);
            }

            bool writeDHWEnable(bool enable) override
            {
                return sim_.writeDHWEnabled(enable);
            }

            bool readMaxModulationLevel(float *level) override
            {
                if (!level)
                    return false;
                *level = sim_.readMaxModulationLevel();
                return true;
            }

            bool readDayTime(uint8_t *day_of_week, uint8_t *hours, uint8_t *minutes) override
            {
                return sim_.readDayTime(day_of_week, hours, minutes);
            }

            bool readDate(uint8_t *month, uint8_t *day) override
            {
                return sim_.readDate(month, day);
            }

            bool readYear(uint16_t *year) override
            {
                return sim_.readYear(year);
            }

            bool readDHWBounds(uint8_t *min_temp, uint8_t *max_temp) override
            {
                // Simulator doesn't support bounds - return reasonable defaults
                if (!min_temp || !max_temp)
                    return false;
                *min_temp = 40;
                *max_temp = 65;
                return true;
            }

            bool readCHBounds(uint8_t *min_temp, uint8_t *max_temp) override
            {
                // Simulator doesn't support bounds - return reasonable defaults
                if (!min_temp || !max_temp)
                    return false;
                *min_temp = 20;
                *max_temp = 80;
                return true;
            }

            bool writeDayTime(uint8_t day_of_week, uint8_t hours, uint8_t minutes) override
            {
                return sim_.writeDayTime(day_of_week, hours, minutes);
            }

            bool writeDate(uint8_t month, uint8_t day) override
            {
                return sim_.writeDate(month, day);
            }

            bool writeYear(uint16_t year) override
            {
                return sim_.writeYear(year);
            }

            // Timeout configuration
            void setTimeout(uint32_t timeout_ms) override
            {
                timeout_ms_ = timeout_ms;
            }

            uint32_t getTimeout() const override
            {
                return timeout_ms_;
            }

            // Access to underlying simulator
            SimulatedInterface &getSimulator() { return sim_; }

        private:
            SimulatedInterface &sim_;
            uint32_t timeout_ms_;
        };

    } // namespace Simulator
} // namespace OpenTherm

#endif // SIMULATED_OPENTHERM_ADAPTER_HPP
