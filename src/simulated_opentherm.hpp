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

            // Update simulator state (call periodically)
            void update(float time_seconds);

        private:
            SimulatorState state_;
        };

    } // namespace Simulator
} // namespace OpenTherm
