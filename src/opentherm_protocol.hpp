/**
 * OpenTherm Protocol Functions - Test-friendly header
 *
 * This header includes only the protocol encoding/decoding functions
 * without hardware dependencies (PIO), making it suitable for host-based
 * unit testing.
 */

#ifndef OPENTHERM_PROTOCOL_HPP
#define OPENTHERM_PROTOCOL_HPP

#include <cstdint>

// OpenTherm message types
enum class MessageType : uint8_t
{
    READ_DATA = 0,
    WRITE_DATA = 1,
    INVALID_DATA = 2,
    RESERVED = 3,
    READ_ACK = 4,
    WRITE_ACK = 5,
    DATA_INVALID = 6,
    UNKNOWN_DATAID = 7
};

// Maintain C compatibility typedefs
typedef enum
{
    OT_MSGTYPE_READ_DATA = 0,
    OT_MSGTYPE_WRITE_DATA = 1,
    OT_MSGTYPE_INVALID_DATA = 2,
    OT_MSGTYPE_RESERVED = 3,
    OT_MSGTYPE_READ_ACK = 4,
    OT_MSGTYPE_WRITE_ACK = 5,
    OT_MSGTYPE_DATA_INVALID = 6,
    OT_MSGTYPE_UNKNOWN_DATAID = 7
} opentherm_msgtype_t;

// OpenTherm frame structure
typedef struct
{
    uint8_t parity;      // Parity bit
    uint8_t msg_type;    // Message type (3 bits)
    uint8_t spare;       // Spare bits (4 bits) - always 0
    uint8_t data_id;     // Data ID (8 bits)
    uint16_t data_value; // Data value (16 bits)
} opentherm_frame_t;

// Common OpenTherm Data IDs
// Class 1: Status Information
#define OT_DATA_ID_STATUS 0
#define OT_DATA_ID_CONTROL_SETPOINT 1
#define OT_DATA_ID_MASTER_CONFIG 2
#define OT_DATA_ID_SLAVE_CONFIG 3
#define OT_DATA_ID_COMMAND 4
#define OT_DATA_ID_FAULT_FLAGS 5
#define OT_DATA_ID_REMOTE_PARAMS 6
#define OT_DATA_ID_COOLING_CONTROL 7
#define OT_DATA_ID_CONTROL_SETPOINT_2 8

// Class 2: Configuration Information
#define OT_DATA_ID_REMOTE_OVERRIDE 9
#define OT_DATA_ID_TSP_NUMBER 10
#define OT_DATA_ID_TSP_ENTRY 11
#define OT_DATA_ID_FHB_SIZE 12
#define OT_DATA_ID_FHB_ENTRY 13
#define OT_DATA_ID_MAX_REL_MOD 14
#define OT_DATA_ID_MAX_CAPACITY 15

// Class 4: Sensor and Informational Data
#define OT_DATA_ID_ROOM_SETPOINT 16
#define OT_DATA_ID_REL_MOD_LEVEL 17
#define OT_DATA_ID_CH_WATER_PRESS 18
#define OT_DATA_ID_DHW_FLOW_RATE 19
#define OT_DATA_ID_DAY_TIME 20
#define OT_DATA_ID_DATE 21
#define OT_DATA_ID_YEAR 22
#define OT_DATA_ID_ROOM_SETPOINT_CH2 23
#define OT_DATA_ID_ROOM_TEMP 24
#define OT_DATA_ID_BOILER_WATER_TEMP 25
#define OT_DATA_ID_DHW_TEMP 26
#define OT_DATA_ID_OUTSIDE_TEMP 27
#define OT_DATA_ID_RETURN_WATER_TEMP 28
#define OT_DATA_ID_SOLAR_STORAGE_TEMP 29
#define OT_DATA_ID_SOLAR_COLL_TEMP 30
#define OT_DATA_ID_FLOW_TEMP_CH2 31
#define OT_DATA_ID_DHW2_TEMP 32
#define OT_DATA_ID_EXHAUST_TEMP 33

// Class 5: Pre-Defined Remote Boiler Parameters
#define OT_DATA_ID_DHW_BOUNDS 48
#define OT_DATA_ID_CH_BOUNDS 49
#define OT_DATA_ID_DHW_SETPOINT 56
#define OT_DATA_ID_MAX_CH_SETPOINT 57

// Class 4: Additional Informational Data
#define OT_DATA_ID_BURNER_STARTS 116
#define OT_DATA_ID_CH_PUMP_STARTS 117
#define OT_DATA_ID_DHW_PUMP_STARTS 118
#define OT_DATA_ID_DHW_BURNER_STARTS 119
#define OT_DATA_ID_BURNER_HOURS 120
#define OT_DATA_ID_CH_PUMP_HOURS 121
#define OT_DATA_ID_DHW_PUMP_HOURS 122
#define OT_DATA_ID_DHW_BURNER_HOURS 123

// Class 2: OpenTherm Version & Product Info
#define OT_DATA_ID_OPENTHERM_VERSION 124
#define OT_DATA_ID_SLAVE_VERSION 125
#define OT_DATA_ID_MASTER_VERSION 126
#define OT_DATA_ID_SLAVE_PRODUCT 127

// Decode status flags (Data ID 0)
typedef struct
{
    // Master status flags (HB)
    bool ch_enable;
    bool dhw_enable;
    bool cooling_enable;
    bool otc_active;
    bool ch2_enable;

    // Slave status flags (LB)
    bool fault;
    bool ch_mode;
    bool dhw_mode;
    bool flame;
    bool cooling;
    bool ch2_mode;
    bool diagnostic;
} opentherm_status_t;

// Configuration flags
typedef struct
{
    // Configuration bits
    bool dhw_present;
    bool control_type; // 0=modulating, 1=on/off
    bool cooling_config;
    bool dhw_config; // 0=instantaneous, 1=storage tank
    bool master_pump_control;
    bool ch2_present;
} opentherm_config_t;

// Fault flags
typedef struct
{
    uint8_t code; // OEM fault code
    bool service_request;
    bool lockout_reset;
    bool low_water_pressure;
    bool gas_flame_fault;
    bool air_pressure_fault;
    bool water_overtemp;
} opentherm_fault_t;

// Time structure
typedef struct
{
    uint8_t day_of_week; // 1=Monday, 7=Sunday, 0=unknown
    uint8_t hours;       // 0-23
    uint8_t minutes;     // 0-59
} opentherm_time_t;

// Date structure
typedef struct
{
    uint8_t month; // 1-12
    uint8_t day;   // 1-31
} opentherm_date_t;

// Remote parameter flags
typedef struct
{
    bool dhw_setpoint_enable;
    bool max_ch_setpoint_enable;
    bool dhw_setpoint_rw;
    bool max_ch_setpoint_rw;
} opentherm_remote_params_t;

// ============================================================================
// Protocol Function Declarations (C API)
// ============================================================================

namespace OpenTherm
{
    namespace Protocol
    {

        // Calculate even parity for 32-bit frame
        uint8_t calculate_parity(uint32_t frame);

        // Pack frame structure into 32-bit word
        uint32_t pack_frame(const opentherm_frame_t *frame);

        // Unpack 32-bit word into frame structure
        void unpack_frame(uint32_t packed, opentherm_frame_t *frame);

        // Verify frame parity
        bool verify_parity(uint32_t frame);

        // Create a READ-DATA request
        uint32_t build_read_request(uint8_t data_id);

        // Create a WRITE-DATA request
        uint32_t build_write_request(uint8_t data_id, uint16_t data_value);

        // Convert float temperature to f8.8 format
        uint16_t f8_8_from_float(float temp);

        // Convert f8.8 format to float temperature
        float f8_8_to_float(uint16_t value);

        // Extract data value from a frame
        uint16_t get_u16(uint32_t frame);

        // Extract and convert f8.8 temperature value from a frame
        float get_f8_8(uint32_t frame);

        // Extract and convert s16 signed integer from a frame
        int16_t get_s16(uint32_t frame);

        // Extract two u8 bytes from a frame
        void get_u8_u8(uint32_t frame, uint8_t *hb, uint8_t *lb);

        // Encode two bytes into a 16-bit data value (HB:LB)
        uint16_t encode_u8_u8(uint8_t hb, uint8_t lb);

        // Decode 16-bit data value into two bytes (HB:LB)
        void decode_u8_u8(uint16_t value, uint8_t *hb, uint8_t *lb);

        // Encode signed 16-bit integer
        uint16_t encode_s16(int16_t value);

        // Decode signed 16-bit integer
        int16_t decode_s16(uint16_t value);

        // Status encoding/decoding
        void decode_status(uint16_t value, opentherm_status_t *status);
        uint16_t encode_status(const opentherm_status_t *status);

        // Configuration encoding/decoding
        void decode_master_config(uint16_t value, opentherm_config_t *config);
        uint16_t encode_master_config(const opentherm_config_t *config);
        void decode_slave_config(uint16_t value, opentherm_config_t *config);
        uint16_t encode_slave_config(const opentherm_config_t *config);

        // Fault decoding
        void decode_fault(uint16_t value, opentherm_fault_t *fault);

        // Remote parameters decoding
        void decode_remote_params(uint16_t value, opentherm_remote_params_t *params);

        // Time encoding/decoding
        uint16_t encode_time(const opentherm_time_t *time);
        void decode_time(uint16_t value, opentherm_time_t *time);

        // Date encoding/decoding
        uint16_t encode_date(const opentherm_date_t *date);
        void decode_date(uint16_t value, opentherm_date_t *date);

        // Convenience functions for building requests with temperature
        uint32_t write_control_setpoint(float temp);
        uint32_t write_room_setpoint(float temp);
        uint32_t write_room_setpoint_ch2(float temp);
        uint32_t write_dhw_setpoint(float temp);
        uint32_t write_max_ch_setpoint(float temp);
        uint32_t write_day_time(uint8_t day_of_week, uint8_t hours, uint8_t minutes);
        uint32_t write_date(uint8_t month, uint8_t day);
        uint32_t write_year(uint16_t year);

        // Helper functions for reading sensor data
        uint32_t read_status();
        uint32_t read_control_setpoint();
        uint32_t read_master_config();
        uint32_t read_slave_config();
        uint32_t read_fault_flags();
        uint32_t read_remote_params();
        uint32_t read_max_rel_mod();
        uint32_t read_max_capacity();
        uint32_t read_rel_mod_level();
        uint32_t read_ch_water_pressure();
        uint32_t read_dhw_flow_rate();
        uint32_t read_day_time();
        uint32_t read_date();
        uint32_t read_year();
        uint32_t read_room_temp();
        uint32_t read_boiler_water_temp();
        uint32_t read_dhw_temp();
        uint32_t read_outside_temp();
        uint32_t read_return_water_temp();
        uint32_t read_solar_storage_temp();
        uint32_t read_solar_collector_temp();
        uint32_t read_flow_temp_ch2();
        uint32_t read_dhw2_temp();
        uint32_t read_exhaust_temp();
        uint32_t read_dhw_bounds();
        uint32_t read_ch_bounds();
        uint32_t read_dhw_setpoint();
        uint32_t read_max_ch_setpoint();
        uint32_t read_burner_starts();
        uint32_t read_ch_pump_starts();
        uint32_t read_dhw_pump_starts();
        uint32_t read_dhw_burner_starts();
        uint32_t read_burner_hours();
        uint32_t read_ch_pump_hours();
        uint32_t read_dhw_pump_hours();
        uint32_t read_dhw_burner_hours();
        uint32_t read_opentherm_version();
        uint32_t read_slave_version();
        uint32_t read_master_version();
        uint32_t read_slave_product();

        // Manchester encoding/decoding
        bool manchester_decode(uint64_t raw_data, uint32_t *decoded_frame);

    } // namespace Protocol
} // namespace OpenTherm

// C API wrapper functions (for backward compatibility with existing code)
inline uint8_t opentherm_calculate_parity(uint32_t frame) { return OpenTherm::Protocol::calculate_parity(frame); }
inline uint32_t opentherm_pack_frame(const opentherm_frame_t *frame) { return OpenTherm::Protocol::pack_frame(frame); }
inline void opentherm_unpack_frame(uint32_t packed, opentherm_frame_t *frame) { OpenTherm::Protocol::unpack_frame(packed, frame); }
inline bool opentherm_verify_parity(uint32_t frame) { return OpenTherm::Protocol::verify_parity(frame); }
inline uint32_t opentherm_build_read_request(uint8_t data_id) { return OpenTherm::Protocol::build_read_request(data_id); }
inline uint32_t opentherm_build_write_request(uint8_t data_id, uint16_t data_value) { return OpenTherm::Protocol::build_write_request(data_id, data_value); }
inline uint16_t opentherm_f8_8_from_float(float temp) { return OpenTherm::Protocol::f8_8_from_float(temp); }
inline float opentherm_f8_8_to_float(uint16_t value) { return OpenTherm::Protocol::f8_8_to_float(value); }
inline uint16_t opentherm_get_u16(uint32_t frame) { return OpenTherm::Protocol::get_u16(frame); }
inline float opentherm_get_f8_8(uint32_t frame) { return OpenTherm::Protocol::get_f8_8(frame); }
inline int16_t opentherm_get_s16(uint32_t frame) { return OpenTherm::Protocol::get_s16(frame); }
inline void opentherm_get_u8_u8(uint32_t frame, uint8_t *hb, uint8_t *lb) { OpenTherm::Protocol::get_u8_u8(frame, hb, lb); }
inline uint16_t opentherm_encode_u8_u8(uint8_t hb, uint8_t lb) { return OpenTherm::Protocol::encode_u8_u8(hb, lb); }
inline void opentherm_decode_u8_u8(uint16_t value, uint8_t *hb, uint8_t *lb) { OpenTherm::Protocol::decode_u8_u8(value, hb, lb); }
inline uint16_t opentherm_encode_s16(int16_t value) { return OpenTherm::Protocol::encode_s16(value); }
inline int16_t opentherm_decode_s16(uint16_t value) { return OpenTherm::Protocol::decode_s16(value); }
inline void opentherm_decode_status(uint16_t value, opentherm_status_t *status) { OpenTherm::Protocol::decode_status(value, status); }
inline uint16_t opentherm_encode_status(const opentherm_status_t *status) { return OpenTherm::Protocol::encode_status(status); }
inline void opentherm_decode_master_config(uint16_t value, opentherm_config_t *config) { OpenTherm::Protocol::decode_master_config(value, config); }
inline uint16_t opentherm_encode_master_config(const opentherm_config_t *config) { return OpenTherm::Protocol::encode_master_config(config); }
inline void opentherm_decode_slave_config(uint16_t value, opentherm_config_t *config) { OpenTherm::Protocol::decode_slave_config(value, config); }
inline uint16_t opentherm_encode_slave_config(const opentherm_config_t *config) { return OpenTherm::Protocol::encode_slave_config(config); }
inline void opentherm_decode_fault(uint16_t value, opentherm_fault_t *fault) { OpenTherm::Protocol::decode_fault(value, fault); }
inline void opentherm_decode_remote_params(uint16_t value, opentherm_remote_params_t *params) { OpenTherm::Protocol::decode_remote_params(value, params); }
inline uint16_t opentherm_encode_time(const opentherm_time_t *time) { return OpenTherm::Protocol::encode_time(time); }
inline void opentherm_decode_time(uint16_t value, opentherm_time_t *time) { OpenTherm::Protocol::decode_time(value, time); }
inline uint16_t opentherm_encode_date(const opentherm_date_t *date) { return OpenTherm::Protocol::encode_date(date); }
inline void opentherm_decode_date(uint16_t value, opentherm_date_t *date) { OpenTherm::Protocol::decode_date(value, date); }
inline uint32_t opentherm_write_control_setpoint(float temp) { return OpenTherm::Protocol::write_control_setpoint(temp); }
inline uint32_t opentherm_write_room_setpoint(float temp) { return OpenTherm::Protocol::write_room_setpoint(temp); }
inline uint32_t opentherm_write_room_setpoint_ch2(float temp) { return OpenTherm::Protocol::write_room_setpoint_ch2(temp); }
inline uint32_t opentherm_write_dhw_setpoint(float temp) { return OpenTherm::Protocol::write_dhw_setpoint(temp); }
inline uint32_t opentherm_write_max_ch_setpoint(float temp) { return OpenTherm::Protocol::write_max_ch_setpoint(temp); }
inline uint32_t opentherm_write_day_time(uint8_t day_of_week, uint8_t hours, uint8_t minutes) { return OpenTherm::Protocol::write_day_time(day_of_week, hours, minutes); }
inline uint32_t opentherm_write_date(uint8_t month, uint8_t day) { return OpenTherm::Protocol::write_date(month, day); }
inline uint32_t opentherm_write_year(uint16_t year) { return OpenTherm::Protocol::write_year(year); }

// Read helper functions
inline uint32_t opentherm_read_status() { return OpenTherm::Protocol::read_status(); }
inline uint32_t opentherm_read_control_setpoint() { return OpenTherm::Protocol::read_control_setpoint(); }
inline uint32_t opentherm_read_master_config() { return OpenTherm::Protocol::read_master_config(); }
inline uint32_t opentherm_read_slave_config() { return OpenTherm::Protocol::read_slave_config(); }
inline uint32_t opentherm_read_fault_flags() { return OpenTherm::Protocol::read_fault_flags(); }
inline uint32_t opentherm_read_remote_params() { return OpenTherm::Protocol::read_remote_params(); }
inline uint32_t opentherm_read_max_rel_mod() { return OpenTherm::Protocol::read_max_rel_mod(); }
inline uint32_t opentherm_read_max_capacity() { return OpenTherm::Protocol::read_max_capacity(); }
inline uint32_t opentherm_read_rel_mod_level() { return OpenTherm::Protocol::read_rel_mod_level(); }
inline uint32_t opentherm_read_ch_water_pressure() { return OpenTherm::Protocol::read_ch_water_pressure(); }
inline uint32_t opentherm_read_dhw_flow_rate() { return OpenTherm::Protocol::read_dhw_flow_rate(); }
inline uint32_t opentherm_read_day_time() { return OpenTherm::Protocol::read_day_time(); }
inline uint32_t opentherm_read_date() { return OpenTherm::Protocol::read_date(); }
inline uint32_t opentherm_read_year() { return OpenTherm::Protocol::read_year(); }
inline uint32_t opentherm_read_room_temp() { return OpenTherm::Protocol::read_room_temp(); }
inline uint32_t opentherm_read_boiler_water_temp() { return OpenTherm::Protocol::read_boiler_water_temp(); }
inline uint32_t opentherm_read_dhw_temp() { return OpenTherm::Protocol::read_dhw_temp(); }
inline uint32_t opentherm_read_outside_temp() { return OpenTherm::Protocol::read_outside_temp(); }
inline uint32_t opentherm_read_return_water_temp() { return OpenTherm::Protocol::read_return_water_temp(); }
inline uint32_t opentherm_read_solar_storage_temp() { return OpenTherm::Protocol::read_solar_storage_temp(); }
inline uint32_t opentherm_read_solar_collector_temp() { return OpenTherm::Protocol::read_solar_collector_temp(); }
inline uint32_t opentherm_read_flow_temp_ch2() { return OpenTherm::Protocol::read_flow_temp_ch2(); }
inline uint32_t opentherm_read_dhw2_temp() { return OpenTherm::Protocol::read_dhw2_temp(); }
inline uint32_t opentherm_read_exhaust_temp() { return OpenTherm::Protocol::read_exhaust_temp(); }
inline uint32_t opentherm_read_dhw_bounds() { return OpenTherm::Protocol::read_dhw_bounds(); }
inline uint32_t opentherm_read_ch_bounds() { return OpenTherm::Protocol::read_ch_bounds(); }
inline uint32_t opentherm_read_dhw_setpoint() { return OpenTherm::Protocol::read_dhw_setpoint(); }
inline uint32_t opentherm_read_max_ch_setpoint() { return OpenTherm::Protocol::read_max_ch_setpoint(); }
inline uint32_t opentherm_read_burner_starts() { return OpenTherm::Protocol::read_burner_starts(); }
inline uint32_t opentherm_read_ch_pump_starts() { return OpenTherm::Protocol::read_ch_pump_starts(); }
inline uint32_t opentherm_read_dhw_pump_starts() { return OpenTherm::Protocol::read_dhw_pump_starts(); }
inline uint32_t opentherm_read_dhw_burner_starts() { return OpenTherm::Protocol::read_dhw_burner_starts(); }
inline uint32_t opentherm_read_burner_hours() { return OpenTherm::Protocol::read_burner_hours(); }
inline uint32_t opentherm_read_ch_pump_hours() { return OpenTherm::Protocol::read_ch_pump_hours(); }
inline uint32_t opentherm_read_dhw_pump_hours() { return OpenTherm::Protocol::read_dhw_pump_hours(); }
inline uint32_t opentherm_read_dhw_burner_hours() { return OpenTherm::Protocol::read_dhw_burner_hours(); }
inline uint32_t opentherm_read_opentherm_version() { return OpenTherm::Protocol::read_opentherm_version(); }
inline uint32_t opentherm_read_slave_version() { return OpenTherm::Protocol::read_slave_version(); }
inline uint32_t opentherm_read_master_version() { return OpenTherm::Protocol::read_master_version(); }
inline uint32_t opentherm_read_slave_product() { return OpenTherm::Protocol::read_slave_product(); }

// Manchester decoding
inline bool opentherm_manchester_decode(uint64_t raw_data, uint32_t *decoded_frame) { return OpenTherm::Protocol::manchester_decode(raw_data, decoded_frame); }

#endif // OPENTHERM_PROTOCOL_HPP
