#include "Failsafe.h"
#include "Config.h"
#include "Drive.h"
#include "Weapon.h"
#include "Diagnostics.h"

static unsigned long g_lastMotionMs  = 0;
static unsigned long g_lastAnyCmdMs  = 0;
static bool g_motionTimeoutActive    = false;
static bool g_linkTimeoutActive      = false;

void Failsafe_init() {
    unsigned long now = millis();
    g_lastMotionMs     = now;
    g_lastAnyCmdMs     = now;
    g_motionTimeoutActive = false;
    g_linkTimeoutActive   = false;
}

void Failsafe_onMotionCommand(unsigned long nowMs) {
    g_lastMotionMs = nowMs;
    g_motionTimeoutActive = false; // Reset, sobald wieder Kommando kommt
}

void Failsafe_onAnyCommand(unsigned long nowMs) {
    g_lastAnyCmdMs = nowMs;
    g_linkTimeoutActive = false;
}

void Failsafe_update(unsigned long nowMs) {
    // Nur Link-Failsafe aktiv: Waffe in Idle, wenn lange kein Kommando (motion ODER function)
    if (!g_linkTimeoutActive &&
        (nowMs - g_lastAnyCmdMs) > FAILSAFE_LINK_TIMEOUT_MS) {

        if (Weapon_getState() == WeaponState::ARMED) {
            Weapon_idle();
        }

        Diag_incLinkTimeout();
        g_linkTimeoutActive = true;

        Serial.println(F("[FS] Link timeout -> weapon idle"));
    }
}
