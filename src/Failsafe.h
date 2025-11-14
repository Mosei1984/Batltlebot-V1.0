#pragma once
#include <Arduino.h>

// Initialisierung der Failsafe-Logik
void Failsafe_init();

// sollte bei jedem gültigen Motion-Command aufgerufen werden
void Failsafe_onMotionCommand(unsigned long nowMs);

// bei jedem (Motion oder Function) Kommando aufrufen
void Failsafe_onAnyCommand(unsigned long nowMs);

// im festen Loop-Takt aufrufen
void Failsafe_update(unsigned long nowMs);
