#ifndef LED_BLINK_HPP
#define LED_BLINK_HPP

#include "pico/stdlib.h"

namespace OpenTherm
{
    namespace LED
    {
        // LED blink patterns
        constexpr uint8_t BLINK_CONTINUOUS = 0;   // Continuous fast blink (fatal error)
        constexpr uint8_t BLINK_NORMAL = 1;       // 1 blink per second (normal operation)
        constexpr uint8_t BLINK_OK = 1;          // 1 blink per second (normal operation)
        constexpr uint8_t BLINK_WIFI_ERROR = 2;   // 2 blinks per second (WiFi error)
        constexpr uint8_t BLINK_MQTT_ERROR = 3;   // 3 blinks per second (MQTT error)
        constexpr uint8_t BLINK_CONFIG_ERROR = 4; // 4 blinks per second (config error)

        // Initialize the timer-based LED blink system
        // Uses the CYW43 WiFi chip's LED (requires cyw43_arch to be initialized first)
        bool init();

        // Set the LED blink pattern
        // blink_count: 0 = continuous fast blink, N = N blinks then pause
        void set_pattern(uint8_t blink_count);

        // Stop the LED blinking (cleanup)
        void stop();

    } // namespace LED
} // namespace OpenTherm

#endif // LED_BLINK_HPP
