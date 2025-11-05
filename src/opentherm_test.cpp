#include <cstdio>
#include <cstdint>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "opentherm.hpp"
#include "opentherm_write.pio.h"
#include "opentherm_read.pio.h"

namespace OpenTherm {

// OpenTherm communication class
class Interface {
private:
    PIO pio_tx_;
    PIO pio_rx_;
    uint sm_tx_;
    uint sm_rx_;
    uint tx_pin_;
    uint rx_pin_;

public:
    Interface(uint tx_pin, uint rx_pin, PIO pio_tx = pio0, PIO pio_rx = pio1)
        : pio_tx_(pio_tx), pio_rx_(pio_rx), tx_pin_(tx_pin), rx_pin_(rx_pin) {
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
    
    // Send an OpenTherm frame
    void send(uint32_t frame) {
        opentherm_tx_send_frame(pio_tx_, sm_tx_, frame);
    }
    
    // Receive an OpenTherm frame (non-blocking)
    bool receive(uint32_t& frame) {
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
    
    // Print frame details
    static void printFrame(uint32_t frame_data) {
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
};

} // namespace OpenTherm

// Main application
// Main application
int main() {
    stdio_init_all();
    sleep_ms(2000);  // Wait for USB connection
    
    printf("\n=== OpenTherm PIO Implementation (C++) ===\n");
    printf("Protocol: OpenTherm v2.2\n");
    printf("Encoding: Manchester (Bi-phase-L)\n");
    printf("Bit Rate: 1000 bits/sec\n\n");
    
    // Initialize OpenTherm interface
    constexpr uint OPENTHERM_TX_PIN = 16;
    constexpr uint OPENTHERM_RX_PIN = 17;
    OpenTherm::Interface ot(OPENTHERM_TX_PIN, OPENTHERM_RX_PIN);
    
    printf("\n=== Example: Sending OpenTherm Frames ===\n\n");
    
    // Example 1: Read status (Data ID 0)
    printf("1. Sending READ-DATA request for Status (ID=0):\n");
    auto read_status = opentherm_build_read_request(OT_DATA_ID_STATUS);
    OpenTherm::Interface::printFrame(read_status);
    ot.send(read_status);
    printf("Sent!\n\n");
    
    sleep_ms(1000);
    
    // Example 2: Write control setpoint (Data ID 1)
    printf("2. Sending WRITE-DATA request for Control Setpoint (ID=1):\n");
    constexpr float setpoint_temp = 65.5f;  // 65.5°C
    auto setpoint_value = opentherm_f8_8_from_float(setpoint_temp);
    auto write_setpoint = opentherm_build_write_request(OT_DATA_ID_CONTROL_SETPOINT, setpoint_value);
    OpenTherm::Interface::printFrame(write_setpoint);
    ot.send(write_setpoint);
    printf("Sent!\n\n");
    
    sleep_ms(1000);
    
    // Example 3: Read slave configuration (Data ID 3)
    printf("3. Sending READ-DATA request for Slave Config (ID=3):\n");
    auto read_config = opentherm_build_read_request(OT_DATA_ID_SLAVE_CONFIG);
    OpenTherm::Interface::printFrame(read_config);
    ot.send(read_config);
    printf("Sent!\n\n");
    
    printf("\n=== Listening for responses ===\n");
    printf("(Connect OpenTherm slave device to receive responses)\n\n");
    
    uint32_t frame_count = 0;
    absolute_time_t last_tx = get_absolute_time();
    
    // Main loop: periodically send status requests and listen for responses
    while (true) {
        // Check for received frames
        uint32_t received_frame;
        if (ot.receive(received_frame)) {
            printf("\n[RX] Frame #%lu received:\n", ++frame_count);
            OpenTherm::Interface::printFrame(received_frame);
            printf("\n");
        }
        
        // Send periodic status request every 5 seconds
        if (absolute_time_diff_us(last_tx, get_absolute_time()) > 5000000) {
            last_tx = get_absolute_time();
            
            printf("[TX] Sending periodic status request...\n");
            auto status_req = opentherm_build_read_request(OT_DATA_ID_STATUS);
            ot.send(status_req);
        }
        
        sleep_ms(100);
    }
    
    return 0;
}
