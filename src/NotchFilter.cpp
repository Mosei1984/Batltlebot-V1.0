#include "NotchFilter.h"
#include "Config.h"

struct Notch {
    uint32_t id;
    int centerUs;
    int halfWidthUs;
    float depth;
    bool enabled;
};

static Notch notches[MAX_NOTCHES];
static int notchCount = 0;
static uint32_t nextId = 1;
static int baseThrottleUs = ESC_ARM_US;
static bool globalEnabled = false;

void NotchFilter_init(int baseUs) {
    baseThrottleUs = baseUs;
    notchCount = 0;
    nextId = 1;
    globalEnabled = false;
    for (int i = 0; i < MAX_NOTCHES; i++) {
        notches[i].enabled = false;
    }
}

void NotchFilter_setEnabled(bool enabled) {
    globalEnabled = enabled;
}

bool NotchFilter_isEnabled() {
    return globalEnabled;
}

bool NotchFilter_add(int centerUs, int halfWidthUs, float depth, uint32_t* outId) {
    if (notchCount >= MAX_NOTCHES) return false;
    if (halfWidthUs <= 0) return false;
    if (depth < 0.0f || depth > 1.0f) return false;

    notches[notchCount].id = nextId;
    notches[notchCount].centerUs = centerUs;
    notches[notchCount].halfWidthUs = halfWidthUs;
    notches[notchCount].depth = depth;
    notches[notchCount].enabled = true;

    if (outId) *outId = nextId;
    nextId++;
    notchCount++;
    return true;
}

bool NotchFilter_removeById(uint32_t id) {
    for (int i = 0; i < notchCount; i++) {
        if (notches[i].id == id) {
            for (int j = i; j < notchCount - 1; j++) {
                notches[j] = notches[j + 1];
            }
            notchCount--;
            return true;
        }
    }
    return false;
}

void NotchFilter_clear() {
    notchCount = 0;
}

int NotchFilter_getCount() {
    return notchCount;
}

int NotchFilter_apply(int inputUs, bool active) {
    if (!globalEnabled || !active || notchCount == 0) {
        return inputUs;
    }

    if (inputUs <= baseThrottleUs + 5) {
        return inputUs;
    }

    float delta = float(inputUs - baseThrottleUs);
    float factor = 1.0f;

    for (int i = 0; i < notchCount; i++) {
        if (!notches[i].enabled) continue;

        int d = abs(inputUs - notches[i].centerUs);
        if (d < notches[i].halfWidthUs) {
            float w = 1.0f - float(d) / float(notches[i].halfWidthUs);
            float f = 1.0f - notches[i].depth * w;
            factor *= f;
        }
    }

    factor = constrain(factor, 0.0f, 1.0f);
    int outUs = baseThrottleUs + int(delta * factor + 0.5f);
    return constrain(outUs, ESC_OFF_US, ESC_MAX_US);
}

void NotchFilter_dump(Stream& s) {
    s.print(F("[NF] Global: "));
    s.println(globalEnabled ? F("ENABLED") : F("DISABLED"));
    s.print(F("[NF] Active notches: "));
    s.println(notchCount);
    
    for (int i = 0; i < notchCount; i++) {
        s.print(F("  ["));
        s.print(notches[i].id);
        s.print(F("] center="));
        s.print(notches[i].centerUs);
        s.print(F("us, halfWidth="));
        s.print(notches[i].halfWidthUs);
        s.print(F("us, depth="));
        s.print(notches[i].depth, 2);
        s.print(F(", "));
        s.println(notches[i].enabled ? F("ON") : F("OFF"));
    }
}
