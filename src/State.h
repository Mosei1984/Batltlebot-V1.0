#pragma once

// Zustand des Fahrbots
enum class BotState {
    IDLE,
    DRIVE
};

// Zustand des Waffenmotors
enum class WeaponState {
    DISARMED,
    ARMING,
    ARMED
};
