#include "DebugIO.h"
#include "Config.h"

void DebugIO_init() {
    pinMode(PIN_DBG_INPUT,  OUTPUT);
    pinMode(PIN_DBG_LEFT,   OUTPUT);
    pinMode(PIN_DBG_RIGHT,  OUTPUT);
    pinMode(PIN_DBG_WEAPON, OUTPUT);

    digitalWrite(PIN_DBG_INPUT,  LOW);
    digitalWrite(PIN_DBG_LEFT,   LOW);
    digitalWrite(PIN_DBG_RIGHT,  LOW);
    digitalWrite(PIN_DBG_WEAPON, LOW);
}

void DebugIO_pulseInput() {
    digitalWrite(PIN_DBG_INPUT, HIGH);
    digitalWrite(PIN_DBG_INPUT, LOW);
}

void DebugIO_setLeftForward(bool forward) {
    digitalWrite(PIN_DBG_LEFT, forward ? HIGH : LOW);
}

void DebugIO_setRightForward(bool forward) {
    digitalWrite(PIN_DBG_RIGHT, forward ? HIGH : LOW);
}

void DebugIO_setWeaponActive(bool active) {
    digitalWrite(PIN_DBG_WEAPON, active ? HIGH : LOW);
}
