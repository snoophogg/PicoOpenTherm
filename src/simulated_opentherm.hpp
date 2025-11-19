#pragma once

#include <cstdint>

namespace OpenTherm
{
    namespace Simulator
    {

        // Simulator state structure
        struct SimulatorState
        {
            float time = 0.0f; // Time in seconds for sine wave
            float room_setpoint = 20.0f;
            float dhw_setpoint = 60.0f;
            bool ch_enabled = true;
            bool dhw_enabled = true;
            bool cooling_enabled = false;
            bool flame_on = false;
            float modulation = 0.0f;
            uint32_t burner_starts = 0;
            uint32_t burner_hours = 0;
            uint32_t ch_pump_hours = 0;
            uint32_t dhw_pump_hours = 0;

            // Time and date storage
            uint8_t day_of_week = 1;  // Monday
            uint8_t hours = 12;
            uint8_t minutes = 0;
            uint8_t month = 1;
            uint8_t day = 1;
            uint16_t year = 2025;
        };

        // Simulated OpenTherm Interface - no actual hardware
        class SimulatedInterface
        {
        public:
            SimulatedInterface();

            // Temperature readings
            float readBoilerTemperature();
            float readRoomTemperature();
            float readDHWTemperature();
            float readReturnWaterTemperature();
            float readOutsideTemperature();

            // Pressure and modulation
            float readCHWaterPressure();
            float readModulationLevel();
            float readMaxModulationLevel();

            // Status flags
            bool readFlameStatus();
            bool readCHActive();
            bool readDHWActive();
            bool readCHEnabled();
            bool readDHWEnabled();
            bool readCoolingEnabled();

            // Setpoints (read)
            float readRoomSetpoint();
            float readDHWSetpoint();

            // Setpoints (write)
            bool writeRoomSetpoint(float setpoint);
            bool writeDHWSetpoint(float setpoint);

            // Control
            bool writeCHEnabled(bool enabled);
            bool writeDHWEnabled(bool enabled);

            // Statistics
            uint32_t readBurnerStarts();
            uint32_t readBurnerHours();
            uint32_t readCHPumpHours();
            uint32_t readDHWPumpHours();

            // Fault/diagnostic codes (none in simulator)
            uint16_t readOEMFaultCode();
            uint16_t readOEMDiagnosticCode();

            // Time and date functions
            bool readDayTime(uint8_t *day_of_week, uint8_t *hours, uint8_t *minutes);
            bool readDate(uint8_t *month, uint8_t *day);
            bool readYear(uint16_t *year);
            bool writeDayTime(uint8_t day_of_week, uint8_t hours, uint8_t minutes);
            bool writeDate(uint8_t month, uint8_t day);
            bool writeYear(uint16_t year);

            // Update simulator state (call periodically)
            void update(float time_seconds);

        private:
            SimulatorState state_;
        };

    } // namespace Simulator
} // namespace OpenTherm
