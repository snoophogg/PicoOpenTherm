/**
 * Unit tests for OpenTherm Simulator
 *
 * These tests validate the simulated OpenTherm boiler behavior
 * without requiring actual hardware.
 */

#include "../src/simulated_opentherm.hpp"
#include <cstdio>
#include <cmath>

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name)                             \
    void test_##name();                        \
    void run_test_##name()                     \
    {                                          \
        printf("Running test: %s... ", #name); \
        tests_run++;                           \
        test_##name();                         \
        tests_passed++;                        \
        printf("PASSED\n");                    \
    }                                          \
    void test_##name()

#define ASSERT(condition)                                                                           \
    do                                                                                              \
    {                                                                                               \
        if (!(condition))                                                                           \
        {                                                                                           \
            printf("FAILED\n  Assertion failed: %s\n  at %s:%d\n", #condition, __FILE__, __LINE__); \
            return;                                                                                 \
        }                                                                                           \
    } while (0)

#define ASSERT_RANGE(value, min, max)                                                  \
    do                                                                                 \
    {                                                                                  \
        if ((value) < (min) || (value) > (max))                                        \
        {                                                                              \
            printf("FAILED\n  Value %f out of range [%f, %f]\n  at %s:%d\n",           \
                   (double)(value), (double)(min), (double)(max), __FILE__, __LINE__); \
            return;                                                                    \
        }                                                                              \
    } while (0)

#define ASSERT_NEAR(actual, expected, tolerance)                                                   \
    do                                                                                             \
    {                                                                                              \
        float diff = fabs((actual) - (expected));                                                  \
        if (diff > (tolerance))                                                                    \
        {                                                                                          \
            printf("FAILED\n  Expected: %f\n  Actual: %f\n  Tolerance: %f\n  at %s:%d\n",          \
                   (double)(expected), (double)(actual), (double)(tolerance), __FILE__, __LINE__); \
            return;                                                                                \
        }                                                                                          \
    } while (0)

using namespace OpenTherm::Simulator;

// ============================================================================
// Initial State Tests
// ============================================================================

TEST(simulator_initial_state)
{
    SimulatedInterface sim;

    // CH should be enabled by default
    ASSERT(sim.readCHEnabled() == true);

    // DHW should be enabled by default
    ASSERT(sim.readDHWEnabled() == true);

    // Cooling should be disabled
    ASSERT(sim.readCoolingEnabled() == false);

    // Default setpoints
    ASSERT_NEAR(sim.readRoomSetpoint(), 20.0f, 0.1f);
    ASSERT_NEAR(sim.readDHWSetpoint(), 60.0f, 0.1f);
}

// ============================================================================
// Temperature Reading Tests
// ============================================================================

TEST(boiler_temperature_in_range)
{
    SimulatedInterface sim;

    for (int i = 0; i < 100; i++)
    {
        float temp = sim.readBoilerTemperature();
        ASSERT_RANGE(temp, 40.0f, 120.0f); // Reasonable boiler temperature
        sim.update();
    }
}

TEST(room_temperature_in_range)
{
    SimulatedInterface sim;

    for (int i = 0; i < 100; i++)
    {
        float temp = sim.readRoomTemperature();
        ASSERT_RANGE(temp, 15.0f, 30.0f); // Reasonable room temperature
        sim.update();
    }
}

TEST(dhw_temperature_in_range)
{
    SimulatedInterface sim;

    for (int i = 0; i < 100; i++)
    {
        float temp = sim.readDHWTemperature();
        ASSERT_RANGE(temp, 25.0f, 70.0f); // DHW temperature range
        sim.update();
    }
}

TEST(outside_temperature_in_range)
{
    SimulatedInterface sim;

    for (int i = 0; i < 100; i++)
    {
        float temp = sim.readOutsideTemperature();
        ASSERT_RANGE(temp, 0.0f, 25.0f); // Outside temperature range
        sim.update();
    }
}

TEST(return_water_cooler_than_boiler)
{
    SimulatedInterface sim;

    for (int i = 0; i < 50; i++)
    {
        float boiler_temp = sim.readBoilerTemperature();
        float return_temp = sim.readReturnWaterTemperature();
        ASSERT(return_temp < boiler_temp);
        sim.update();
    }
}

// ============================================================================
// Pressure Tests
// ============================================================================

TEST(ch_water_pressure_in_range)
{
    SimulatedInterface sim;

    for (int i = 0; i < 100; i++)
    {
        float pressure = sim.readCHWaterPressure();
        ASSERT_RANGE(pressure, 1.0f, 2.0f); // Typical CH pressure 1-2 bar
        sim.update();
    }
}

// ============================================================================
// Modulation Tests
// ============================================================================

TEST(modulation_zero_when_ch_disabled)
{
    SimulatedInterface sim;
    sim.writeCHEnabled(false);

    for (int i = 0; i < 10; i++)
    {
        float modulation = sim.readModulationLevel();
        ASSERT_NEAR(modulation, 0.0f, 0.1f);
        sim.update();
    }
}

TEST(modulation_in_range)
{
    SimulatedInterface sim;
    sim.writeCHEnabled(true);

    for (int i = 0; i < 100; i++)
    {
        float modulation = sim.readModulationLevel();
        ASSERT_RANGE(modulation, 0.0f, 100.0f);
        sim.update();
    }
}

TEST(max_modulation_level)
{
    SimulatedInterface sim;

    float max_mod = sim.readMaxModulationLevel();
    ASSERT_NEAR(max_mod, 100.0f, 0.1f);
}

// ============================================================================
// Flame Status Tests
// ============================================================================

TEST(flame_off_when_ch_disabled)
{
    SimulatedInterface sim;
    sim.writeCHEnabled(false);

    for (int i = 0; i < 20; i++)
    {
        sim.readModulationLevel(); // Update modulation/flame state
        ASSERT(sim.readFlameStatus() == false);
        sim.update();
    }
}

TEST(ch_active_requires_ch_enabled_and_flame)
{
    SimulatedInterface sim;

    // Disable CH - should not be active
    sim.writeCHEnabled(false);
    ASSERT(sim.readCHActive() == false);

    // Enable CH - might be active depending on flame
    sim.writeCHEnabled(true);
    bool ch_active = sim.readCHActive();
    bool flame = sim.readFlameStatus();

    // CH active should match flame status when enabled
    ASSERT(ch_active == flame);
}

// ============================================================================
// Setpoint Tests
// ============================================================================

TEST(write_read_room_setpoint)
{
    SimulatedInterface sim;

    float new_setpoint = 22.5f;
    sim.writeRoomSetpoint(new_setpoint);

    float read_setpoint = sim.readRoomSetpoint();
    ASSERT_NEAR(read_setpoint, new_setpoint, 0.1f);
}

TEST(write_read_dhw_setpoint)
{
    SimulatedInterface sim;

    float new_setpoint = 55.0f;
    sim.writeDHWSetpoint(new_setpoint);

    float read_setpoint = sim.readDHWSetpoint();
    ASSERT_NEAR(read_setpoint, new_setpoint, 0.1f);
}

TEST(room_temperature_approaches_setpoint)
{
    SimulatedInterface sim;

    float initial_temp = sim.readRoomTemperature();
    float setpoint = initial_temp + 5.0f;
    sim.writeRoomSetpoint(setpoint);
    sim.writeCHEnabled(true);

    // Run simulation for many steps
    float prev_diff = fabs(setpoint - initial_temp);
    for (int i = 0; i < 100; i++)
    {
        sim.update();
        float temp = sim.readRoomTemperature();
        float diff = fabs(setpoint - temp);

        // Difference should generally decrease or stay similar
        // (allowing for sine wave variation)
        ASSERT(diff < prev_diff + 2.0f);
        prev_diff = diff;
    }
}

// ============================================================================
// Control Tests
// ============================================================================

TEST(enable_disable_ch)
{
    SimulatedInterface sim;

    sim.writeCHEnabled(true);
    ASSERT(sim.readCHEnabled() == true);

    sim.writeCHEnabled(false);
    ASSERT(sim.readCHEnabled() == false);
}

TEST(enable_disable_dhw)
{
    SimulatedInterface sim;

    sim.writeDHWEnabled(true);
    ASSERT(sim.readDHWEnabled() == true);

    sim.writeDHWEnabled(false);
    ASSERT(sim.readDHWEnabled() == false);
}

TEST(dhw_temperature_lower_when_disabled)
{
    SimulatedInterface sim;

    sim.writeDHWEnabled(true);
    sim.update();
    float temp_enabled = sim.readDHWTemperature();

    sim.writeDHWEnabled(false);
    sim.update();
    float temp_disabled = sim.readDHWTemperature();

    // DHW should be cooler when disabled
    ASSERT(temp_disabled < temp_enabled);
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST(burner_stats_non_negative)
{
    SimulatedInterface sim;

    ASSERT(sim.readBurnerStarts() >= 0);
    ASSERT(sim.readBurnerHours() >= 0);
    ASSERT(sim.readCHPumpHours() >= 0);
    ASSERT(sim.readDHWPumpHours() >= 0);
}

// ============================================================================
// Fault/Diagnostic Tests
// ============================================================================

TEST(no_faults_in_simulator)
{
    SimulatedInterface sim;

    // Simulator should have no faults
    ASSERT(sim.readOEMFaultCode() == 0);
    ASSERT(sim.readOEMDiagnosticCode() == 0);
}

// ============================================================================
// Update Mechanism Tests
// ============================================================================

TEST(update_advances_simulation)
{
    SimulatedInterface sim;

    float temp1 = sim.readBoilerTemperature();

    // Update multiple times
    for (int i = 0; i < 50; i++)
    {
        sim.update();
    }

    float temp2 = sim.readBoilerTemperature();

    // Temperature should have changed (due to sine wave)
    ASSERT(temp1 != temp2);
}

TEST(dhw_active_when_below_setpoint)
{
    SimulatedInterface sim;

    // Set high DHW setpoint
    sim.writeDHWSetpoint(70.0f);
    sim.writeDHWEnabled(true);

    // DHW should be active if temperature is below setpoint
    float dhw_temp = sim.readDHWTemperature();
    bool dhw_active = sim.readDHWActive();

    if (dhw_temp < 70.0f)
    {
        ASSERT(dhw_active == true);
    }
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main()
{
    printf("\n");
    printf("==============================================\n");
    printf("  OpenTherm Simulator Tests\n");
    printf("==============================================\n\n");

    // Initial state tests
    printf("--- Initial State Tests ---\n");
    run_test_simulator_initial_state();

    // Temperature tests
    printf("\n--- Temperature Reading Tests ---\n");
    run_test_boiler_temperature_in_range();
    run_test_room_temperature_in_range();
    run_test_dhw_temperature_in_range();
    run_test_outside_temperature_in_range();
    run_test_return_water_cooler_than_boiler();

    // Pressure tests
    printf("\n--- Pressure Tests ---\n");
    run_test_ch_water_pressure_in_range();

    // Modulation tests
    printf("\n--- Modulation Tests ---\n");
    run_test_modulation_zero_when_ch_disabled();
    run_test_modulation_in_range();
    run_test_max_modulation_level();

    // Flame tests
    printf("\n--- Flame Status Tests ---\n");
    run_test_flame_off_when_ch_disabled();
    run_test_ch_active_requires_ch_enabled_and_flame();

    // Setpoint tests
    printf("\n--- Setpoint Tests ---\n");
    run_test_write_read_room_setpoint();
    run_test_write_read_dhw_setpoint();
    run_test_room_temperature_approaches_setpoint();

    // Control tests
    printf("\n--- Control Tests ---\n");
    run_test_enable_disable_ch();
    run_test_enable_disable_dhw();
    run_test_dhw_temperature_lower_when_disabled();

    // Statistics tests
    printf("\n--- Statistics Tests ---\n");
    run_test_burner_stats_non_negative();

    // Fault tests
    printf("\n--- Fault/Diagnostic Tests ---\n");
    run_test_no_faults_in_simulator();

    // Update tests
    printf("\n--- Simulation Update Tests ---\n");
    run_test_update_advances_simulation();
    run_test_dhw_active_when_below_setpoint();

    // Summary
    printf("\n==============================================\n");
    printf("  Test Results: %d/%d passed\n", tests_passed, tests_run);
    printf("==============================================\n\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
