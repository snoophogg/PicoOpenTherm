#include "simulated_opentherm.hpp"
#include <cmath>
#include <algorithm>
#include <cstdio>

namespace OpenTherm
{
    namespace Simulator
    {

        SimulatedInterface::SimulatedInterface() = default;

        // Simulate boiler temperature with sine wave + modulation heating effect
        float SimulatedInterface::readBoilerTemperature()
        {
            float base = 50.0f + 30.0f * std::sin(state_.time * 0.1f); // 50-80°C sine wave
            float heating_effect = state_.modulation * 0.3f;           // Up to +30°C when modulating
            return base + heating_effect;
        }

        // Simulate room temperature approaching setpoint
        float SimulatedInterface::readRoomTemperature()
        {
            float base = 18.0f + 4.0f * std::sin(state_.time * 0.05f); // 18-22°C slow sine wave
            // Gradually move toward setpoint
            float current = base;
            float diff = state_.room_setpoint - current;
            return current + (diff * 0.1f); // 10% toward setpoint each read
        }

        // DHW temperature
        float SimulatedInterface::readDHWTemperature()
        {
            if (state_.dhw_enabled)
            {
                return 55.0f + 10.0f * std::sin(state_.time * 0.15f); // 55-65°C
            }
            return 30.0f; // Cold when not heating
        }

        // Return water temperature (slightly cooler than boiler)
        float SimulatedInterface::readReturnWaterTemperature()
        {
            return readBoilerTemperature() - 10.0f;
        }

        // Outside temperature
        float SimulatedInterface::readOutsideTemperature()
        {
            return 10.0f + 8.0f * std::sin(state_.time * 0.02f); // 2-18°C daily variation
        }

        // Exhaust temperature (hotter when flame is on)
        int16_t SimulatedInterface::readExhaustTemperature()
        {
            if (state_.flame_on)
            {
                return state_.exhaust_temp + static_cast<int16_t>(state_.modulation * 0.5f); // Up to +50°C with modulation
            }
            return 50; // Ambient when off
        }

        // Modulation level (affected by temperature difference)
        float SimulatedInterface::readModulationLevel()
        {
            if (!state_.ch_enabled)
                return 0.0f;

            float temp_diff = state_.room_setpoint - readRoomTemperature();
            if (temp_diff > 0)
            {
                state_.modulation = std::min(100.0f, temp_diff * 20.0f); // 20% per degree
                state_.flame_on = state_.modulation > 5.0f;
            }
            else
            {
                state_.modulation = 0.0f;
                state_.flame_on = false;
            }

            return state_.modulation;
        }

        // CH water pressure (stable)
        float SimulatedInterface::readCHWaterPressure()
        {
            return 1.5f + 0.2f * std::sin(state_.time * 0.3f); // 1.3-1.7 bar
        }

        // DHW flow rate (higher when DHW is active)
        float SimulatedInterface::readDHWFlowRate()
        {
            if (state_.dhw_enabled && readDHWActive())
            {
                return state_.dhw_flow_rate + 2.0f * std::sin(state_.time * 0.5f); // 8-12 L/min variation
            }
            return 0.0f; // No flow when not active
        }

        // Flame status
        bool SimulatedInterface::readFlameStatus()
        {
            readModulationLevel(); // Update flame status
            return state_.flame_on;
        }

        // CH active
        bool SimulatedInterface::readCHActive()
        {
            return state_.ch_enabled && state_.flame_on;
        }

        // DHW active
        bool SimulatedInterface::readDHWActive()
        {
            return state_.dhw_enabled && (readDHWTemperature() < state_.dhw_setpoint);
        }

        // Setpoints
        float SimulatedInterface::readRoomSetpoint()
        {
            return state_.room_setpoint;
        }

        float SimulatedInterface::readDHWSetpoint()
        {
            return state_.dhw_setpoint;
        }

        float SimulatedInterface::readMaxCHSetpoint()
        {
            return state_.max_ch_setpoint;
        }

        bool SimulatedInterface::writeRoomSetpoint(float setpoint)
        {
            state_.room_setpoint = setpoint;
            printf("Simulator: Room setpoint set to %.1f°C\n", setpoint);
            return true;
        }

        bool SimulatedInterface::writeDHWSetpoint(float setpoint)
        {
            state_.dhw_setpoint = setpoint;
            printf("Simulator: DHW setpoint set to %.1f°C\n", setpoint);
            return true;
        }

        bool SimulatedInterface::writeMaxCHSetpoint(float setpoint)
        {
            // Validate range (20-80°C is typical)
            if (setpoint < 20.0f || setpoint > 80.0f)
                return false;

            state_.max_ch_setpoint = setpoint;
            printf("Simulator: Max CH setpoint set to %.1f°C\n", setpoint);
            return true;
        }

        // Status flags
        bool SimulatedInterface::readCHEnabled() { return state_.ch_enabled; }
        bool SimulatedInterface::readDHWEnabled() { return state_.dhw_enabled; }
        bool SimulatedInterface::readCoolingEnabled() { return state_.cooling_enabled; }

        bool SimulatedInterface::writeCHEnabled(bool enabled)
        {
            state_.ch_enabled = enabled;
            printf("Simulator: CH %s\n", enabled ? "enabled" : "disabled");
            return true;
        }

        bool SimulatedInterface::writeDHWEnabled(bool enabled)
        {
            state_.dhw_enabled = enabled;
            printf("Simulator: DHW %s\n", enabled ? "enabled" : "disabled");
            return true;
        }

        // Statistics (slowly incrementing)
        uint32_t SimulatedInterface::readBurnerStarts()
        {
            return state_.burner_starts;
        }

        uint32_t SimulatedInterface::readBurnerHours()
        {
            return state_.burner_hours;
        }

        uint32_t SimulatedInterface::readCHPumpHours()
        {
            return state_.ch_pump_hours;
        }

        uint32_t SimulatedInterface::readDHWPumpHours()
        {
            return state_.dhw_pump_hours;
        }

        uint32_t SimulatedInterface::readCHPumpStarts()
        {
            return state_.ch_pump_starts;
        }

        uint32_t SimulatedInterface::readDHWPumpStarts()
        {
            return state_.dhw_pump_starts;
        }

        // Max modulation (fixed)
        float SimulatedInterface::readMaxModulationLevel()
        {
            return 100.0f;
        }

        // Fault/diagnostic codes (none in simulator)
        uint16_t SimulatedInterface::readOEMFaultCode() { return 0; }
        uint16_t SimulatedInterface::readOEMDiagnosticCode() { return 0; }

        // Time and date read functions
        bool SimulatedInterface::readDayTime(uint8_t *day_of_week, uint8_t *hours, uint8_t *minutes)
        {
            if (!day_of_week || !hours || !minutes)
                return false;

            *day_of_week = state_.day_of_week;
            *hours = state_.hours;
            *minutes = state_.minutes;
            return true;
        }

        bool SimulatedInterface::readDate(uint8_t *month, uint8_t *day)
        {
            if (!month || !day)
                return false;

            *month = state_.month;
            *day = state_.day;
            return true;
        }

        bool SimulatedInterface::readYear(uint16_t *year)
        {
            if (!year)
                return false;

            *year = state_.year;
            return true;
        }

        // Time and date write functions
        bool SimulatedInterface::writeDayTime(uint8_t day_of_week, uint8_t hours, uint8_t minutes)
        {
            // Validate inputs
            if (day_of_week > 7 || hours > 23 || minutes > 59)
                return false;

            state_.day_of_week = day_of_week;
            state_.hours = hours;
            state_.minutes = minutes;
            printf("Simulator: Time set to day=%u %02u:%02u\n", day_of_week, hours, minutes);
            return true;
        }

        bool SimulatedInterface::writeDate(uint8_t month, uint8_t day)
        {
            // Validate inputs
            if (month < 1 || month > 12 || day < 1 || day > 31)
                return false;

            state_.month = month;
            state_.day = day;
            printf("Simulator: Date set to %02u/%02u\n", month, day);
            return true;
        }

        bool SimulatedInterface::writeYear(uint16_t year)
        {
            // Validate year (reasonable range)
            if (year < 2000 || year > 2099)
                return false;

            state_.year = year;
            printf("Simulator: Year set to %u\n", year);
            return true;
        }

        // Update simulator state
        void SimulatedInterface::update(float time_seconds)
        {
            // Track elapsed real time for date/time increment
            static float last_time = 0.0f;
            float elapsed = time_seconds - last_time;
            last_time = time_seconds;

            state_.time = time_seconds;

            // Increment date/time based on elapsed real time
            // Note: time_seconds is typically updated every second
            if (elapsed > 0.0f && elapsed < 10.0f) // Sanity check
            {
                static float accumulated_seconds = 0.0f;
                accumulated_seconds += elapsed;

                // Increment minutes every 60 seconds
                while (accumulated_seconds >= 60.0f)
                {
                    accumulated_seconds -= 60.0f;
                    state_.minutes++;

                    if (state_.minutes >= 60)
                    {
                        state_.minutes = 0;
                        state_.hours++;

                        if (state_.hours >= 24)
                        {
                            state_.hours = 0;
                            state_.day_of_week++;
                            if (state_.day_of_week > 7)
                            {
                                state_.day_of_week = 1; // Monday
                            }
                            state_.day++;

                            // Days per month (simplified - doesn't handle leap years perfectly)
                            uint8_t days_in_month[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
                            uint8_t max_days = (state_.month >= 1 && state_.month <= 12) ? days_in_month[state_.month] : 31;

                            // Simple leap year check for February
                            if (state_.month == 2 && (state_.year % 4 == 0))
                            {
                                max_days = 29;
                            }

                            if (state_.day > max_days)
                            {
                                state_.day = 1;
                                state_.month++;

                                if (state_.month > 12)
                                {
                                    state_.month = 1;
                                    state_.year++;
                                }
                            }
                        }
                    }
                }
            }

            // Increment statistics slowly
            if (state_.flame_on)
            {
                static uint32_t flame_time = 0;
                flame_time++;
                if (flame_time >= 36000)
                { // ~1 hour of flame time
                    state_.burner_hours++;
                    flame_time = 0;
                }
            }

            if (state_.ch_enabled)
            {
                static uint32_t ch_time = 0;
                ch_time++;
                if (ch_time >= 36000)
                {
                    state_.ch_pump_hours++;
                    ch_time = 0;
                }
            }

            if (state_.dhw_enabled)
            {
                static uint32_t dhw_time = 0;
                dhw_time++;
                if (dhw_time >= 36000)
                {
                    state_.dhw_pump_hours++;
                    dhw_time = 0;
                }
            }
        }

    } // namespace Simulator
} // namespace OpenTherm
