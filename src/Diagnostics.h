#pragma once
#include <Arduino.h>

// einfache Fehlerzähler-Struktur
struct DiagnosticsCounters {
    uint32_t invalidMotionFormat   = 0;
    uint32_t invalidFunctionFormat = 0;
    uint32_t invalidMoveDir        = 0;
    uint32_t invalidSteerDir       = 0;
    uint32_t btBufferOverflow      = 0;
    uint32_t motionTimeouts        = 0;
    uint32_t linkTimeouts          = 0;
    uint32_t weaponArmingTimeout   = 0;
    uint32_t invalidLedCommand     = 0;
    uint32_t ledShowOverrun        = 0;
};

void Diag_init();
void Diag_incInvalidMotionFormat();
void Diag_incInvalidFunctionFormat();
void Diag_incInvalidMoveDir();
void Diag_incInvalidSteerDir();
void Diag_incBtBufferOverflow();
void Diag_incMotionTimeout();
void Diag_incLinkTimeout();
void Diag_incWeaponArmingTimeout();
void Diag_incInvalidLedCommand();
void Diag_incLedShowOverrun();

// optional: Einmal pro Sekunde eine kompakte Übersicht ausgeben
void Diag_update(unsigned long nowMs);
