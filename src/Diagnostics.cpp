#include "Diagnostics.h"
#include <string.h>

static DiagnosticsCounters g_diag;
static unsigned long g_lastPrintMs = 0;
static DiagnosticsCounters g_lastPrinted; // zum Erkennen von Änderungen

void Diag_init() {
    g_diag = DiagnosticsCounters{};
    g_lastPrinted = g_diag;
    g_lastPrintMs = 0;
}

#define DIAG_INC(field) do { g_diag.field++; } while(0)

void Diag_incInvalidMotionFormat()   { DIAG_INC(invalidMotionFormat); }
void Diag_incInvalidFunctionFormat() { DIAG_INC(invalidFunctionFormat); }
void Diag_incInvalidMoveDir()        { DIAG_INC(invalidMoveDir); }
void Diag_incInvalidSteerDir()       { DIAG_INC(invalidSteerDir); }
void Diag_incBtBufferOverflow()      { DIAG_INC(btBufferOverflow); }
void Diag_incMotionTimeout()         { DIAG_INC(motionTimeouts); }
void Diag_incLinkTimeout()           { DIAG_INC(linkTimeouts); }
void Diag_incWeaponArmingTimeout()   { DIAG_INC(weaponArmingTimeout); }
void Diag_incInvalidLedCommand()     { DIAG_INC(invalidLedCommand); }
void Diag_incLedShowOverrun()        { DIAG_INC(ledShowOverrun); }

static bool countersChanged() {
    return memcmp(&g_diag, &g_lastPrinted, sizeof(DiagnosticsCounters)) != 0;
}

void Diag_update(unsigned long nowMs) {
    // Nur ca. 1x pro Sekunde prüfen/loggen
    if (nowMs - g_lastPrintMs < 1000UL) return;
    g_lastPrintMs = nowMs;

    if (!countersChanged()) return;

    Serial.println(F("[DIAG] Counters:"));
    Serial.print(F("  invalidMotionFormat   = ")); Serial.println(g_diag.invalidMotionFormat);
    Serial.print(F("  invalidFunctionFormat = ")); Serial.println(g_diag.invalidFunctionFormat);
    Serial.print(F("  invalidMoveDir        = ")); Serial.println(g_diag.invalidMoveDir);
    Serial.print(F("  invalidSteerDir       = ")); Serial.println(g_diag.invalidSteerDir);
    Serial.print(F("  btBufferOverflow      = ")); Serial.println(g_diag.btBufferOverflow);
    Serial.print(F("  motionTimeouts        = ")); Serial.println(g_diag.motionTimeouts);
    Serial.print(F("  linkTimeouts          = ")); Serial.println(g_diag.linkTimeouts);
    Serial.print(F("  weaponArmingTimeout   = ")); Serial.println(g_diag.weaponArmingTimeout);
    Serial.print(F("  invalidLedCommand     = ")); Serial.println(g_diag.invalidLedCommand);
    Serial.print(F("  ledShowOverrun        = ")); Serial.println(g_diag.ledShowOverrun);

    g_lastPrinted = g_diag;
}
