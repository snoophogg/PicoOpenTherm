#ifndef OPENTHERM_BASE_HPP
#define OPENTHERM_BASE_HPP

#include <cstdint>
#include "opentherm_protocol.hpp"

namespace OpenTherm
{
    /**
     * @brief Base interface for OpenTherm communication
     *
     * This abstract base class defines the interface for OpenTherm communication.
     * It can be implemented by hardware-based OpenTherm interfaces or simulators.
     */
    class BaseInterface
    {
    public:
        virtual ~BaseInterface() = default;

        // Status and configuration reads
        virtual bool readStatus(opentherm_status_t *status) = 0;
        virtual bool readSlaveConfig(opentherm_config_t *config) = 0;
        virtual bool readFaultFlags(opentherm_fault_t *fault) = 0;
        virtual bool readOemDiagnosticCode(uint16_t *diag_code) = 0;

        // Temperature sensor reads (returns temperature in °C)
        virtual bool readBoilerTemperature(float *temp) = 0;
        virtual bool readDHWTemperature(float *temp) = 0;
        virtual bool readOutsideTemperature(float *temp) = 0;
        virtual bool readReturnWaterTemperature(float *temp) = 0;
        virtual bool readRoomTemperature(float *temp) = 0;
        virtual bool readExhaustTemperature(int16_t *temp) = 0;

        // Pressure and flow reads
        virtual bool readCHWaterPressure(float *pressure) = 0;
        virtual bool readDHWFlowRate(float *flow_rate) = 0;

        // Modulation level read (percentage)
        virtual bool readModulationLevel(float *level) = 0;

        // Setpoint reads
        virtual bool readControlSetpoint(float *setpoint) = 0;
        virtual bool readDHWSetpoint(float *setpoint) = 0;
        virtual bool readMaxCHSetpoint(float *setpoint) = 0;

        // Counter/statistics reads
        virtual bool readBurnerStarts(uint16_t *count) = 0;
        virtual bool readCHPumpStarts(uint16_t *count) = 0;
        virtual bool readDHWPumpStarts(uint16_t *count) = 0;
        virtual bool readBurnerHours(uint16_t *hours) = 0;
        virtual bool readCHPumpHours(uint16_t *hours) = 0;
        virtual bool readDHWPumpHours(uint16_t *hours) = 0;

        // Version information reads
        virtual bool readOpenThermVersion(float *version) = 0;
        virtual bool readSlaveVersion(uint8_t *type, uint8_t *version) = 0;

        // Write functions
        virtual bool writeControlSetpoint(float temperature) = 0;
        virtual bool writeRoomSetpoint(float temperature) = 0;
        virtual bool writeDHWSetpoint(float temperature) = 0;
        virtual bool writeMaxCHSetpoint(float temperature) = 0;

        // Timeout configuration
        virtual void setTimeout(uint32_t timeout_ms) = 0;
        virtual uint32_t getTimeout() const = 0;

        // Low-level request/response handling
        // Sends a request frame and waits for response
        virtual bool sendAndReceive(uint32_t request, uint32_t *response) = 0;
    };

} // namespace OpenTherm

#endif // OPENTHERM_BASE_HPP
