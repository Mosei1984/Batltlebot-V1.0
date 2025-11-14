#pragma once

#include <Arduino.h>
#include "State.h"

void Weapon_init();
void Weapon_armRequest();
void Weapon_disarm();
void Weapon_fullThrottle();
void Weapon_idle();

void Weapon_updateArming(unsigned long nowMs);
void Weapon_update(unsigned long dtMs, unsigned long nowMs);

WeaponState Weapon_getState();
int Weapon_getTargetThrottleUs();  // Get current target throttle (for LED status)
