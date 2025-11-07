#ifndef OPENTHERM_HPP
#define OPENTHERM_HPP

#include <cstdint>
#include "hardware/pio.h"
#include "opentherm_protocol.hpp"

// C++ OpenTherm Interface
namespace OpenTherm
{

    class Interface
    {
    private:
        PIO pio_tx_;
        PIO pio_rx_;
        unsigned int sm_tx_;
        unsigned int sm_rx_;
        unsigned int tx_pin_;
        unsigned int rx_pin_;
        uint32_t timeout_ms_;

    public:
        // Constructor
        Interface(unsigned int tx_pin, unsigned int rx_pin, PIO pio_tx = pio0, PIO pio_rx = pio1);

        // Set/get timeout for read/write operations (default 1000ms)
        void setTimeout(uint32_t timeout_ms) { timeout_ms_ = timeout_ms; }
        uint32_t getTimeout() const { return timeout_ms_; }

        // Send an OpenTherm frame
        void send(uint32_t frame);

        // Receive an OpenTherm frame (non-blocking)
        bool receive(uint32_t &frame);

        // Print frame details
        static void printFrame(uint32_t frame_data);

        // High-level read functions with automatic request/response handling
        // Returns true if successful, false if timeout or error

        // Status and configuration reads
        bool readStatus(opentherm_status_t *status);
        bool readSlaveConfig(opentherm_config_t *config);
        bool readFaultFlags(opentherm_fault_t *fault);

        // Temperature sensor reads (returns temperature in °C)
        bool readBoilerTemperature(float *temp);
        bool readDHWTemperature(float *temp);
        bool readOutsideTemperature(float *temp);
        bool readReturnWaterTemperature(float *temp);
        bool readRoomTemperature(float *temp);
        bool readExhaustTemperature(int16_t *temp);

        // Pressure and flow reads
        bool readCHWaterPressure(float *pressure); // bar
        bool readDHWFlowRate(float *flow_rate);    // l/min

        // Modulation level read (percentage)
        bool readModulationLevel(float *level);

        // Setpoint reads
        bool readControlSetpoint(float *setpoint);
        bool readDHWSetpoint(float *setpoint);
        bool readMaxCHSetpoint(float *setpoint);

        // Counter/statistics reads
        bool readBurnerStarts(uint16_t *count);
        bool readCHPumpStarts(uint16_t *count);
        bool readDHWPumpStarts(uint16_t *count);
        bool readBurnerHours(uint16_t *hours);
        bool readCHPumpHours(uint16_t *hours);
        bool readDHWPumpHours(uint16_t *hours);

        // Version information reads
        bool readOpenThermVersion(float *version);
        bool readSlaveVersion(uint8_t *type, uint8_t *version);

        // Write functions
        bool writeControlSetpoint(float temperature);
        bool writeRoomSetpoint(float temperature);
        bool writeDHWSetpoint(float temperature);
        bool writeMaxCHSetpoint(float temperature);

        // Helper function to send request and wait for response
        bool sendAndReceive(uint32_t request, uint32_t *response);

    private:
    };

} // namespace OpenTherm

#endif // OPENTHERM_HPP
