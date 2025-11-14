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
- **Weapon**: Arming sequence and weapon motor control
- **Leds**: Non-blocking WS2812B LED effects with state-based visualization
- **BluetoothComm**: Bluetooth serial communication handler
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

Commands via Bluetooth app:
- **M**: Motion control (left/right motor speed)
- **F**: Function commands (weapon, LED modes)
- **Weapon Arming**: Two-step safety sequence

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
