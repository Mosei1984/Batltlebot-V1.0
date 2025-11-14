#pragma once

#include <Arduino.h>

// --- Pins ---
constexpr int PIN_IN1        = 25;   // Motor links IN1
constexpr int PIN_IN2        = 26;   // Motor links IN2
constexpr int PIN_IN3        = 27;   // Motor rechts IN3
constexpr int PIN_IN4        = 14;   // Motor rechts IN4

constexpr int PIN_LED_ARM    = 15;   // LED‐Indikator für Waffe / Arming

constexpr int PIN_DBG_INPUT  = 16;   // Debug: Bluetooth Kommando empfangen
constexpr int PIN_DBG_LEFT   = 17;   // Debug: linker Motor Vorwärts
constexpr int PIN_DBG_RIGHT  = 18;   // Debug: rechter Motor Vorwärts
constexpr int PIN_DBG_WEAPON = 19;   // Debug: Waffenmotor aktiv

constexpr int PIN_WEAPON     = 4;    // ESC‐Signal

// --- WS2812B LED Strip Config ---
constexpr int PIN_LED_DATA      = 23;   // WS2812B data pin (safe GPIO, avoid boot pins)
constexpr int LED_COUNT         = 10;   // Number of LEDs in strip/ring
constexpr uint8_t LED_BRIGHTNESS = 64;  // Default brightness 0..255
constexpr uint16_t LED_TICK_MS  = 20;   // LED effect update interval (50 Hz)
constexpr uint16_t LED_SHOW_BUDGET_US = 5000; // Max allowed show() time in microseconds

static_assert(LED_COUNT > 0, "LED_COUNT must be > 0");

// --- LED Zone Layout (for AUTO mode) ---
// Symmetric layout: [Drive Left] [Weapon Center] [Drive Right]
constexpr int LED_DRIVE_LEFT_START  = 0;    // Left drive zone start
constexpr int LED_DRIVE_LEFT_COUNT  = 3;    // Left drive zone: 7 pixels (0-6)
constexpr int LED_WEAPON_START      = 3;    // Weapon zone start (center)
constexpr int LED_WEAPON_COUNT      = 2;    // Weapon zone: 2 pixels (7-8)
constexpr int LED_DRIVE_RIGHT_START = 5;    // Right drive zone start
constexpr int LED_DRIVE_RIGHT_COUNT = 9;    // Right drive zone: 7 pixels (9-15)

// --- Motor PWM Config ---
constexpr int MOTOR_PWM_FREQ = 20000; // 20 kHz
constexpr int MOTOR_PWM_RES  = 8;     // 8 Bit

constexpr int MAX_PWM        = 255;   // Max PWM für Motoren

// --- Weapon ESC PWM Config ---
constexpr int WEAPON_PWM_FREQ = 50;   // 50 Hz
constexpr int WEAPON_PWM_RES  = 16;   // 16 Bit
constexpr int WEAPON_CHANNEL  = 6;    // LEDC Channel für ESC (Timer 3)

// --- ESC Pulszeiten in Mikrosekunden ---
constexpr int ESC_OFF_US =  988;  // Disarmed
constexpr int ESC_ARM_US = 1001;  // Armed/Idle
constexpr int ESC_MAX_US = 2000;  // Vollgas

// --- Rampzeiten für Waffenmotor ---
constexpr unsigned long WEAPON_RAMP_UP_TIME_MS   = 1000UL;
constexpr unsigned long WEAPON_RAMP_DOWN_TIME_MS = 800UL;

// --- Arming Zeitdauer ---
constexpr unsigned long WEAPON_ARM_PULSE_TIME_MS = 1000UL;

// --- Loop Timing ---
constexpr unsigned long LOOP_INTERVAL_MS = 10UL;  // 100 Hz Steuerloop

// --- Failsafe Settings ---
// Wenn länger als diese Zeit kein Fahrkommando kam -> Motoren stoppen
constexpr unsigned long FAILSAFE_MOTION_TIMEOUT_MS = 500UL;   // 0.5 s
// Wenn länger kein Kommando (Motion oder Function) -> Waffe in Idle
constexpr unsigned long FAILSAFE_LINK_TIMEOUT_MS   = 60000UL;  // 60 s (1 minute)
