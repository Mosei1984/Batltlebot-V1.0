#pragma once

#include <Arduino.h>

void BluetoothComm_init();
bool BluetoothComm_poll(String &outLine, unsigned long nowMs);
