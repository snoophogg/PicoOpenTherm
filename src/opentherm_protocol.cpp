/**
 * OpenTherm Protocol Functions - Implementation
 *
 * This file contains all the protocol encoding/decoding functions
 * without hardware dependencies (PIO), making it suitable for host-based
 * unit testing.
 *
 * All protocol-level functions are implemented here in the OpenTherm::Protocol namespace.
 */

#include "opentherm_protocol.hpp"

namespace OpenTherm
{
    namespace Protocol
    {

        // Calculate even parity for 32-bit frame
        uint8_t calculate_parity(uint32_t frame)
        {
            uint8_t parity = 0;
            for (int i = 0; i < 32; i++)
            {
                if (frame & (1UL << i))
                {
                    parity ^= 1;
                }
            }
            return parity;
        }

        // Pack frame structure into 32-bit word
        uint32_t pack_frame(const opentherm_frame_t *frame)
        {
            uint32_t packed = 0;

            packed |= (static_cast<uint32_t>(frame->msg_type) & 0x07) << 28;
            // Spare bits must always be 0 per OpenTherm spec
            packed |= (0) << 24;
            packed |= (static_cast<uint32_t>(frame->data_id) & 0xFF) << 16;
            packed |= (static_cast<uint32_t>(frame->data_value) & 0xFFFF);

            // Calculate and set parity bit
            uint8_t parity = opentherm_calculate_parity(packed);
            packed |= (static_cast<uint32_t>(parity) << 31);

            return packed;
        }

        // Unpack 32-bit word into frame structure
        void unpack_frame(uint32_t packed, opentherm_frame_t *frame)
        {
            frame->parity = (packed >> 31) & 0x01;
            frame->msg_type = (packed >> 28) & 0x07;
            frame->spare = (packed >> 24) & 0x0F;
            frame->data_id = (packed >> 16) & 0xFF;
            frame->data_value = packed & 0xFFFF;
        }

        // Verify frame parity
        bool verify_parity(uint32_t frame)
        {
            uint8_t calculated_parity = opentherm_calculate_parity(frame & 0x7FFFFFFF);
            uint8_t frame_parity = (frame >> 31) & 0x01;
            return calculated_parity == frame_parity;
        }

        // Create a READ-DATA request
        uint32_t build_read_request(uint8_t data_id)
        {
            opentherm_frame_t frame = {
                .parity = 0, // Will be calculated
                .msg_type = OT_MSGTYPE_READ_DATA,
                .spare = 0,
                .data_id = data_id,
                .data_value = 0x0000};
            return opentherm_pack_frame(&frame);
        }

        // Create a WRITE-DATA request
        uint32_t build_write_request(uint8_t data_id, uint16_t data_value)
        {
            opentherm_frame_t frame = {
                .parity = 0, // Will be calculated
                .msg_type = OT_MSGTYPE_WRITE_DATA,
                .spare = 0,
                .data_id = data_id,
                .data_value = data_value};
            return opentherm_pack_frame(&frame);
        }

        // Convert float temperature to f8.8 format
        uint16_t f8_8_from_float(float temp)
        {
            int16_t value = static_cast<int16_t>(temp * 256.0f);
            return static_cast<uint16_t>(value);
        }

        // Convert f8.8 format to float temperature
        float f8_8_to_float(uint16_t value)
        {
            int16_t signed_value = static_cast<int16_t>(value);
            return static_cast<float>(signed_value) / 256.0f;
        }

        // Helper functions to extract and convert data values from frames

        // Extract data value from a frame
        uint16_t get_u16(uint32_t frame)
        {
            return static_cast<uint16_t>(frame & 0xFFFF);
        }

        // Extract and convert f8.8 temperature value from a frame
        float get_f8_8(uint32_t frame)
        {
            uint16_t value = opentherm_get_u16(frame);
            return opentherm_f8_8_to_float(value);
        }

        // Extract and convert s16 signed integer from a frame
        int16_t get_s16(uint32_t frame)
        {
            uint16_t value = opentherm_get_u16(frame);
            return opentherm_decode_s16(value);
        }

        // Extract two u8 bytes from a frame
        void get_u8_u8(uint32_t frame, uint8_t *hb, uint8_t *lb)
        {
            uint16_t value = opentherm_get_u16(frame);
            opentherm_decode_u8_u8(value, hb, lb);
        }

        // Encode two bytes into a 16-bit data value (HB:LB)
        uint16_t encode_u8_u8(uint8_t hb, uint8_t lb)
        {
            return (static_cast<uint16_t>(hb) << 8) | lb;
        }

        // Decode 16-bit data value into two bytes (HB:LB)
        void decode_u8_u8(uint16_t value, uint8_t *hb, uint8_t *lb)
        {
            *hb = (value >> 8) & 0xFF;
            *lb = value & 0xFF;
        }

        // Encode signed 16-bit integer
        uint16_t encode_s16(int16_t value)
        {
            return static_cast<uint16_t>(value);
        }

        // Decode signed 16-bit integer
        int16_t decode_s16(uint16_t value)
        {
            return static_cast<int16_t>(value);
        }

        // Decode status flags (Data ID 0)
        void decode_status(uint16_t value, opentherm_status_t *status)
        {
            uint8_t master_flags = (value >> 8) & 0xFF;
            uint8_t slave_flags = value & 0xFF;

            // Master status flags (HB)
            status->ch_enable = (master_flags & 0x01) != 0;
            status->dhw_enable = (master_flags & 0x02) != 0;
            status->cooling_enable = (master_flags & 0x04) != 0;
            status->otc_active = (master_flags & 0x08) != 0;
            status->ch2_enable = (master_flags & 0x10) != 0;

            // Slave status flags (LB)
            status->fault = (slave_flags & 0x01) != 0;
            status->ch_mode = (slave_flags & 0x02) != 0;
            status->dhw_mode = (slave_flags & 0x04) != 0;
            status->flame = (slave_flags & 0x08) != 0;
            status->cooling = (slave_flags & 0x10) != 0;
            status->ch2_mode = (slave_flags & 0x20) != 0;
            status->diagnostic = (slave_flags & 0x40) != 0;
        }

        uint16_t encode_status(const opentherm_status_t *status)
        {
            uint8_t master_flags = 0;
            uint8_t slave_flags = 0;

            // Master status flags (HB)
            if (status->ch_enable)
                master_flags |= 0x01;
            if (status->dhw_enable)
                master_flags |= 0x02;
            if (status->cooling_enable)
                master_flags |= 0x04;
            if (status->otc_active)
                master_flags |= 0x08;
            if (status->ch2_enable)
                master_flags |= 0x10;

            // Slave status flags (LB)
            if (status->fault)
                slave_flags |= 0x01;
            if (status->ch_mode)
                slave_flags |= 0x02;
            if (status->dhw_mode)
                slave_flags |= 0x04;
            if (status->flame)
                slave_flags |= 0x08;
            if (status->cooling)
                slave_flags |= 0x10;
            if (status->ch2_mode)
                slave_flags |= 0x20;
            if (status->diagnostic)
                slave_flags |= 0x40;

            return opentherm_encode_u8_u8(master_flags, slave_flags);
        }

        // Decode configuration flags
        void decode_master_config(uint16_t value, opentherm_config_t *config)
        {
            uint8_t flags = value & 0xFF;

            config->dhw_present = (flags & 0x01) != 0;
            config->control_type = (flags & 0x02) != 0;
            config->cooling_config = (flags & 0x04) != 0;
            config->dhw_config = (flags & 0x08) != 0;
            config->master_pump_control = (flags & 0x10) != 0;
            config->ch2_present = (flags & 0x20) != 0;
        }

        uint16_t encode_master_config(const opentherm_config_t *config)
        {
            uint8_t flags = 0;

            if (config->dhw_present)
                flags |= 0x01;
            if (config->control_type)
                flags |= 0x02;
            if (config->cooling_config)
                flags |= 0x04;
            if (config->dhw_config)
                flags |= 0x08;
            if (config->master_pump_control)
                flags |= 0x10;
            if (config->ch2_present)
                flags |= 0x20;

            return flags;
        }

        void decode_slave_config(uint16_t value, opentherm_config_t *config)
        {
            uint8_t flags = value & 0xFF;

            config->dhw_present = (flags & 0x01) != 0;
            config->control_type = (flags & 0x02) != 0;
            config->cooling_config = (flags & 0x04) != 0;
            config->dhw_config = (flags & 0x08) != 0;
            config->master_pump_control = (flags & 0x10) != 0;
            config->ch2_present = (flags & 0x20) != 0;
        }

        uint16_t encode_slave_config(const opentherm_config_t *config)
        {
            uint8_t flags = 0;

            if (config->dhw_present)
                flags |= 0x01;
            if (config->control_type)
                flags |= 0x02;
            if (config->cooling_config)
                flags |= 0x04;
            if (config->dhw_config)
                flags |= 0x08;
            if (config->master_pump_control)
                flags |= 0x10;
            if (config->ch2_present)
                flags |= 0x20;

            return flags;
        }

        // Decode fault flags (Data ID 5)
        void decode_fault(uint16_t value, opentherm_fault_t *fault)
        {
            fault->code = (value >> 8) & 0xFF;
            uint8_t flags = value & 0xFF;

            fault->service_request = (flags & 0x01) != 0;
            fault->lockout_reset = (flags & 0x02) != 0;
            fault->low_water_pressure = (flags & 0x04) != 0;
            fault->gas_flame_fault = (flags & 0x08) != 0;
            fault->air_pressure_fault = (flags & 0x10) != 0;
            fault->water_overtemp = (flags & 0x20) != 0;
        }

        // Decode remote parameter flags (Data ID 6)
        void decode_remote_params(uint16_t value, opentherm_remote_params_t *params)
        {
            uint8_t transfer_enable = (value >> 8) & 0xFF;
            uint8_t rw_flags = value & 0xFF;

            params->dhw_setpoint_enable = (transfer_enable & 0x01) != 0;
            params->max_ch_setpoint_enable = (transfer_enable & 0x02) != 0;
            params->dhw_setpoint_rw = (rw_flags & 0x01) != 0;
            params->max_ch_setpoint_rw = (rw_flags & 0x02) != 0;
        }

        // Encode/decode day of week and time (Data ID 20)
        uint16_t encode_time(const opentherm_time_t *time)
        {
            uint8_t hb = ((time->day_of_week & 0x07) << 5) | (time->hours & 0x1F);
            uint8_t lb = time->minutes & 0x3F;
            return opentherm_encode_u8_u8(hb, lb);
        }

        void decode_time(uint16_t value, opentherm_time_t *time)
        {
            uint8_t hb, lb;
            opentherm_decode_u8_u8(value, &hb, &lb);

            time->day_of_week = (hb >> 5) & 0x07;
            time->hours = hb & 0x1F;
            time->minutes = lb & 0x3F;
        }

        // Encode/decode date (Data ID 21)
        uint16_t encode_date(const opentherm_date_t *date)
        {
            return opentherm_encode_u8_u8(date->month, date->day);
        }

        void decode_date(uint16_t value, opentherm_date_t *date)
        {
            opentherm_decode_u8_u8(value, &date->month, &date->day);
        }

        // Helper functions for reading and writing with proper type conversions
        // Build write requests with proper encoding
        uint32_t write_control_setpoint(float temperature)
        {
            uint16_t value = f8_8_from_float(temperature);
            return build_write_request(OT_DATA_ID_CONTROL_SETPOINT, value);
        }

        uint32_t write_room_setpoint(float temperature)
        {
            uint16_t value = f8_8_from_float(temperature);
            return build_write_request(OT_DATA_ID_ROOM_SETPOINT, value);
        }

        uint32_t write_room_setpoint_ch2(float temperature)
        {
            uint16_t value = f8_8_from_float(temperature);
            return build_write_request(OT_DATA_ID_ROOM_SETPOINT_CH2, value);
        }

        uint32_t write_dhw_setpoint(float temperature)
        {
            uint16_t value = f8_8_from_float(temperature);
            return build_write_request(OT_DATA_ID_DHW_SETPOINT, value);
        }

        uint32_t write_max_ch_setpoint(float temperature)
        {
            uint16_t value = f8_8_from_float(temperature);
            return build_write_request(OT_DATA_ID_MAX_CH_SETPOINT, value);
        }

        uint32_t write_day_time(uint8_t day_of_week, uint8_t hours, uint8_t minutes)
        {
            opentherm_time_t time = {day_of_week, hours, minutes};
            uint16_t value = encode_time(&time);
            return build_write_request(OT_DATA_ID_DAY_TIME, value);
        }

        uint32_t write_date(uint8_t month, uint8_t day)
        {
            opentherm_date_t date = {month, day};
            uint16_t value = encode_date(&date);
            return build_write_request(OT_DATA_ID_DATE, value);
        }

        uint32_t write_year(uint16_t year)
        {
            return build_write_request(OT_DATA_ID_YEAR, year);
        }

        // Helper functions for reading sensor data with proper type conversions

        // Build read requests for specific data types
        uint32_t read_status()
        {
            return build_read_request(OT_DATA_ID_STATUS);
        }

        uint32_t read_control_setpoint()
        {
            return build_read_request(OT_DATA_ID_CONTROL_SETPOINT);
        }

        uint32_t read_master_config()
        {
            return build_read_request(OT_DATA_ID_MASTER_CONFIG);
        }

        uint32_t read_slave_config()
        {
            return build_read_request(OT_DATA_ID_SLAVE_CONFIG);
        }

        uint32_t read_fault_flags()
        {
            return build_read_request(OT_DATA_ID_FAULT_FLAGS);
        }

        uint32_t read_remote_params()
        {
            return build_read_request(OT_DATA_ID_REMOTE_PARAMS);
        }

        uint32_t read_max_rel_mod()
        {
            return build_read_request(OT_DATA_ID_MAX_REL_MOD);
        }

        uint32_t read_max_capacity()
        {
            return build_read_request(OT_DATA_ID_MAX_CAPACITY);
        }

        // Sensor data reads
        uint32_t read_rel_mod_level()
        {
            return build_read_request(OT_DATA_ID_REL_MOD_LEVEL);
        }

        uint32_t read_ch_water_pressure()
        {
            return build_read_request(OT_DATA_ID_CH_WATER_PRESS);
        }

        uint32_t read_dhw_flow_rate()
        {
            return build_read_request(OT_DATA_ID_DHW_FLOW_RATE);
        }

        uint32_t read_day_time()
        {
            return build_read_request(OT_DATA_ID_DAY_TIME);
        }

        uint32_t read_date()
        {
            return build_read_request(OT_DATA_ID_DATE);
        }

        uint32_t read_year()
        {
            return build_read_request(OT_DATA_ID_YEAR);
        }

        uint32_t read_room_temp()
        {
            return build_read_request(OT_DATA_ID_ROOM_TEMP);
        }

        uint32_t read_boiler_water_temp()
        {
            return build_read_request(OT_DATA_ID_BOILER_WATER_TEMP);
        }

        uint32_t read_dhw_temp()
        {
            return build_read_request(OT_DATA_ID_DHW_TEMP);
        }

        uint32_t read_outside_temp()
        {
            return build_read_request(OT_DATA_ID_OUTSIDE_TEMP);
        }

        uint32_t read_return_water_temp()
        {
            return build_read_request(OT_DATA_ID_RETURN_WATER_TEMP);
        }

        uint32_t read_solar_storage_temp()
        {
            return build_read_request(OT_DATA_ID_SOLAR_STORAGE_TEMP);
        }

        uint32_t read_solar_collector_temp()
        {
            return build_read_request(OT_DATA_ID_SOLAR_COLL_TEMP);
        }

        uint32_t read_flow_temp_ch2()
        {
            return build_read_request(OT_DATA_ID_FLOW_TEMP_CH2);
        }

        uint32_t read_dhw2_temp()
        {
            return build_read_request(OT_DATA_ID_DHW2_TEMP);
        }

        uint32_t read_exhaust_temp()
        {
            return build_read_request(OT_DATA_ID_EXHAUST_TEMP);
        }

        // Remote boiler parameter reads
        uint32_t read_dhw_bounds()
        {
            return build_read_request(OT_DATA_ID_DHW_BOUNDS);
        }

        uint32_t read_ch_bounds()
        {
            return build_read_request(OT_DATA_ID_CH_BOUNDS);
        }

        uint32_t read_dhw_setpoint()
        {
            return build_read_request(OT_DATA_ID_DHW_SETPOINT);
        }

        uint32_t read_max_ch_setpoint()
        {
            return build_read_request(OT_DATA_ID_MAX_CH_SETPOINT);
        }

        // Counter/statistics reads
        uint32_t read_burner_starts()
        {
            return build_read_request(OT_DATA_ID_BURNER_STARTS);
        }

        uint32_t read_ch_pump_starts()
        {
            return build_read_request(OT_DATA_ID_CH_PUMP_STARTS);
        }

        uint32_t read_dhw_pump_starts()
        {
            return build_read_request(OT_DATA_ID_DHW_PUMP_STARTS);
        }

        uint32_t read_dhw_burner_starts()
        {
            return build_read_request(OT_DATA_ID_DHW_BURNER_STARTS);
        }

        uint32_t read_burner_hours()
        {
            return build_read_request(OT_DATA_ID_BURNER_HOURS);
        }

        uint32_t read_ch_pump_hours()
        {
            return build_read_request(OT_DATA_ID_CH_PUMP_HOURS);
        }

        uint32_t read_dhw_pump_hours()
        {
            return build_read_request(OT_DATA_ID_DHW_PUMP_HOURS);
        }

        uint32_t read_dhw_burner_hours()
        {
            return build_read_request(OT_DATA_ID_DHW_BURNER_HOURS);
        }

        // Version information reads
        uint32_t read_opentherm_version()
        {
            return build_read_request(OT_DATA_ID_OPENTHERM_VERSION);
        }

        uint32_t read_slave_version()
        {
            return build_read_request(OT_DATA_ID_SLAVE_VERSION);
        }

        uint32_t read_master_version()
        {
            return build_read_request(OT_DATA_ID_MASTER_VERSION);
        }

        uint32_t read_slave_product()
        {
            return build_read_request(OT_DATA_ID_SLAVE_PRODUCT);
        }

        // Manchester encoding/decoding
        // Decode Manchester encoding: each bit is represented by 2 samples
        // '1' = 1,0 (active-to-idle)
        // '0' = 0,1 (idle-to-active)
        bool manchester_decode(uint64_t raw_data, uint32_t *decoded_frame)
        {
            *decoded_frame = 0;

            // Process 34 bit pairs (start + 32 data + stop)
            for (int i = 0; i < 34; i++)
            {
                int bit_pos = (33 - i) * 2; // Start from MSB pairs
                uint8_t first_half = (raw_data >> (bit_pos + 1)) & 1;
                uint8_t second_half = (raw_data >> bit_pos) & 1;

                if (i == 0 || i == 33)
                {
                    // Start and stop bits should be '1' (1,0 pattern)
                    if (first_half != 1 || second_half != 0)
                    {
                        return false; // Invalid frame
                    }
                }
                else
                {
                    // Data bits
                    if (first_half == 1 && second_half == 0)
                    {
                        // '1' bit
                        *decoded_frame |= (1u << (32 - i));
                    }
                    else if (first_half == 0 && second_half == 1)
                    {
                        // '0' bit - already 0
                    }
                    else
                    {
                        return false; // Invalid Manchester encoding
                    }
                }
            }

            return true;
        }

    } // namespace Protocol
} // namespace OpenTherm
