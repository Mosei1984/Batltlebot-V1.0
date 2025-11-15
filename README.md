# ESP32 Battlebot

High-performance combat robot control system built on ESP32 with Bluetooth app integration and WS2812B LED visualization.

## Features

- **Precision Control**: 10ms main loop interval for responsive driving and weapon control
- **Bluetooth Remote**: Full wireless control via smartphone app
- **WS2812B LED System**: 3-zone RGB LED visualization (drive left, weapon center, drive right)
- **Robust Failsafe**: 60-second link timeout with intelligent weapon state preservation
- **Modular Architecture**: Clean separation of concerns across Drive, Weapon, Bluetooth, LED, Failsafe, and Diagnostics modules

## Hardware

- **MCU**: ESP32-WROOM-32
- **LEDs**: WS2812B addressable RGB (GPIO 23)
- **Communication**: Bluetooth Classic
- **Motor Control**: PWM-based drive and weapon control

## System Architecture

### Core Modules

- **main.cpp**: Main control loop with strict 10ms timing
- **Drive**: Motor control with left/right differential steering
- **Weapon**: Arming sequence and weapon motor control with notch filtering
- **NotchFilter**: Dynamic resonance avoidance for weapon ESC output
- **Leds**: Non-blocking WS2812B LED effects with state-based visualization
- **BluetoothComm**: Bluetooth + USB Serial communication handler
- **CommandParser**: Command protocol parser for app integration
- **Failsafe**: Link timeout monitoring with weapon-aware behavior
- **Diagnostics**: Error tracking and system health monitoring

### Timing Constraints

- **Main Loop**: 10ms interval (LOOP_INTERVAL_MS)
- **LED Update**: 20ms throttle (LED_TICK_MS)
- **Failsafe Timeout**: 60 seconds (FAILSAFE_LINK_TIMEOUT_MS)
- **Non-blocking**: All operations use state machines, no delay() calls

## Failsafe Behavior

The failsafe system prioritizes safety while allowing operational flexibility:

- **Link Timeout**: 60 seconds without any commands triggers failsafe
- **Weapon Armed**: When weapon is armed, motors can continue running during steady input
- **Failsafe Action**: Weapon idles automatically, motors maintain last command until timeout

## LED Visualization

3-zone WS2812B LED strip provides real-time status feedback:

- **Idle Mode**: Soft pulsing (blue/cyan)
- **Drive Mode**: Direction-based effects
- **Weapon Arming**: Warning flash pattern (yellow)
- **Weapon Armed**: Solid red with pulse effect
- **Error/Failsafe**: Red double-blink pattern

## Command Protocol

### Motion & Function Commands (Bluetooth/Serial)
- **Motion**: 6-character format (e.g., `F99R50` = Forward 99%, Right turn 50%)
- **Weapon Arming**: `U` (arm request), `u` (disarm), `W` (full throttle), `w` (idle)
- **LED Commands**: `L0` (off), `L1RRGGBB` (solid color), `LA` (auto mode)

### Notch Filter Commands (USB Serial or Bluetooth)

Dynamic ESC output filtering to avoid mechanical resonances:

| Command | Description | Example |
|---------|-------------|---------|
| `NFEN=1` | Enable notch filter | `NFEN=1` |
| `NFEN=0` | **Disable notch filter (SAFETY!)** | `NFEN=0` |
| `NF+<center>,<width>,<depth>` | Add notch | `NF+1500,100,0.7` |
| `NF-` | Clear all notches | `NF-` |
| `NF?` | Show current config | `NF?` |
| `NF#<id>` | Remove notch by ID | `NF#1` |

**Parameters:**
- `center`: PWM center frequency in µs (e.g., 1500)
- `width`: Half-width in µs (±range, e.g., 100 = ±100µs)
- `depth`: Attenuation depth 0.0-1.0 (0=no effect, 1=full damping)

**Example Session:**
```
NFEN=1              // Enable filter (default: OFF)
NF+1500,100,0.7     // Avoid 1400-1600µs range, 70% damping
NF+1800,50,0.5      // Avoid 1750-1850µs range, 50% damping
NF?                 // Verify configuration
```

**Safety:** Filter is OFF by default and can be disabled anytime with `NFEN=0` to bypass all filtering if issues occur.

## Build & Upload

```bash
# Build
pio run

# Upload
pio run --target upload

# Monitor
pio device monitor
```

## Configuration

See `src/Config.h` for pin assignments, timing constants, and hardware configuration.

## Dependencies

- PlatformIO
- Arduino framework for ESP32
- Adafruit NeoPixel library (^1.12.0)

## License

Private project - All rights reserved
