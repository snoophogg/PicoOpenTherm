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

                // Simulator doesn't have exhaust temp, return 0
                *temp = 0;
                return false;
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

                // Simulator doesn't have flow rate, return 0
                *flow_rate = 0.0f;
                return false;
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

                *setpoint = sim_.readMaxModulationLevel();
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

                // Simulator doesn't track pump starts separately
                *count = 0;
                return false;
            }

            bool readDHWPumpStarts(uint16_t *count) override
            {
                if (!count)
                    return false;

                // Simulator doesn't track DHW pump starts
                *count = 0;
                return false;
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
                // Simulator doesn't support max CH setpoint
                return false;
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

            // Low-level request/response handling
            bool sendAndReceive(uint32_t request, uint32_t *response) override
            {
                // Simulator doesn't implement low-level frame handling
                // For now, return false for unsupported operations
                // In the future, we could simulate responses based on request type
                if (!response)
                    return false;

                // Parse request to determine what's being asked
                uint8_t msg_type = (request >> 28) & 0x07;
                uint8_t data_id = (request >> 16) & 0xFF;

                // We could implement specific responses here based on data_id
                // For now, return false to indicate not supported
                (void)msg_type;
                (void)data_id;
                *response = 0;
                return false;
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
