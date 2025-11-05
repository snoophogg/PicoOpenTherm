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
    
    printf("\n=== Main Loop: Send requests and listen for responses ===\n");
    printf("(Connect OpenTherm slave device to receive responses)\n\n");
    
    uint32_t frame_count = 0;
    absolute_time_t last_tx = get_absolute_time();
    
    // Main loop: periodically send status requests and listen for responses
    while (true) {
        // Send periodic status request every 5 seconds
        if (absolute_time_diff_us(last_tx, get_absolute_time()) > 5000000) {
            last_tx = get_absolute_time();
            
            printf("[TX] Sending periodic status request...\n");
            auto status_req = opentherm_build_read_request(OT_DATA_ID_STATUS);
            ot.send(status_req);
        }
        
        // Check for received frames
        uint32_t received_frame;
        if (ot.receive(received_frame)) {
            printf("\n[RX] Frame #%lu received:\n", ++frame_count);
            OpenTherm::Interface::printFrame(received_frame);
            printf("\n");
        }
        
        sleep_ms(100);
    }
    
    return 0;
}
