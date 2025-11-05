#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "opentherm.h"
#include "opentherm_write.pio.h"
#include "opentherm_read.pio.h"

// OpenTherm GPIO pins (configurable)
#define OPENTHERM_TX_PIN 16    // GPIO for transmitting OpenTherm frames
#define OPENTHERM_RX_PIN 17    // GPIO for receiving OpenTherm frames

// PIO instances
static PIO pio_tx = pio0;
static PIO pio_rx = pio1;
static uint sm_tx = 0;
static uint sm_rx = 0;

// Initialize OpenTherm communication
void opentherm_init(uint tx_pin, uint rx_pin) {
    // Load and initialize TX PIO program
    uint offset_tx = pio_add_program(pio_tx, &opentherm_tx_program);
    sm_tx = pio_claim_unused_sm(pio_tx, true);
    opentherm_tx_program_init(pio_tx, sm_tx, offset_tx, tx_pin);
    
    // Load and initialize RX PIO program
    uint offset_rx = pio_add_program(pio_rx, &opentherm_rx_program);
    sm_rx = pio_claim_unused_sm(pio_rx, true);
    opentherm_rx_program_init(pio_rx, sm_rx, offset_rx, rx_pin);
    
    printf("OpenTherm initialized: TX=GPIO%d, RX=GPIO%d\n", tx_pin, rx_pin);
}

// Send an OpenTherm frame
void opentherm_send(uint32_t frame) {
    opentherm_tx_send_frame(pio_tx, sm_tx, frame);
}

// Receive an OpenTherm frame (non-blocking)
bool opentherm_receive(uint32_t *frame) {
    if (!opentherm_rx_available(pio_rx, sm_rx)) {
        return false;
    }
    
    // Get raw Manchester-encoded data
    uint64_t raw_data = opentherm_rx_get_raw(pio_rx, sm_rx);
    
    // Decode Manchester encoding
    if (!opentherm_manchester_decode(raw_data, frame)) {
        printf("Manchester decode error\n");
        return false;
    }
    
    // Verify parity
    if (!opentherm_verify_parity(*frame)) {
        printf("Parity error\n");
        return false;
    }
    
    return true;
}

// Print OpenTherm frame details
void opentherm_print_frame(uint32_t frame_data) {
    opentherm_frame_t frame;
    opentherm_unpack_frame(frame_data, &frame);
    
    printf("Frame: 0x%08lX\n", frame_data);
    printf("  Parity: %d\n", frame.parity);
    printf("  MsgType: %d ", frame.msg_type);
    
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
    
    printf("  DataID: %d\n", frame.data_id);
    printf("  DataValue: 0x%04X (%d)\n", frame.data_value, frame.data_value);
    
    // Decode common data values
    if (frame.data_id == OT_DATA_ID_CONTROL_SETPOINT) {
        float temp = opentherm_f8_8_to_float(frame.data_value);
        printf("    -> Temperature: %.2f°C\n", temp);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);  // Wait for USB connection
    
    printf("\n=== OpenTherm PIO Implementation ===\n");
    printf("Protocol: OpenTherm v2.2\n");
    printf("Encoding: Manchester (Bi-phase-L)\n");
    printf("Bit Rate: 1000 bits/sec\n\n");
    
    // Initialize OpenTherm PIO
    opentherm_init(OPENTHERM_TX_PIN, OPENTHERM_RX_PIN);
    
    printf("\n=== Example: Sending OpenTherm Frames ===\n\n");
    
    // Example 1: Read status (Data ID 0)
    printf("1. Sending READ-DATA request for Status (ID=0):\n");
    uint32_t read_status = opentherm_build_read_request(OT_DATA_ID_STATUS);
    opentherm_print_frame(read_status);
    opentherm_send(read_status);
    printf("Sent!\n\n");
    
    sleep_ms(1000);
    
    // Example 2: Write control setpoint (Data ID 1)
    printf("2. Sending WRITE-DATA request for Control Setpoint (ID=1):\n");
    float setpoint_temp = 65.5f;  // 65.5°C
    uint16_t setpoint_value = opentherm_f8_8_from_float(setpoint_temp);
    uint32_t write_setpoint = opentherm_build_write_request(OT_DATA_ID_CONTROL_SETPOINT, setpoint_value);
    opentherm_print_frame(write_setpoint);
    opentherm_send(write_setpoint);
    printf("Sent!\n\n");
    
    sleep_ms(1000);
    
    // Example 3: Read slave configuration (Data ID 3)
    printf("3. Sending READ-DATA request for Slave Config (ID=3):\n");
    uint32_t read_config = opentherm_build_read_request(OT_DATA_ID_SLAVE_CONFIG);
    opentherm_print_frame(read_config);
    opentherm_send(read_config);
    printf("Sent!\n\n");
    
    printf("\n=== Listening for responses ===\n");
    printf("(Connect OpenTherm slave device to receive responses)\n\n");
    
    uint32_t frame_count = 0;
    
    // Main loop: periodically send status requests and listen for responses
    while (true) {
        // Check for received frames
        uint32_t received_frame;
        if (opentherm_receive(&received_frame)) {
            printf("\n[RX] Frame #%lu received:\n", ++frame_count);
            opentherm_print_frame(received_frame);
            printf("\n");
        }
        
        // Send periodic status request every 5 seconds
        static absolute_time_t last_tx = {0};
        if (absolute_time_diff_us(last_tx, get_absolute_time()) > 5000000) {
            last_tx = get_absolute_time();
            
            printf("[TX] Sending periodic status request...\n");
            uint32_t status_req = opentherm_build_read_request(OT_DATA_ID_STATUS);
            opentherm_send(status_req);
        }
        
        sleep_ms(100);
    }
    
    return 0;
}
