#include "Weapon.h"
#include "Config.h"
#include "DebugIO.h"
#include "Diagnostics.h"
#include "NotchFilter.h"
#include <Arduino.h>

static WeaponState weaponState       = WeaponState::DISARMED;
static unsigned long weaponArmStartMs = 0;

static int currentWeaponUs = ESC_OFF_US;
static int targetWeaponUs  = ESC_OFF_US;

static float weaponRateUpUsPerMs   = 0.0f;
static float weaponRateDownUsPerMs = 0.0f;

static unsigned long lastWeaponDebugMs = 0;

static uint32_t usToDuty(int us) {
    uint32_t maxDuty   = (1U << WEAPON_PWM_RES) - 1U;
    float periodUs     = 1000000.0f / float(WEAPON_PWM_FREQ); // ~20000 us
    float dutyFraction = float(us) / periodUs;
    if (dutyFraction < 0.0f) dutyFraction = 0.0f;
    if (dutyFraction > 1.0f) dutyFraction = 1.0f;
    return uint32_t(dutyFraction * maxDuty);
}

WeaponState Weapon_getState() {
    return weaponState;
}

int Weapon_getTargetThrottleUs() {
    return targetWeaponUs;
}

void Weapon_init() {
    pinMode(PIN_LED_ARM, OUTPUT);
    digitalWrite(PIN_LED_ARM, LOW);

    DebugIO_setWeaponActive(false);

    ledcSetup(WEAPON_CHANNEL, WEAPON_PWM_FREQ, WEAPON_PWM_RES);
    ledcAttachPin(PIN_WEAPON, WEAPON_CHANNEL);

    weaponRateUpUsPerMs   = float(ESC_MAX_US - ESC_OFF_US) / float(WEAPON_RAMP_UP_TIME_MS);
    weaponRateDownUsPerMs = float(ESC_MAX_US - ESC_OFF_US) / float(WEAPON_RAMP_DOWN_TIME_MS);

    currentWeaponUs = ESC_OFF_US;
    targetWeaponUs  = ESC_OFF_US;
    ledcWrite(WEAPON_CHANNEL, usToDuty(currentWeaponUs));

    weaponState        = WeaponState::DISARMED;
    weaponArmStartMs   = 0;
    lastWeaponDebugMs  = millis();

    NotchFilter_init(ESC_ARM_US);
}

void Weapon_armRequest() {
    if (weaponState == WeaponState::DISARMED) {
        weaponState      = WeaponState::ARMING;
        weaponArmStartMs = millis();
        targetWeaponUs   = ESC_ARM_US;
        digitalWrite(PIN_LED_ARM, HIGH);
        Serial.println(F("[DBG] Weapon: ARMING requested"));
    }
}

void Weapon_disarm() {
    weaponState    = WeaponState::DISARMED;
    targetWeaponUs = ESC_OFF_US;
    digitalWrite(PIN_LED_ARM, LOW);
    DebugIO_setWeaponActive(false);
    Serial.println(F("[DBG] Weapon: DISARMED"));
}

void Weapon_fullThrottle() {
    if (weaponState == WeaponState::ARMED) {
        targetWeaponUs = ESC_MAX_US;
        Serial.println(F("[DBG] Weapon: FULL THROTTLE"));
    } else {
        Serial.println(F("[DBG] Weapon_fullThrottle ignored (not ARMED)"));
    }
}

void Weapon_idle() {
    if (weaponState == WeaponState::ARMED) {
        targetWeaponUs = ESC_ARM_US;
        Serial.println(F("[DBG] Weapon: IDLE"));
    } else {
        Serial.println(F("[DBG] Weapon_idle ignored (not ARMED)"));
    }
}

void Weapon_updateArming(unsigned long nowMs) {
    switch (weaponState) {
        case WeaponState::DISARMED:
            // nichts
            break;

        case WeaponState::ARMING: {
            unsigned long elapsed = nowMs - weaponArmStartMs;

            // Normaler Übergang nach WEAPON_ARM_PULSE_TIME_MS
            if (elapsed >= WEAPON_ARM_PULSE_TIME_MS) {
                // Plausibilitätscheck: sind wir in der Nähe von ESC_ARM_US?
                if (abs(currentWeaponUs - ESC_ARM_US) <= 20) {
                    weaponState = WeaponState::ARMED;
                    targetWeaponUs = ESC_ARM_US;
                    Serial.println(F("[DBG] Weapon: ARMED"));
                } else {
                    // Arming fehlgeschlagen -> Failsafe: disarm + Fehlerzähler
                    Weapon_disarm();
                    Diag_incWeaponArmingTimeout();
                    Serial.println(F("[ERR] Weapon: ARming failed (PWM not at ARM level)"));
                }
            }
            // Sicherheitsnetz: wenn aus irgendeinem Grund extrem lange im ARMING
            else if (elapsed > (WEAPON_ARM_PULSE_TIME_MS * 3UL)) {
                Weapon_disarm();
                Diag_incWeaponArmingTimeout();
                Serial.println(F("[ERR] Weapon: ARming timeout -> DISARM"));
            }
            break;
        }

        case WeaponState::ARMED:
            // keine Änderung
            break;
    }
}

void Weapon_update(unsigned long dtMs, unsigned long nowMs) {
    if (dtMs == 0) return;

    int before = currentWeaponUs;

    if (currentWeaponUs < targetWeaponUs) {
        currentWeaponUs += int(weaponRateUpUsPerMs * float(dtMs));
        if (currentWeaponUs > targetWeaponUs) currentWeaponUs = targetWeaponUs;
    } else if (currentWeaponUs > targetWeaponUs) {
        currentWeaponUs -= int(weaponRateDownUsPerMs * float(dtMs));
        if (currentWeaponUs < targetWeaponUs) currentWeaponUs = targetWeaponUs;
    }

    // Bounds-Sicherheit
    if (currentWeaponUs < ESC_OFF_US) currentWeaponUs = ESC_OFF_US;
    if (currentWeaponUs > ESC_MAX_US) currentWeaponUs = ESC_MAX_US;

    // Notch Filter anwenden (nur wenn ARMED und über Idle)
    bool filterActive = (weaponState == WeaponState::ARMED && currentWeaponUs > ESC_ARM_US + 5);
    int outputUs = NotchFilter_apply(currentWeaponUs, filterActive);

    if (currentWeaponUs != before) {
        ledcWrite(WEAPON_CHANNEL, usToDuty(outputUs));

        if ((nowMs - lastWeaponDebugMs) >= 100UL) {
            lastWeaponDebugMs = nowMs;
            Serial.print(F("[DBG] Weapon us="));
            Serial.print(currentWeaponUs);
            Serial.print(F(" target="));
            Serial.print(targetWeaponUs);
            Serial.print(F(" state="));
            switch (weaponState) {
                case WeaponState::DISARMED: Serial.println(F("DISARMED")); break;
                case WeaponState::ARMING:   Serial.println(F("ARMING"));   break;
                case WeaponState::ARMED:    Serial.println(F("ARMED"));    break;
            }
        }
    }

    // Debug Pin: aktiv, wenn Waffe ARMED und Target > Idle
    if (weaponState == WeaponState::ARMED && targetWeaponUs > ESC_ARM_US + 10) {
        DebugIO_setWeaponActive(true);
    } else {
        DebugIO_setWeaponActive(false);
    }
}
