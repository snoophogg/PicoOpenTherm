#ifndef OPENTHERM_HPP
#define OPENTHERM_HPP

#include <cstdint>
#include "hardware/pio.h"
#include "opentherm_protocol.hpp"
#include "opentherm_base.hpp"

// C++ OpenTherm Interface
namespace OpenTherm
{

    class Interface : public BaseInterface
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
        void setTimeout(uint32_t timeout_ms) override { timeout_ms_ = timeout_ms; }
        uint32_t getTimeout() const override { return timeout_ms_; }

        // Send an OpenTherm frame
        void send(uint32_t frame);

        // Receive an OpenTherm frame (non-blocking)
        bool receive(uint32_t &frame);

        // Print frame details
        static void printFrame(uint32_t frame_data);

        // High-level read functions with automatic request/response handling
        // Returns true if successful, false if timeout or error

        // Status and configuration reads
        bool readStatus(opentherm_status_t *status) override;
        bool readSlaveConfig(opentherm_config_t *config) override;
        bool readFaultFlags(opentherm_fault_t *fault) override;
        bool readOemDiagnosticCode(uint16_t *diag_code) override;

        // Temperature sensor reads (returns temperature in °C)
        bool readBoilerTemperature(float *temp) override;
        bool readDHWTemperature(float *temp) override;
        bool readOutsideTemperature(float *temp) override;
        bool readReturnWaterTemperature(float *temp) override;
        bool readRoomTemperature(float *temp) override;
        bool readExhaustTemperature(int16_t *temp) override;

        // Pressure and flow reads
        bool readCHWaterPressure(float *pressure) override; // bar
        bool readDHWFlowRate(float *flow_rate) override;    // l/min

        // Modulation level read (percentage)
        bool readModulationLevel(float *level) override;
        bool readMaxModulationLevel(float *level) override;

        // Setpoint reads
        bool readControlSetpoint(float *setpoint) override;
        bool readDHWSetpoint(float *setpoint) override;
        bool readMaxCHSetpoint(float *setpoint) override;

        // Counter/statistics reads
        bool readBurnerStarts(uint16_t *count) override;
        bool readCHPumpStarts(uint16_t *count) override;
        bool readDHWPumpStarts(uint16_t *count) override;
        bool readBurnerHours(uint16_t *hours) override;
        bool readCHPumpHours(uint16_t *hours) override;
        bool readDHWPumpHours(uint16_t *hours) override;

        // Version information reads
        bool readOpenThermVersion(float *version) override;
        bool readSlaveVersion(uint8_t *type, uint8_t *version) override;

        // Time and date reads
        bool readDayTime(uint8_t *day_of_week, uint8_t *hours, uint8_t *minutes) override;
        bool readDate(uint8_t *month, uint8_t *day) override;
        bool readYear(uint16_t *year) override;

        // Temperature bounds reads
        bool readDHWBounds(uint8_t *min_temp, uint8_t *max_temp) override;
        bool readCHBounds(uint8_t *min_temp, uint8_t *max_temp) override;

        // Write functions
        bool writeControlSetpoint(float temperature) override;
        bool writeRoomSetpoint(float temperature) override;
        bool writeDHWSetpoint(float temperature) override;
        bool writeMaxCHSetpoint(float temperature) override;
        bool writeCHEnable(bool enable) override;
        bool writeDHWEnable(bool enable) override;

        // Time and date writes
        bool writeDayTime(uint8_t day_of_week, uint8_t hours, uint8_t minutes) override;
        bool writeDate(uint8_t month, uint8_t day) override;
        bool writeYear(uint16_t year) override;

        // Low-level request/response handling
        // Sends a request frame and waits for response
        bool sendAndReceive(uint32_t request, uint32_t *response);

    private:
    };

} // namespace OpenTherm

#endif // OPENTHERM_HPP
