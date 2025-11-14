#pragma once

#include <Arduino.h>

// LED display modes
enum class LedMode {
    AUTO,    // Automatically reflect bot/weapon state
    OFF,     // All LEDs off
    SOLID,   // Static color, all LEDs same
    BLINK,   // On/off blinking
    PULSE,   // Breathing effect (triangle wave)
    WIPE     // Single pixel moving along strip
};

// Initialize WS2812B LED strip
void Leds_init();

// Non-blocking LED update (call every main loop iteration)
void Leds_update(unsigned long nowMs);

// Handle LED command from Bluetooth/Serial
// Returns true if command was valid, false otherwise
bool Leds_handleCommand(const String& line);
