/**
 * Unit tests for OpenTherm protocol functions
 *
 * These tests validate the core OpenTherm protocol encoding/decoding logic
 * without requiring hardware (no PIO/GPIO dependencies).
 */

#include "../src/opentherm_protocol.hpp"
#include <gtest/gtest.h>
#include <cmath>

using namespace OpenTherm::Protocol;

// ============================================================================
// Parity Tests
// ============================================================================

TEST(ParityTests, EvenFrame)
{
    // Frame with even number of 1s should have parity=0
    uint32_t frame = 0x80000000; // Just MSB set (parity bit itself)
    uint8_t parity = calculate_parity(frame & 0x7FFFFFFF);
    EXPECT_EQ(parity, 0);
}

TEST(ParityTests, OddFrame)
{
    // Frame with odd number of 1s should have parity=1
    uint32_t frame = 0x00000001; // Just LSB set
    uint8_t parity = calculate_parity(frame);
    EXPECT_EQ(parity, 1);
}

TEST(ParityTests, VerifyValid)
{
    // Build a valid frame and verify it
    uint32_t frame = build_read_request(OT_DATA_ID_STATUS);
    EXPECT_TRUE(verify_parity(frame));
}

TEST(ParityTests, VerifyInvalid)
{
    // Flip a bit to make parity invalid
    uint32_t frame = build_read_request(OT_DATA_ID_STATUS);
    frame ^= (1 << 10); // Flip a data bit
    EXPECT_FALSE(verify_parity(frame));
}

// ============================================================================
// Frame Packing/Unpacking Tests
// ============================================================================

TEST(FrameTests, PackUnpack)
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

    EXPECT_EQ(unpacked.msg_type, frame.msg_type);
    EXPECT_EQ(unpacked.data_id, frame.data_id);
    EXPECT_EQ(unpacked.data_value, frame.data_value);
}

TEST(FrameTests, SpareBitsZero)
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
    EXPECT_EQ(unpacked.spare, 0);
}

// ============================================================================
// Temperature Conversion Tests (f8.8 format)
// ============================================================================

TEST(F88ConversionTests, Zero)
{
    float temp = 0.0f;
    uint16_t encoded = f8_8_from_float(temp);
    float decoded = f8_8_to_float(encoded);
    EXPECT_NEAR(decoded, temp, 0.01f);
}

TEST(F88ConversionTests, Positive)
{
    float temp = 21.5f;
    uint16_t encoded = f8_8_from_float(temp);
    float decoded = f8_8_to_float(encoded);
    EXPECT_NEAR(decoded, temp, 0.01f);
}

TEST(F88ConversionTests, Negative)
{
    float temp = -5.25f;
    uint16_t encoded = f8_8_from_float(temp);
    float decoded = f8_8_to_float(encoded);
    EXPECT_NEAR(decoded, temp, 0.01f);
}

TEST(F88ConversionTests, Fractional)
{
    float temp = 65.75f;
    uint16_t encoded = f8_8_from_float(temp);
    float decoded = f8_8_to_float(encoded);
    EXPECT_NEAR(decoded, temp, 0.01f);
}

TEST(F88ConversionTests, Range)
{
    // Test various temperatures in normal operating range
    float temps[] = {-40.0f, -10.5f, 0.0f, 15.25f, 20.5f, 60.0f, 100.0f};
    for (float temp : temps)
    {
        uint16_t encoded = f8_8_from_float(temp);
        float decoded = f8_8_to_float(encoded);
        EXPECT_NEAR(decoded, temp, 0.01f);
    }
}

// ============================================================================
// Request Building Tests
// ============================================================================

TEST(RequestBuildingTests, ReadRequestStatus)
{
    uint32_t frame = build_read_request(OT_DATA_ID_STATUS);

    opentherm_frame_t unpacked;
    unpack_frame(frame, &unpacked);

    EXPECT_EQ(unpacked.msg_type, OT_MSGTYPE_READ_DATA);
    EXPECT_EQ(unpacked.data_id, OT_DATA_ID_STATUS);
    EXPECT_EQ(unpacked.data_value, 0);
    EXPECT_TRUE(verify_parity(frame));
}

TEST(RequestBuildingTests, WriteRequestSetpoint)
{
    float setpoint = 45.5f;
    uint32_t frame = write_control_setpoint(setpoint);

    opentherm_frame_t unpacked;
    unpack_frame(frame, &unpacked);

    EXPECT_EQ(unpacked.msg_type, OT_MSGTYPE_WRITE_DATA);
    EXPECT_EQ(unpacked.data_id, OT_DATA_ID_CONTROL_SETPOINT);

    float decoded = f8_8_to_float(unpacked.data_value);
    EXPECT_NEAR(decoded, setpoint, 0.01f);
    EXPECT_TRUE(verify_parity(frame));
}

// ============================================================================
// Status Encoding/Decoding Tests
// ============================================================================

TEST(StatusTests, DecodeAllFlags)
{
    // Master: CH+DHW enabled
    // Slave: CH mode + flame on
    uint16_t value = (0x03 << 8) | 0x0A;

    opentherm_status_t status;
    decode_status(value, &status);

    EXPECT_TRUE(status.ch_enable);
    EXPECT_TRUE(status.dhw_enable);
    EXPECT_FALSE(status.cooling_enable);
    EXPECT_TRUE(status.ch_mode);
    EXPECT_FALSE(status.dhw_mode);
    EXPECT_TRUE(status.flame);
}

TEST(StatusTests, EncodeDecodeRoundtrip)
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

    EXPECT_EQ(decoded.ch_enable, orig.ch_enable);
    EXPECT_EQ(decoded.dhw_enable, orig.dhw_enable);
    EXPECT_EQ(decoded.otc_active, orig.otc_active);
    EXPECT_EQ(decoded.ch_mode, orig.ch_mode);
    EXPECT_EQ(decoded.flame, orig.flame);
}

// ============================================================================
// Data Extraction Tests
// ============================================================================

TEST(DataExtractionTests, GetU16FromFrame)
{
    uint16_t expected = 0xABCD;
    uint32_t frame = build_write_request(OT_DATA_ID_MAX_CH_SETPOINT, expected);
    uint16_t actual = get_u16(frame);
    EXPECT_EQ(actual, expected);
}

TEST(DataExtractionTests, GetF88FromFrame)
{
    float temp = 55.5f;
    uint32_t frame = write_control_setpoint(temp);
    float decoded = get_f8_8(frame);
    EXPECT_NEAR(decoded, temp, 0.01f);
}

TEST(DataExtractionTests, GetU8U8FromFrame)
{
    uint8_t hb = 0x12;
    uint8_t lb = 0x34;
    uint16_t value = encode_u8_u8(hb, lb);
    uint32_t frame = build_write_request(OT_DATA_ID_DATE, value);

    uint8_t decoded_hb, decoded_lb;
    get_u8_u8(frame, &decoded_hb, &decoded_lb);

    EXPECT_EQ(decoded_hb, hb);
    EXPECT_EQ(decoded_lb, lb);
}

// ============================================================================
// Configuration Encoding/Decoding Tests
// ============================================================================

TEST(ConfigTests, MasterConfigEncodeDecode)
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

    EXPECT_EQ(decoded.dhw_present, config.dhw_present);
    EXPECT_EQ(decoded.control_type, config.control_type);
    EXPECT_EQ(decoded.dhw_config, config.dhw_config);
    EXPECT_EQ(decoded.master_pump_control, config.master_pump_control);
}

// ============================================================================
// Fault Flags Tests
// ============================================================================

TEST(FaultTests, DecodeAllFlags)
{
    // OEM code=5, service request + air pressure fault
    uint16_t value = (5 << 8) | 0x11;

    opentherm_fault_t fault;
    OpenTherm::Protocol::decode_fault(value, &fault);

    EXPECT_EQ(fault.code, 5);
    EXPECT_TRUE(fault.service_request);
    EXPECT_FALSE(fault.lockout_reset);
    EXPECT_FALSE(fault.low_water_pressure);
    EXPECT_FALSE(fault.gas_flame_fault);
    EXPECT_TRUE(fault.air_pressure_fault);
    EXPECT_FALSE(fault.water_overtemp);
}

// ============================================================================
// Time/Date Encoding Tests
// ============================================================================

TEST(TimeTests, EncodeDecodeTime)
{
    opentherm_time_t time = {
        .day_of_week = 3, // Wednesday
        .hours = 14,
        .minutes = 30};

    uint16_t encoded = encode_time(&time);

    opentherm_time_t decoded;
    decode_time(encoded, &decoded);

    EXPECT_EQ(decoded.day_of_week, time.day_of_week);
    EXPECT_EQ(decoded.hours, time.hours);
    EXPECT_EQ(decoded.minutes, time.minutes);
}

TEST(TimeTests, EncodeDecodeDate)
{
    opentherm_date_t date = {
        .month = 11, // November
        .day = 7};

    uint16_t encoded = encode_date(&date);

    opentherm_date_t decoded;
    decode_date(encoded, &decoded);

    EXPECT_EQ(decoded.month, date.month);
    EXPECT_EQ(decoded.day, date.day);
}

// ============================================================================
// Signed Integer Tests
// ============================================================================

TEST(SignedIntTests, PositiveValue)
{
    int16_t value = 1234;
    uint16_t encoded = encode_s16(value);
    int16_t decoded = decode_s16(encoded);
    EXPECT_EQ(decoded, value);
}

TEST(SignedIntTests, NegativeValue)
{
    int16_t value = -567;
    uint16_t encoded = encode_s16(value);
    int16_t decoded = decode_s16(encoded);
    EXPECT_EQ(decoded, value);
}

TEST(SignedIntTests, ZeroValue)
{
    int16_t value = 0;
    uint16_t encoded = encode_s16(value);
    int16_t decoded = decode_s16(encoded);
    EXPECT_EQ(decoded, value);
}
