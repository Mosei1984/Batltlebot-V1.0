#pragma once

#include <Arduino.h>

void DebugIO_init();
void DebugIO_pulseInput();
void DebugIO_setLeftForward(bool forward);
void DebugIO_setRightForward(bool forward);
void DebugIO_setWeaponActive(bool active);
