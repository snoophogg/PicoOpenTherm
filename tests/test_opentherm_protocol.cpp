/**
 * Unit tests for OpenTherm protocol functions
 *
 * These tests validate the core OpenTherm protocol encoding/decoding logic
 * without requiring hardware (no PIO/GPIO dependencies).
 */

#include "../src/opentherm_protocol.hpp"
#include <cstdio>
#include <cassert>
#include <cmath>

using namespace OpenTherm::Protocol;

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

#define ASSERT_EQ(actual, expected)                                                                                           \
    do                                                                                                                        \
    {                                                                                                                         \
        if ((actual) != (expected))                                                                                           \
        {                                                                                                                     \
            printf("FAILED\n  Expected: %d\n  Actual: %d\n  at %s:%d\n", (int)(expected), (int)(actual), __FILE__, __LINE__); \
            return;                                                                                                           \
        }                                                                                                                     \
    } while (0)

#define ASSERT_FLOAT_EQ(actual, expected, tolerance)                                       \
    do                                                                                     \
    {                                                                                      \
        float diff = std::abs((float)(actual) - (float)(expected));                        \
        if (diff > (tolerance))                                                            \
        {                                                                                  \
            printf("FAILED\n  Expected: %.6f\n  Actual: %.6f\n  Diff: %.6f\n  at %s:%d\n", \
                   (float)(expected), (float)(actual), diff, __FILE__, __LINE__);          \
            return;                                                                        \
        }                                                                                  \
    } while (0)

// ============================================================================
// Parity Tests
// ============================================================================

TEST(parity_even_frame)
{
    // Frame with even number of 1s should have parity=0
    uint32_t frame = 0x80000000; // Just MSB set (parity bit itself)
    uint8_t parity = calculate_parity(frame & 0x7FFFFFFF);
    ASSERT_EQ(parity, 0);
}

TEST(parity_odd_frame)
{
    // Frame with odd number of 1s should have parity=1
    uint32_t frame = 0x00000001; // Just LSB set
    uint8_t parity = calculate_parity(frame);
    ASSERT_EQ(parity, 1);
}

TEST(parity_verify_valid)
{
    // Build a valid frame and verify it
    uint32_t frame = build_read_request(OT_DATA_ID_STATUS);
    ASSERT(verify_parity(frame));
}

TEST(parity_verify_invalid)
{
    // Flip a bit to make parity invalid
    uint32_t frame = build_read_request(OT_DATA_ID_STATUS);
    frame ^= (1 << 10); // Flip a data bit
    ASSERT(!verify_parity(frame));
}

// ============================================================================
// Frame Packing/Unpacking Tests
// ============================================================================

TEST(frame_pack_unpack)
{
    opentherm_frame_t frame = {
        .parity = 1,
        .msg_type = OT_MSGTYPE_READ_DATA,
        .spare = 0,
        .data_id = OT_DATA_ID_STATUS,
        .data_value = 0x1234};

    uint32_t packed = pack_frame(&frame);

    opentherm_frame_t unpacked;
    unpack_frame(packed, &unpacked);

    ASSERT_EQ(unpacked.msg_type, frame.msg_type);
    ASSERT_EQ(unpacked.data_id, frame.data_id);
    ASSERT_EQ(unpacked.data_value, frame.data_value);
}

TEST(frame_spare_bits_zero)
{
    opentherm_frame_t frame = {
        .parity = 0,
        .msg_type = OT_MSGTYPE_READ_DATA,
        .spare = 0xF, // Try to set all spare bits
        .data_id = 0,
        .data_value = 0};

    uint32_t packed = pack_frame(&frame);
    opentherm_frame_t unpacked;
    unpack_frame(packed, &unpacked);

    // Spare bits should be masked to 0
    ASSERT_EQ(unpacked.spare, 0);
}

// ============================================================================
// Temperature Conversion Tests (f8.8 format)
// ============================================================================

TEST(f8_8_conversion_zero)
{
    float temp = 0.0f;
    uint16_t encoded = f8_8_from_float(temp);
    float decoded = f8_8_to_float(encoded);
    ASSERT_FLOAT_EQ(decoded, temp, 0.01f);
}

TEST(f8_8_conversion_positive)
{
    float temp = 21.5f;
    uint16_t encoded = f8_8_from_float(temp);
    float decoded = f8_8_to_float(encoded);
    ASSERT_FLOAT_EQ(decoded, temp, 0.01f);
}

TEST(f8_8_conversion_negative)
{
    float temp = -5.25f;
    uint16_t encoded = f8_8_from_float(temp);
    float decoded = f8_8_to_float(encoded);
    ASSERT_FLOAT_EQ(decoded, temp, 0.01f);
}

TEST(f8_8_conversion_fractional)
{
    float temp = 65.75f;
    uint16_t encoded = f8_8_from_float(temp);
    float decoded = f8_8_to_float(encoded);
    ASSERT_FLOAT_EQ(decoded, temp, 0.01f);
}

TEST(f8_8_conversion_range)
{
    // Test various temperatures in normal operating range
    float temps[] = {-40.0f, -10.5f, 0.0f, 15.25f, 20.5f, 60.0f, 100.0f};
    for (float temp : temps)
    {
        uint16_t encoded = f8_8_from_float(temp);
        float decoded = f8_8_to_float(encoded);
        ASSERT_FLOAT_EQ(decoded, temp, 0.01f);
    }
}

// ============================================================================
// Request Building Tests
// ============================================================================

TEST(build_read_request_status)
{
    uint32_t frame = build_read_request(OT_DATA_ID_STATUS);

    opentherm_frame_t unpacked;
    unpack_frame(frame, &unpacked);

    ASSERT_EQ(unpacked.msg_type, OT_MSGTYPE_READ_DATA);
    ASSERT_EQ(unpacked.data_id, OT_DATA_ID_STATUS);
    ASSERT_EQ(unpacked.data_value, 0);
    ASSERT(verify_parity(frame));
}

TEST(build_write_request_setpoint)
{
    float setpoint = 45.5f;
    uint32_t frame = write_control_setpoint(setpoint);

    opentherm_frame_t unpacked;
    unpack_frame(frame, &unpacked);

    ASSERT_EQ(unpacked.msg_type, OT_MSGTYPE_WRITE_DATA);
    ASSERT_EQ(unpacked.data_id, OT_DATA_ID_CONTROL_SETPOINT);

    float decoded = f8_8_to_float(unpacked.data_value);
    ASSERT_FLOAT_EQ(decoded, setpoint, 0.01f);
    ASSERT(verify_parity(frame));
}

// ============================================================================
// Status Encoding/Decoding Tests
// ============================================================================

TEST(status_decode_all_flags)
{
    // Master: CH+DHW enabled
    // Slave: CH mode + flame on
    uint16_t value = (0x03 << 8) | 0x0A;

    opentherm_status_t status;
    decode_status(value, &status);

    ASSERT(status.ch_enable);
    ASSERT(status.dhw_enable);
    ASSERT(!status.cooling_enable);
    ASSERT(status.ch_mode);
    ASSERT(!status.dhw_mode);
    ASSERT(status.flame);
}

TEST(status_encode_decode_roundtrip)
{
    opentherm_status_t orig = {
        .ch_enable = true,
        .dhw_enable = false,
        .cooling_enable = false,
        .otc_active = true,
        .ch2_enable = false,
        .fault = false,
        .ch_mode = true,
        .dhw_mode = false,
        .flame = true,
        .cooling = false,
        .ch2_mode = false,
        .diagnostic = false};

    uint16_t encoded = encode_status(&orig);

    opentherm_status_t decoded;
    decode_status(encoded, &decoded);

    ASSERT_EQ(decoded.ch_enable, orig.ch_enable);
    ASSERT_EQ(decoded.dhw_enable, orig.dhw_enable);
    ASSERT_EQ(decoded.otc_active, orig.otc_active);
    ASSERT_EQ(decoded.ch_mode, orig.ch_mode);
    ASSERT_EQ(decoded.flame, orig.flame);
}

// ============================================================================
// Data Extraction Tests
// ============================================================================

TEST(get_u16_from_frame)
{
    uint16_t expected = 0xABCD;
    uint32_t frame = build_write_request(OT_DATA_ID_MAX_CH_SETPOINT, expected);
    uint16_t actual = get_u16(frame);
    ASSERT_EQ(actual, expected);
}

TEST(get_f8_8_from_frame)
{
    float temp = 55.5f;
    uint32_t frame = write_control_setpoint(temp);
    float decoded = get_f8_8(frame);
    ASSERT_FLOAT_EQ(decoded, temp, 0.01f);
}

TEST(get_u8_u8_from_frame)
{
    uint8_t hb = 0x12;
    uint8_t lb = 0x34;
    uint16_t value = encode_u8_u8(hb, lb);
    uint32_t frame = build_write_request(OT_DATA_ID_DATE, value);

    uint8_t decoded_hb, decoded_lb;
    get_u8_u8(frame, &decoded_hb, &decoded_lb);

    ASSERT_EQ(decoded_hb, hb);
    ASSERT_EQ(decoded_lb, lb);
}

// ============================================================================
// Configuration Encoding/Decoding Tests
// ============================================================================

TEST(master_config_encode_decode)
{
    opentherm_config_t config = {
        .dhw_present = true,
        .control_type = false, // modulating
        .cooling_config = false,
        .dhw_config = true, // storage tank
        .master_pump_control = true,
        .ch2_present = false};

    uint16_t encoded = encode_master_config(&config);

    opentherm_config_t decoded;
    decode_master_config(encoded, &decoded);

    ASSERT_EQ(decoded.dhw_present, config.dhw_present);
    ASSERT_EQ(decoded.control_type, config.control_type);
    ASSERT_EQ(decoded.dhw_config, config.dhw_config);
    ASSERT_EQ(decoded.master_pump_control, config.master_pump_control);
}

// ============================================================================
// Fault Flags Tests
// ============================================================================

TEST(fault_decode_all_flags)
{
    // OEM code=5, service request + low water pressure
    uint16_t value = (5 << 8) | 0x11;

    opentherm_fault_t fault;
    decode_fault(value, &fault);

    ASSERT_EQ(fault.code, 5);
    ASSERT(fault.service_request);
    ASSERT(!fault.lockout_reset);
    ASSERT(!fault.low_water_pressure);
    ASSERT(!fault.gas_flame_fault);
    ASSERT(fault.water_overtemp);
}

// ============================================================================
// Time/Date Encoding Tests
// ============================================================================

TEST(time_encode_decode)
{
    opentherm_time_t time = {
        .day_of_week = 3, // Wednesday
        .hours = 14,
        .minutes = 30};

    uint16_t encoded = encode_time(&time);

    opentherm_time_t decoded;
    decode_time(encoded, &decoded);

    ASSERT_EQ(decoded.day_of_week, time.day_of_week);
    ASSERT_EQ(decoded.hours, time.hours);
    ASSERT_EQ(decoded.minutes, time.minutes);
}

TEST(date_encode_decode)
{
    opentherm_date_t date = {
        .month = 11, // November
        .day = 7};

    uint16_t encoded = encode_date(&date);

    opentherm_date_t decoded;
    decode_date(encoded, &decoded);

    ASSERT_EQ(decoded.month, date.month);
    ASSERT_EQ(decoded.day, date.day);
}

// ============================================================================
// Signed Integer Tests
// ============================================================================

TEST(s16_encode_decode_positive)
{
    int16_t value = 1234;
    uint16_t encoded = encode_s16(value);
    int16_t decoded = decode_s16(encoded);
    ASSERT_EQ(decoded, value);
}

TEST(s16_encode_decode_negative)
{
    int16_t value = -567;
    uint16_t encoded = encode_s16(value);
    int16_t decoded = decode_s16(encoded);
    ASSERT_EQ(decoded, value);
}

TEST(s16_encode_decode_zero)
{
    int16_t value = 0;
    uint16_t encoded = encode_s16(value);
    int16_t decoded = decode_s16(encoded);
    ASSERT_EQ(decoded, value);
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main()
{
    printf("\n");
    printf("==============================================\n");
    printf("  OpenTherm Protocol Unit Tests\n");
    printf("==============================================\n\n");

    // Parity tests
    printf("--- Parity Tests ---\n");
    run_test_parity_even_frame();
    run_test_parity_odd_frame();
    run_test_parity_verify_valid();
    run_test_parity_verify_invalid();

    // Frame tests
    printf("\n--- Frame Packing/Unpacking Tests ---\n");
    run_test_frame_pack_unpack();
    run_test_frame_spare_bits_zero();

    // Temperature tests
    printf("\n--- Temperature Conversion Tests (f8.8) ---\n");
    run_test_f8_8_conversion_zero();
    run_test_f8_8_conversion_positive();
    run_test_f8_8_conversion_negative();
    run_test_f8_8_conversion_fractional();
    run_test_f8_8_conversion_range();

    // Request building tests
    printf("\n--- Request Building Tests ---\n");
    run_test_build_read_request_status();
    run_test_build_write_request_setpoint();

    // Status tests
    printf("\n--- Status Encoding/Decoding Tests ---\n");
    run_test_status_decode_all_flags();
    run_test_status_encode_decode_roundtrip();

    // Data extraction tests
    printf("\n--- Data Extraction Tests ---\n");
    run_test_get_u16_from_frame();
    run_test_get_f8_8_from_frame();
    run_test_get_u8_u8_from_frame();

    // Configuration tests
    printf("\n--- Configuration Encoding/Decoding Tests ---\n");
    run_test_master_config_encode_decode();

    // Fault tests
    printf("\n--- Fault Flags Tests ---\n");
    run_test_fault_decode_all_flags();

    // Time/Date tests
    printf("\n--- Time/Date Encoding Tests ---\n");
    run_test_time_encode_decode();
    run_test_date_encode_decode();

    // Signed integer tests
    printf("\n--- Signed Integer Tests ---\n");
    run_test_s16_encode_decode_positive();
    run_test_s16_encode_decode_negative();
    run_test_s16_encode_decode_zero();

    // Summary
    printf("\n==============================================\n");
    printf("  Test Results: %d/%d passed\n", tests_passed, tests_run);
    printf("==============================================\n\n");

    return (tests_passed == tests_run) ? 0 : 1;
}
