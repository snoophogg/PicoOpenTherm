#include "opentherm.hpp"
#include "hardware/pio.h"
#include "opentherm_write.pio.h"
#include "opentherm_read.pio.h"
#include <cstdio>

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

// OpenTherm Interface Implementation
namespace OpenTherm {

Interface::Interface(unsigned int tx_pin, unsigned int rx_pin, PIO pio_tx, PIO pio_rx)
    : pio_tx_(pio_tx ? pio_tx : pio0), 
      pio_rx_(pio_rx ? pio_rx : pio1),
      tx_pin_(tx_pin), 
      rx_pin_(rx_pin) {
    
    // Load and initialize TX PIO program
    uint offset_tx = pio_add_program(pio_tx_, &opentherm_tx_program);
    sm_tx_ = pio_claim_unused_sm(pio_tx_, true);
    opentherm_tx_program_init(pio_tx_, sm_tx_, offset_tx, tx_pin_);
    
    // Load and initialize RX PIO program
    uint offset_rx = pio_add_program(pio_rx_, &opentherm_rx_program);
    sm_rx_ = pio_claim_unused_sm(pio_rx_, true);
    opentherm_rx_program_init(pio_rx_, sm_rx_, offset_rx, rx_pin_);
    
    printf("OpenTherm initialized: TX=GPIO%u, RX=GPIO%u\n", tx_pin_, rx_pin_);
}

void Interface::send(uint32_t frame) {
    opentherm_tx_send_frame(pio_tx_, sm_tx_, frame);
}

bool Interface::receive(uint32_t& frame) {
    if (!opentherm_rx_available(pio_rx_, sm_rx_)) {
        return false;
    }
    
    // Get raw Manchester-encoded data
    uint64_t raw_data = opentherm_rx_get_raw(pio_rx_, sm_rx_);
    
    // Decode Manchester encoding
    if (!opentherm_manchester_decode(raw_data, &frame)) {
        printf("Manchester decode error\n");
        return false;
    }
    
    // Verify parity
    if (!opentherm_verify_parity(frame)) {
        printf("Parity error\n");
        return false;
    }
    
    return true;
}

void Interface::printFrame(uint32_t frame_data) {
    opentherm_frame_t frame;
    opentherm_unpack_frame(frame_data, &frame);
    
    printf("Frame: 0x%08lX\n", frame_data);
    printf("  Parity: %u\n", frame.parity);
    printf("  MsgType: %u ", frame.msg_type);
    
    switch(frame.msg_type) {
        case OT_MSGTYPE_READ_DATA: printf("(READ-DATA)\n"); break;
        case OT_MSGTYPE_WRITE_DATA: printf("(WRITE-DATA)\n"); break;
        case OT_MSGTYPE_INVALID_DATA: printf("(INVALID-DATA)\n"); break;
        case OT_MSGTYPE_READ_ACK: printf("(READ-ACK)\n"); break;
        case OT_MSGTYPE_WRITE_ACK: printf("(WRITE-ACK)\n"); break;
        case OT_MSGTYPE_DATA_INVALID: printf("(DATA-INVALID)\n"); break;
        case OT_MSGTYPE_UNKNOWN_DATAID: printf("(UNKNOWN-DATAID)\n"); break;
        default: printf("(RESERVED)\n"); break;
    }
    
    printf("  DataID: %u\n", frame.data_id);
    printf("  DataValue: 0x%04X (%u)\n", frame.data_value, frame.data_value);
    
    // Decode common data values
    if (frame.data_id == OT_DATA_ID_CONTROL_SETPOINT) {
        float temp = opentherm_f8_8_to_float(frame.data_value);
        printf("    -> Temperature: %.2f°C\n", temp);
    }
}

} // namespace OpenTherm

