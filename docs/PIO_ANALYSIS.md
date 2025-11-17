# OpenTherm PIO Implementation Analysis

## Overview

The OpenTherm protocol implementation uses two Raspberry Pi Pico PIO (Programmable I/O) state machines for hardware-accelerated Manchester encoding/decoding:
- **TX PIO** (`opentherm_write.pio`) - Transmits OpenTherm frames
- **RX PIO** (`opentherm_read.pio`) - Receives OpenTherm frames

## OpenTherm Protocol Requirements

- **Bit Rate**: 1000 bits/sec (1ms per bit)
- **Encoding**: Manchester (Bi-phase-L)
  - `'1'` = active-to-idle transition (high-to-low)
  - `'0'` = idle-to-active transition (low-to-high)
- **Half-bit Period**: 500µs (each bit has two 500µs halves)
- **Frame Format**: Start bit (1) + 32 data bits + Stop bit (1) = 34 bits total

## TX PIO Analysis (`opentherm_write.pio`)

### Current Implementation

```pio
.program opentherm_tx
    pull block              ; Wait for 32-bit frame
    set x, 31               ; Bit counter

send_start_bit:
    set pins, 1 [31]        ; Active (500µs)
    set pins, 0 [31]        ; Idle (500µs)

bit_loop:
    out y, 1                ; Get next bit
    jmp !y, send_zero
send_one:
    set pins, 1 [31]        ; Active
    set pins, 0 [31]        ; Idle
    jmp next_bit
send_zero:
    set pins, 0 [31]        ; Idle
    set pins, 1 [31]        ; Active
next_bit:
    jmp x--, bit_loop

send_stop_bit:
    set pins, 1 [31]        ; Active
    set pins, 0 [31]        ; Idle
    jmp start
```

### Timing Analysis

**Clock Divider Calculation:**
```c
float div = clock_get_hz(clk_sys) / (66000.0f);  // ~66kHz PIO clock
```

At 125MHz system clock:
- Divider = 125MHz / 66kHz = **1893.94**
- PIO clock = 125MHz / 1893.94 = **66.0kHz**
- Each instruction cycle = **15.15µs**

**Delay Calculation:**
- `[31]` delay = 31 + 1 (instruction) = 32 cycles
- Half-bit time = 32 × 15.15µs = **485µs**
- **Target: 500µs** → **Error: -3%**

### ✅ Status: WORKING (with minor timing deviation)

The timing is slightly fast but within acceptable tolerance for OpenTherm (±10% typical). Most boilers can handle this.

### 🔧 Suggested Improvement

For more accurate timing:

```c
// Target: 500µs per half-bit
// With [31] delay = 32 cycles per half-bit
// PIO freq = 32 / 500µs = 64kHz
float div = clock_get_hz(clk_sys) / (64000.0f);  // 64kHz PIO clock
```

This gives:
- Divider = 125MHz / 64kHz = **1953.125**
- PIO clock = **64kHz**
- Half-bit time = 32 / 64kHz = **500µs exactly**

## RX PIO Analysis (`opentherm_read.pio`)

### Current Implementation

```pio
.program opentherm_rx
.wrap_target
wait_for_start:
    wait 1 pin 0            ; Wait for start edge
    set x, 31               ; Bit counter
    nop [15]                ; Center of first half
    in pins, 1              ; Sample first half
    nop [31]                ; Wait for second half
    in pins, 1              ; Sample second half

bit_loop:
    nop [15]                ; Center of first half
    in pins, 1
    nop [31]                ; Second half
    in pins, 1
    jmp x--, bit_loop

    ; Stop bit
    nop [15]
    in pins, 1
    nop [31]
    in pins, 1
    push block
.wrap
```

### Timing Analysis

**Clock Divider:**
```c
float div = clock_get_hz(clk_sys) / (2000000.0f);  // 2MHz PIO clock
```

At 125MHz system clock:
- Divider = 125MHz / 2MHz = **62.5**
- PIO clock = **2MHz**
- Each instruction cycle = **0.5µs**

**Sampling Points:**
- `[15]` delay = 15 + 1 = 16 cycles = 16 × 0.5µs = **8µs**
- `[31]` delay = 31 + 1 = 32 cycles = 32 × 0.5µs = **16µs**

**Start Bit Timing:**
1. `wait 1 pin 0` - triggers on rising edge
2. `nop [15]` - delay 8µs from edge
3. `in pins, 1` - sample first half at ~8µs
4. `nop [31]` - wait 16µs
5. `in pins, 1` - sample second half at ~24µs

### ⚠️ Issues Identified

**Problem 1: Incorrect Clock Speed**

RX uses 2MHz PIO clock but TX uses 66kHz. The comment says "Same timing as TX" but this is false:
- TX: 66kHz PIO clock
- RX: 2MHz PIO clock (30× faster!)

**Problem 2: Wrong Sample Timing**

With a 2MHz PIO clock:
- First sample at 8µs after edge
- Second sample at 24µs after edge
- **This is sampling too early!**

For a 1ms bit period (500µs half-bits):
- First sample should be at ~250µs (center of first half)
- Second sample should be at ~750µs (center of second half)

**Problem 3: Autopush Configuration**

```c
sm_config_set_in_shift(&c, false, true, 32);  // Autopush after 32 bits
```

But the PIO pushes 64 bits (2 samples × 33 bits = 66 samples). The autopush at 32 will split the data incorrectly.

### 🔧 Suggested Fix for RX PIO

**Option 1: Match TX Clock Speed (Recommended)**

```c
static inline void opentherm_rx_program_init(PIO pio, uint sm, uint offset, uint pin) {
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, false);

    pio_sm_config c = opentherm_rx_program_get_default_config(offset);
    sm_config_set_in_pins(&c, pin);

    // Match TX timing: 64kHz PIO clock for 500µs half-bits
    float div = clock_get_hz(clk_sys) / (64000.0f);
    sm_config_set_clkdiv(&c, div);

    // Shift in MSB first, autopush after 64 bits (2 samples × 32 bits)
    sm_config_set_in_shift(&c, false, true, 64);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}
```

**Option 2: Adjust RX PIO for Correct Sampling**

If keeping 2MHz clock, recalculate delays:
- Target: Sample at 250µs and 750µs from edge
- At 2MHz (0.5µs/cycle):
  - First delay: 250µs / 0.5µs = 500 cycles (not possible with PIO [n] max of 31!)

**Conclusion**: The RX clock MUST be slowed down to match the bit rate.

## Manchester Decoding

The software Manchester decoder in `opentherm_protocol.cpp` expects:
- 64 bits of raw samples (2 samples per bit × 32 bits)
- `10` pattern = '1' bit (active-to-idle)
- `01` pattern = '0' bit (idle-to-active)

This is correct and well-implemented.

## Overall Assessment

| Component | Status | Issue | Priority |
|-----------|--------|-------|----------|
| TX PIO | ✅ Working | Timing -3% fast | Low |
| RX PIO | ⚠️ Problematic | Wrong clock speed, wrong sampling | **HIGH** |
| Manchester Decode | ✅ Correct | None | - |
| Parity Check | ✅ Correct | None | - |

## Recommended Actions

### Critical (RX PIO)

1. **Fix RX clock divider** to match TX timing (64kHz recommended)
2. **Test with real OpenTherm device** to verify reception works
3. **Add error counters** to track failed decodes in production

### Optional Improvements

1. **Improve TX timing accuracy** (66kHz → 64kHz)
2. **Add timeout handling** in RX for malformed frames
3. **Add FIFO overflow detection** in both TX and RX
4. **Implement frame queuing** to handle burst traffic

## Testing Recommendations

### Unit Tests
- Generate test frames at exact 1kHz bit rate
- Verify TX output with logic analyzer
- Test RX with simulated Manchester-encoded input
- Test edge cases (all 0s, all 1s, alternating pattern)

### Integration Tests
- Connect to real OpenTherm boiler/thermostat
- Monitor error rates over 24 hours
- Test with different boiler brands (timing tolerance varies)
- Test at different temperatures (check clock stability)

## References

- OpenTherm Protocol Specification v2.2
- Manchester Encoding: IEEE 802.3 style (Bi-phase-L)
- Raspberry Pi Pico PIO Documentation
- Pico SDK PIO Examples
