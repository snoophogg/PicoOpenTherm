#include "opentherm.hpp"

// Calculate even parity for 32-bit frame
uint8_t opentherm_calculate_parity(uint32_t frame) {
    uint8_t parity = 0;
    for (int i = 0; i < 32; i++) {
        if (frame & (1UL << i)) {
            parity ^= 1;
        }
    }
    return parity;
}

// Pack frame structure into 32-bit word
uint32_t opentherm_pack_frame(const opentherm_frame_t *frame) {
    uint32_t packed = 0;
    
    packed |= (static_cast<uint32_t>(frame->msg_type) & 0x07) << 28;
    packed |= (static_cast<uint32_t>(frame->spare) & 0x0F) << 24;
    packed |= (static_cast<uint32_t>(frame->data_id) & 0xFF) << 16;
    packed |= (static_cast<uint32_t>(frame->data_value) & 0xFFFF);
    
    // Calculate and set parity bit
    uint8_t parity = opentherm_calculate_parity(packed);
    packed |= (static_cast<uint32_t>(parity) << 31);
    
    return packed;
}

// Unpack 32-bit word into frame structure
void opentherm_unpack_frame(uint32_t packed, opentherm_frame_t *frame) {
    frame->parity = (packed >> 31) & 0x01;
    frame->msg_type = (packed >> 28) & 0x07;
    frame->spare = (packed >> 24) & 0x0F;
    frame->data_id = (packed >> 16) & 0xFF;
    frame->data_value = packed & 0xFFFF;
}

// Verify frame parity
bool opentherm_verify_parity(uint32_t frame) {
    uint8_t calculated_parity = opentherm_calculate_parity(frame & 0x7FFFFFFF);
    uint8_t frame_parity = (frame >> 31) & 0x01;
    return calculated_parity == frame_parity;
}

// Create a READ-DATA request
uint32_t opentherm_build_read_request(uint8_t data_id) {
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
uint32_t opentherm_build_write_request(uint8_t data_id, uint16_t data_value) {
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
uint16_t opentherm_f8_8_from_float(float temp) {
    int16_t value = static_cast<int16_t>(temp * 256.0f);
    return static_cast<uint16_t>(value);
}

// Convert f8.8 format to float temperature
float opentherm_f8_8_to_float(uint16_t value) {
    int16_t signed_value = static_cast<int16_t>(value);
    return static_cast<float>(signed_value) / 256.0f;
}
