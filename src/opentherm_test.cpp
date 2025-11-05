#include <cstdio>
#include <cstdint>
#include "pico/stdlib.h"
#include "opentherm.hpp"

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
