#pragma once

#include <Arduino.h>
#include "State.h"

void Drive_init();
void Drive_setTargets(int left, int right);  // Werte: -255..+255
void Drive_update();
BotState Drive_getState();
