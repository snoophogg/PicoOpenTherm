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

// Encode two bytes into a 16-bit data value (HB:LB)
uint16_t opentherm_encode_u8_u8(uint8_t hb, uint8_t lb) {
    return (static_cast<uint16_t>(hb) << 8) | lb;
}

// Decode 16-bit data value into two bytes (HB:LB)
void opentherm_decode_u8_u8(uint16_t value, uint8_t *hb, uint8_t *lb) {
    *hb = (value >> 8) & 0xFF;
    *lb = value & 0xFF;
}

// Encode signed 16-bit integer
uint16_t opentherm_encode_s16(int16_t value) {
    return static_cast<uint16_t>(value);
}

// Decode signed 16-bit integer
int16_t opentherm_decode_s16(uint16_t value) {
    return static_cast<int16_t>(value);
}

// Decode status flags (Data ID 0)
void opentherm_decode_status(uint16_t value, opentherm_status_t *status) {
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

uint16_t opentherm_encode_status(const opentherm_status_t *status) {
    uint8_t master_flags = 0;
    uint8_t slave_flags = 0;
    
    // Master status flags (HB)
    if (status->ch_enable) master_flags |= 0x01;
    if (status->dhw_enable) master_flags |= 0x02;
    if (status->cooling_enable) master_flags |= 0x04;
    if (status->otc_active) master_flags |= 0x08;
    if (status->ch2_enable) master_flags |= 0x10;
    
    // Slave status flags (LB)
    if (status->fault) slave_flags |= 0x01;
    if (status->ch_mode) slave_flags |= 0x02;
    if (status->dhw_mode) slave_flags |= 0x04;
    if (status->flame) slave_flags |= 0x08;
    if (status->cooling) slave_flags |= 0x10;
    if (status->ch2_mode) slave_flags |= 0x20;
    if (status->diagnostic) slave_flags |= 0x40;
    
    return opentherm_encode_u8_u8(master_flags, slave_flags);
}

// Decode configuration flags
void opentherm_decode_master_config(uint16_t value, opentherm_config_t *config) {
    uint8_t flags = value & 0xFF;
    
    config->dhw_present = (flags & 0x01) != 0;
    config->control_type = (flags & 0x02) != 0;
    config->cooling_config = (flags & 0x04) != 0;
    config->dhw_config = (flags & 0x08) != 0;
    config->master_pump_control = (flags & 0x10) != 0;
    config->ch2_present = (flags & 0x20) != 0;
}

uint16_t opentherm_encode_master_config(const opentherm_config_t *config) {
    uint8_t flags = 0;
    
    if (config->dhw_present) flags |= 0x01;
    if (config->control_type) flags |= 0x02;
    if (config->cooling_config) flags |= 0x04;
    if (config->dhw_config) flags |= 0x08;
    if (config->master_pump_control) flags |= 0x10;
    if (config->ch2_present) flags |= 0x20;
    
    return flags;
}

void opentherm_decode_slave_config(uint16_t value, opentherm_config_t *config) {
    uint8_t flags = value & 0xFF;
    
    config->dhw_present = (flags & 0x01) != 0;
    config->control_type = (flags & 0x02) != 0;
    config->cooling_config = (flags & 0x04) != 0;
    config->dhw_config = (flags & 0x08) != 0;
    config->master_pump_control = (flags & 0x10) != 0;
    config->ch2_present = (flags & 0x20) != 0;
}

uint16_t opentherm_encode_slave_config(const opentherm_config_t *config) {
    uint8_t flags = 0;
    
    if (config->dhw_present) flags |= 0x01;
    if (config->control_type) flags |= 0x02;
    if (config->cooling_config) flags |= 0x04;
    if (config->dhw_config) flags |= 0x08;
    if (config->master_pump_control) flags |= 0x10;
    if (config->ch2_present) flags |= 0x20;
    
    return flags;
}

// Decode fault flags (Data ID 5)
void opentherm_decode_fault(uint16_t value, opentherm_fault_t *fault) {
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
void opentherm_decode_remote_params(uint16_t value, opentherm_remote_params_t *params) {
    uint8_t transfer_enable = (value >> 8) & 0xFF;
    uint8_t rw_flags = value & 0xFF;
    
    params->dhw_setpoint_enable = (transfer_enable & 0x01) != 0;
    params->max_ch_setpoint_enable = (transfer_enable & 0x02) != 0;
    params->dhw_setpoint_rw = (rw_flags & 0x01) != 0;
    params->max_ch_setpoint_rw = (rw_flags & 0x02) != 0;
}

// Encode/decode day of week and time (Data ID 20)
uint16_t opentherm_encode_time(const opentherm_time_t *time) {
    uint8_t hb = ((time->day_of_week & 0x07) << 5) | (time->hours & 0x1F);
    uint8_t lb = time->minutes & 0x3F;
    return opentherm_encode_u8_u8(hb, lb);
}

void opentherm_decode_time(uint16_t value, opentherm_time_t *time) {
    uint8_t hb, lb;
    opentherm_decode_u8_u8(value, &hb, &lb);
    
    time->day_of_week = (hb >> 5) & 0x07;
    time->hours = hb & 0x1F;
    time->minutes = lb & 0x3F;
}

// Encode/decode date (Data ID 21)
uint16_t opentherm_encode_date(const opentherm_date_t *date) {
    return opentherm_encode_u8_u8(date->month, date->day);
}

void opentherm_decode_date(uint16_t value, opentherm_date_t *date) {
    opentherm_decode_u8_u8(value, &date->month, &date->day);
}

// Manchester encoding/decoding
// Decode Manchester encoding: each bit is represented by 2 samples
// '1' = 1,0 (active-to-idle)
// '0' = 0,1 (idle-to-active)
bool opentherm_manchester_decode(uint64_t raw_data, uint32_t *decoded_frame) {
    *decoded_frame = 0;
    
    // Process 34 bit pairs (start + 32 data + stop)
    for (int i = 0; i < 34; i++) {
        int bit_pos = (33 - i) * 2;  // Start from MSB pairs
        uint8_t first_half = (raw_data >> (bit_pos + 1)) & 1;
        uint8_t second_half = (raw_data >> bit_pos) & 1;
        
        if (i == 0 || i == 33) {
            // Start and stop bits should be '1' (1,0 pattern)
            if (first_half != 1 || second_half != 0) {
                return false;  // Invalid frame
            }
        } else {
            // Data bits
            if (first_half == 1 && second_half == 0) {
                // '1' bit
                *decoded_frame |= (1u << (32 - i));
            } else if (first_half == 0 && second_half == 1) {
                // '0' bit - already 0
            } else {
                return false;  // Invalid Manchester encoding
            }
        }
    }
    
    return true;
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

