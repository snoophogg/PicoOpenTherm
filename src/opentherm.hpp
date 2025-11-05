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
#define OT_DATA_ID_STATUS           0
#define OT_DATA_ID_CONTROL_SETPOINT 1
#define OT_DATA_ID_MASTER_CONFIG    2
#define OT_DATA_ID_SLAVE_CONFIG     3
#define OT_DATA_ID_COMMAND          4
#define OT_DATA_ID_FAULT_FLAGS      5
#define OT_DATA_ID_REMOTE_PARAMS    6
#define OT_DATA_ID_COOLING_CONTROL  7
#define OT_DATA_ID_CONTROL_SETPOINT_2 8

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
