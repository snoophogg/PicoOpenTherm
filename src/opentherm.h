#ifndef OPENTHERM_H
#define OPENTHERM_H

#include <stdint.h>
#include <stdbool.h>

// OpenTherm message types
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
static inline uint8_t opentherm_calculate_parity(uint32_t frame) {
    uint8_t parity = 0;
    for (int i = 0; i < 32; i++) {
        if (frame & (1UL << i)) {
            parity ^= 1;
        }
    }
    return parity;
}

// Pack frame structure into 32-bit word
static inline uint32_t opentherm_pack_frame(const opentherm_frame_t *frame) {
    uint32_t packed = 0;
    
    packed |= ((uint32_t)frame->msg_type & 0x07) << 28;
    packed |= ((uint32_t)frame->spare & 0x0F) << 24;
    packed |= ((uint32_t)frame->data_id & 0xFF) << 16;
    packed |= ((uint32_t)frame->data_value & 0xFFFF);
    
    // Calculate and set parity bit
    uint8_t parity = opentherm_calculate_parity(packed);
    packed |= ((uint32_t)parity << 31);
    
    return packed;
}

// Unpack 32-bit word into frame structure
static inline void opentherm_unpack_frame(uint32_t packed, opentherm_frame_t *frame) {
    frame->parity = (packed >> 31) & 0x01;
    frame->msg_type = (packed >> 28) & 0x07;
    frame->spare = (packed >> 24) & 0x0F;
    frame->data_id = (packed >> 16) & 0xFF;
    frame->data_value = packed & 0xFFFF;
}

// Verify frame parity
static inline bool opentherm_verify_parity(uint32_t frame) {
    uint8_t calculated_parity = opentherm_calculate_parity(frame & 0x7FFFFFFF);
    uint8_t frame_parity = (frame >> 31) & 0x01;
    return calculated_parity == frame_parity;
}

// Create a READ-DATA request
static inline uint32_t opentherm_build_read_request(uint8_t data_id) {
    opentherm_frame_t frame = {
        .parity = 0,  // Will be calculated
        .msg_type = OT_MSGTYPE_READ_DATA,
        .spare = 0,
        .data_id = data_id,
        .data_value = 0x0000
    };
    return opentherm_pack_frame(&frame);
}

// Create a WRITE-DATA request
static inline uint32_t opentherm_build_write_request(uint8_t data_id, uint16_t data_value) {
    opentherm_frame_t frame = {
        .parity = 0,  // Will be calculated
        .msg_type = OT_MSGTYPE_WRITE_DATA,
        .spare = 0,
        .data_id = data_id,
        .data_value = data_value
    };
    return opentherm_pack_frame(&frame);
}

// Convert float temperature to f8.8 format
static inline uint16_t opentherm_f8_8_from_float(float temp) {
    int16_t value = (int16_t)(temp * 256.0f);
    return (uint16_t)value;
}

// Convert f8.8 format to float temperature
static inline float opentherm_f8_8_to_float(uint16_t value) {
    int16_t signed_value = (int16_t)value;
    return (float)signed_value / 256.0f;
}

#endif // OPENTHERM_H
