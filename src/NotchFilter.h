#pragma once

#include <Arduino.h>

constexpr int MAX_NOTCHES = 8;

void NotchFilter_init(int baseUs);
void NotchFilter_setEnabled(bool enabled);
bool NotchFilter_isEnabled();

bool NotchFilter_add(int centerUs, int halfWidthUs, float depth, uint32_t* outId = nullptr);
bool NotchFilter_removeById(uint32_t id);
void NotchFilter_clear();
int  NotchFilter_getCount();

int  NotchFilter_apply(int inputUs, bool active);

void NotchFilter_dump(Stream& s);
