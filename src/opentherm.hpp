#ifndef OPENTHERM_HPP
#define OPENTHERM_HPP

#include <cstdint>
#include "hardware/pio.h"

// OpenTherm message types
enum class MessageType : uint8_t {
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
typedef enum {
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
typedef struct {
    uint8_t parity;          // Parity bit
    uint8_t msg_type;        // Message type (3 bits)
    uint8_t spare;           // Spare bits (4 bits) - always 0
    uint8_t data_id;         // Data ID (8 bits)
    uint16_t data_value;     // Data value (16 bits)
} opentherm_frame_t;

// Common OpenTherm Data IDs
// Class 1: Status Information
#define OT_DATA_ID_STATUS           0
#define OT_DATA_ID_CONTROL_SETPOINT 1
#define OT_DATA_ID_MASTER_CONFIG    2
#define OT_DATA_ID_SLAVE_CONFIG     3
#define OT_DATA_ID_COMMAND          4
#define OT_DATA_ID_FAULT_FLAGS      5
#define OT_DATA_ID_REMOTE_PARAMS    6
#define OT_DATA_ID_COOLING_CONTROL  7
#define OT_DATA_ID_CONTROL_SETPOINT_2 8

// Class 2: Configuration Information
#define OT_DATA_ID_REMOTE_OVERRIDE  9
#define OT_DATA_ID_TSP_NUMBER       10
#define OT_DATA_ID_TSP_ENTRY        11
#define OT_DATA_ID_FHB_SIZE         12
#define OT_DATA_ID_FHB_ENTRY        13
#define OT_DATA_ID_MAX_REL_MOD      14
#define OT_DATA_ID_MAX_CAPACITY     15

// Class 4: Sensor and Informational Data
#define OT_DATA_ID_ROOM_SETPOINT    16
#define OT_DATA_ID_REL_MOD_LEVEL    17
#define OT_DATA_ID_CH_WATER_PRESS   18
#define OT_DATA_ID_DHW_FLOW_RATE    19
#define OT_DATA_ID_DAY_TIME         20
#define OT_DATA_ID_DATE             21
#define OT_DATA_ID_YEAR             22
#define OT_DATA_ID_ROOM_SETPOINT_CH2 23
#define OT_DATA_ID_ROOM_TEMP        24
#define OT_DATA_ID_BOILER_WATER_TEMP 25
#define OT_DATA_ID_DHW_TEMP         26
#define OT_DATA_ID_OUTSIDE_TEMP     27
#define OT_DATA_ID_RETURN_WATER_TEMP 28
#define OT_DATA_ID_SOLAR_STORAGE_TEMP 29
#define OT_DATA_ID_SOLAR_COLL_TEMP  30
#define OT_DATA_ID_FLOW_TEMP_CH2    31
#define OT_DATA_ID_DHW2_TEMP        32
#define OT_DATA_ID_EXHAUST_TEMP     33

// Class 5: Pre-Defined Remote Boiler Parameters  
#define OT_DATA_ID_DHW_BOUNDS       48
#define OT_DATA_ID_CH_BOUNDS        49
#define OT_DATA_ID_DHW_SETPOINT     56
#define OT_DATA_ID_MAX_CH_SETPOINT  57

// Class 4: Additional Informational Data
#define OT_DATA_ID_BURNER_STARTS    116
#define OT_DATA_ID_CH_PUMP_STARTS   117
#define OT_DATA_ID_DHW_PUMP_STARTS  118
#define OT_DATA_ID_DHW_BURNER_STARTS 119
#define OT_DATA_ID_BURNER_HOURS     120
#define OT_DATA_ID_CH_PUMP_HOURS    121
#define OT_DATA_ID_DHW_PUMP_HOURS   122
#define OT_DATA_ID_DHW_BURNER_HOURS 123

// Class 2: OpenTherm Version & Product Info
#define OT_DATA_ID_OPENTHERM_VERSION 124
#define OT_DATA_ID_SLAVE_VERSION    125
#define OT_DATA_ID_MASTER_VERSION   126
#define OT_DATA_ID_SLAVE_PRODUCT    127

// Status flags (Data ID 0)
#define OT_STATUS_MASTER_CH_ENABLE      (1 << 0)
#define OT_STATUS_MASTER_DHW_ENABLE     (1 << 1)
#define OT_STATUS_MASTER_COOLING_ENABLE (1 << 2)
#define OT_STATUS_MASTER_OTC_ACTIVE     (1 << 3)
#define OT_STATUS_MASTER_CH2_ENABLE     (1 << 4)

#define OT_STATUS_SLAVE_FAULT           (1 << 0)
#define OT_STATUS_SLAVE_CH_MODE         (1 << 1)
#define OT_STATUS_SLAVE_DHW_MODE        (1 << 2)
#define OT_STATUS_SLAVE_FLAME           (1 << 3)
#define OT_STATUS_SLAVE_COOLING         (1 << 4)
#define OT_STATUS_SLAVE_CH2_MODE        (1 << 5)
#define OT_STATUS_SLAVE_DIAGNOSTIC      (1 << 6)

// Function prototypes

// Calculate even parity for 32-bit frame
uint8_t opentherm_calculate_parity(uint32_t frame);

// Pack frame structure into 32-bit word
uint32_t opentherm_pack_frame(const opentherm_frame_t *frame);

// Unpack 32-bit word into frame structure
void opentherm_unpack_frame(uint32_t packed, opentherm_frame_t *frame);

// Verify frame parity
bool opentherm_verify_parity(uint32_t frame);

// Create a READ-DATA request
uint32_t opentherm_build_read_request(uint8_t data_id);

// Create a WRITE-DATA request
uint32_t opentherm_build_write_request(uint8_t data_id, uint16_t data_value);

// Convert float temperature to f8.8 format
uint16_t opentherm_f8_8_from_float(float temp);

// Convert f8.8 format to float temperature
float opentherm_f8_8_to_float(uint16_t value);

// Encode/Decode functions for specific data types

// Encode two bytes into a 16-bit data value (HB:LB)
uint16_t opentherm_encode_u8_u8(uint8_t hb, uint8_t lb);

// Decode 16-bit data value into two bytes (HB:LB)
void opentherm_decode_u8_u8(uint16_t value, uint8_t *hb, uint8_t *lb);

// Encode signed 16-bit integer
uint16_t opentherm_encode_s16(int16_t value);

// Decode signed 16-bit integer
int16_t opentherm_decode_s16(uint16_t value);

// Decode status flags (Data ID 0)
typedef struct {
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

void opentherm_decode_status(uint16_t value, opentherm_status_t *status);
uint16_t opentherm_encode_status(const opentherm_status_t *status);

// Decode configuration flags
typedef struct {
    // Configuration bits
    bool dhw_present;
    bool control_type;  // 0=modulating, 1=on/off
    bool cooling_config;
    bool dhw_config;  // 0=instantaneous, 1=storage tank
    bool master_pump_control;
    bool ch2_present;
} opentherm_config_t;

void opentherm_decode_master_config(uint16_t value, opentherm_config_t *config);
uint16_t opentherm_encode_master_config(const opentherm_config_t *config);
void opentherm_decode_slave_config(uint16_t value, opentherm_config_t *config);
uint16_t opentherm_encode_slave_config(const opentherm_config_t *config);

// Decode fault flags (Data ID 5)
typedef struct {
    uint8_t code;  // OEM fault code
    bool service_request;
    bool lockout_reset;
    bool low_water_pressure;
    bool gas_flame_fault;
    bool air_pressure_fault;
    bool water_overtemp;
} opentherm_fault_t;

void opentherm_decode_fault(uint16_t value, opentherm_fault_t *fault);

// Decode remote parameter flags (Data ID 6)
typedef struct {
    bool dhw_setpoint_enable;
    bool max_ch_setpoint_enable;
    bool dhw_setpoint_rw;
    bool max_ch_setpoint_rw;
} opentherm_remote_params_t;

void opentherm_decode_remote_params(uint16_t value, opentherm_remote_params_t *params);

// Encode/decode day of week and time (Data ID 20)
typedef struct {
    uint8_t day_of_week;  // 1=Monday, 7=Sunday, 0=unknown
    uint8_t hours;        // 0-23
    uint8_t minutes;      // 0-59
} opentherm_time_t;

uint16_t opentherm_encode_time(const opentherm_time_t *time);
void opentherm_decode_time(uint16_t value, opentherm_time_t *time);

// Encode/decode date (Data ID 21)
typedef struct {
    uint8_t month;  // 1-12
    uint8_t day;    // 1-31
} opentherm_date_t;

uint16_t opentherm_encode_date(const opentherm_date_t *date);
void opentherm_decode_date(uint16_t value, opentherm_date_t *date);

// Manchester encoding
bool opentherm_manchester_decode(uint64_t raw_data, uint32_t *decoded_frame);

// C++ OpenTherm Interface
namespace OpenTherm {

class Interface {
private:
    PIO pio_tx_;
    PIO pio_rx_;
    unsigned int sm_tx_;
    unsigned int sm_rx_;
    unsigned int tx_pin_;
    unsigned int rx_pin_;

public:
    // Constructor
    Interface(unsigned int tx_pin, unsigned int rx_pin, PIO pio_tx = pio0, PIO pio_rx = pio1);
    
    // Send an OpenTherm frame
    void send(uint32_t frame);
    
    // Receive an OpenTherm frame (non-blocking)
    bool receive(uint32_t& frame);
    
    // Print frame details
    static void printFrame(uint32_t frame_data);
};

} // namespace OpenTherm

#endif // OPENTHERM_HPP
