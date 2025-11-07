#include "led_blink.hpp"
#include "pico/cyw43_arch.h"
#include "hardware/timer.h"
#include <cstdio>

namespace OpenTherm
{
    namespace LED
    {
        // LED state machine - all logic contained here, main code just sets pattern
        static struct
        {
            uint8_t pattern;      // Current blink pattern (0-4)
            uint8_t blink_count;  // Number of blinks in current cycle
            bool led_state;       // Current LED state (on/off)
            uint32_t state_timer; // Timer for current state
            uint32_t cycle_timer; // Timer for full pattern cycle
            bool initialized;
            repeating_timer_t timer;
        } state = {BLINK_CONTINUOUS, 0, false, 0, 0, false};

        // Blink timing constants (in ms)
        static const uint32_t BLINK_ON_TIME = 100;    // LED on duration per blink
        static const uint32_t BLINK_OFF_TIME = 100;   // LED off duration per blink
        static const uint32_t CYCLE_PERIOD = 1000;    // Pattern repeats every 1 second
        static const uint32_t CONTINUOUS_TOGGLE = 50; // Continuous blink toggle time

        // State machine - runs autonomously every 10ms
        static bool led_state_machine(repeating_timer_t *rt)
        {
            uint32_t now = to_ms_since_boot(get_absolute_time());

            // Handle continuous blink mode (fast toggle for critical states)
            if (state.pattern == BLINK_CONTINUOUS)
            {
                if (now - state.state_timer >= CONTINUOUS_TOGGLE)
                {
                    state.led_state = !state.led_state;
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state.led_state);
                    state.state_timer = now;
                }
                return true;
            }

            // Handle pattern-based blinking (1-4 blinks per second)
            uint32_t cycle_elapsed = now - state.cycle_timer;
            uint32_t blink_phase_duration = (BLINK_ON_TIME + BLINK_OFF_TIME) * state.pattern;

            if (cycle_elapsed < blink_phase_duration)
            {
                // We're in the active blinking phase
                uint32_t time_in_phase = cycle_elapsed % (BLINK_ON_TIME + BLINK_OFF_TIME);
                bool should_be_on = (time_in_phase < BLINK_ON_TIME);

                if (state.led_state != should_be_on)
                {
                    state.led_state = should_be_on;
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, state.led_state);
                }
            }
            else
            {
                // We're in the pause phase - LED should be off
                if (state.led_state)
                {
                    state.led_state = false;
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                }

                // Check if cycle is complete
                if (cycle_elapsed >= CYCLE_PERIOD)
                {
                    state.cycle_timer = now; // Start new cycle
                }
            }

            return true;
        }

        bool init()
        {
            if (state.initialized)
            {
                return true;
            }

            printf("Initializing LED state machine...\n");

            // Initialize state - start with continuous blink (indicates not configured)
            state.pattern = BLINK_CONTINUOUS;
            state.blink_count = 0;
            state.led_state = false;
            state.state_timer = to_ms_since_boot(get_absolute_time());
            state.cycle_timer = state.state_timer;

            // Create repeating timer that runs state machine every 10ms
            if (!add_repeating_timer_ms(10, led_state_machine, NULL, &state.timer))
            {
                printf("Failed to create LED state machine timer!\n");
                return false;
            }

            state.initialized = true;
            printf("LED state machine started (10ms tick rate)\n");
            return true;
        }

        void set_pattern(uint8_t pattern)
        {
            if (!state.initialized)
            {
                printf("LED state machine not initialized!\n");
                return;
            }

            // Clamp pattern to valid range
            if (pattern > BLINK_CONFIG_ERROR)
            {
                pattern = BLINK_CONFIG_ERROR;
            }

            // Update pattern and reset timing
            state.pattern = pattern;
            state.led_state = false;
            state.state_timer = to_ms_since_boot(get_absolute_time());
            state.cycle_timer = state.state_timer;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        }

        void stop()
        {
            if (state.initialized)
            {
                cancel_repeating_timer(&state.timer);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                state.initialized = false;
                printf("LED state machine stopped\n");
            }
        }

    } // namespace LED
} // namespace OpenTherm
