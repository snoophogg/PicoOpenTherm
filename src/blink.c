#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/pio.h"
#include "blink.pio.h"

// Use the onboard LED on Pico W (connected via CYW43 chip)
// We'll use GPIO 25 for a regular GPIO blink via PIO
// For Pico W onboard LED, we need to use cyw43_arch functions

#define BLINK_PIN 25  // For regular Pico, this is onboard LED
                       // For Pico W, use external LED on this pin

int main() {
    stdio_init_all();
    
    // Initialize CYW43 architecture (needed for Pico W)
    if (cyw43_arch_init()) {
        printf("Wi-Fi init failed\n");
        return -1;
    }

    printf("PIO Blink Example for Pico W\n");
    
    // We'll blink the CYW43 LED (onboard) using traditional method
    // and also demonstrate PIO on an external GPIO pin
    
    // PIO Blink setup for external pin (GPIO 25)
    PIO pio = pio0;
    uint offset = pio_add_program(pio, &blink_program);
    uint sm = pio_claim_unused_sm(pio, true);
    
    // Initialize the PIO blink program
    // Parameters: PIO instance, state machine, program offset, pin, PIO frequency, blink frequency
    blink_program_init(pio, sm, offset, BLINK_PIN, 2000, 1.0f); // 1 Hz blink
    
    printf("PIO blink running on GPIO %d\n", BLINK_PIN);
    printf("Onboard LED (CYW43) will also blink\n");
    printf("Starting blink loop...\n\n");
    
    // Blink counter
    uint32_t blink_count = 0;
    
    // Also blink the onboard LED (CYW43) in the main loop
    while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        printf("Blink #%lu: LED ON\n", blink_count);
        sleep_ms(250);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        printf("Blink #%lu: LED OFF\n", blink_count);
        sleep_ms(250);
        blink_count++;
    }

    // Cleanup (never reached in this example)
    cyw43_arch_deinit();
    
    return 0;
}
