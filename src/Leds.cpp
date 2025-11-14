#include "Leds.h"
#include "Config.h"
#include "State.h"
#include "Drive.h"
#include "Weapon.h"
#include "Diagnostics.h"
#include <Adafruit_NeoPixel.h>

// Internal state structure
static struct
{
    Adafruit_NeoPixel strip;

    LedMode currentMode;
    bool overrideActive; // true if user command overrides AUTO mode

    uint32_t color;    // Current color (24-bit RGB)
    uint16_t periodMs; // Effect period for BLINK/PULSE/WIPE
    uint8_t dutyCycle; // Duty cycle for BLINK (0..100 %)

    uint32_t phaseStartMs; // When current phase/cycle began
    uint8_t wipePosition;  // Current position for WIPE effect

    BotState lastBotState; // Cached state for AUTO mode
    WeaponState lastWeaponState;

    bool dirty;               // true if pixels changed, need show()
    unsigned long lastTickMs; // Last LED update tick

} s_led = {
    Adafruit_NeoPixel(LED_COUNT, PIN_LED_DATA, NEO_GRB + NEO_KHZ800),
    LedMode::AUTO,
    false,
    0x202020, // Default dim white
    500,
    50,
    0,
    0,
    BotState::IDLE,
    WeaponState::DISARMED,
    false,
    0};

// Helper: parse 2-digit hex string to uint8_t
static bool parseHex2(const String &s, int offset, uint8_t &out)
{
    if (offset + 2 > (int)s.length())
        return false;
    char buf[3] = {s[offset], s[offset + 1], 0};
    char *end;
    long val = strtol(buf, &end, 16);
    if (end != buf + 2)
        return false;
    out = (uint8_t)val;
    return true;
}

// Helper: set all pixels to same color
static void setAllPixels(uint32_t color)
{
    for (int i = 0; i < LED_COUNT; i++)
    {
        s_led.strip.setPixelColor(i, color);
    }
    s_led.dirty = true;
}

// Helper: clear all pixels
static void clearAllPixels()
{
    s_led.strip.clear();
    s_led.dirty = true;
}

// Helper: set one pixel
static void setOnePixel(uint16_t idx, uint32_t color)
{
    if (idx < LED_COUNT)
    {
        s_led.strip.setPixelColor(idx, color);
        s_led.dirty = true;
    }
}

// Helper: set pixel range to same color
static void setRange(uint16_t start, uint16_t count, uint32_t color)
{
    for (uint16_t i = 0; i < count; i++)
    {
        if (start + i < LED_COUNT)
        {
            s_led.strip.setPixelColor(start + i, color);
        }
    }
    s_led.dirty = true;
}

// Helper: clear pixel range
static void clearRange(uint16_t start, uint16_t count)
{
    for (uint16_t i = 0; i < count; i++)
    {
        if (start + i < LED_COUNT)
        {
            s_led.strip.setPixelColor(start + i, 0);
        }
    }
    s_led.dirty = true;
}

// Helper: apply SOLID effect
static void applySolid()
{
    setAllPixels(s_led.color);
}

// Helper: apply BLINK effect (non-blocking, time-based)
static void applyBlink(unsigned long nowMs)
{
    if (s_led.periodMs == 0)
    {
        clearAllPixels();
        return;
    }

    unsigned long elapsed = nowMs - s_led.phaseStartMs;
    unsigned long cycle = elapsed % s_led.periodMs;
    uint32_t onTime = (uint32_t(s_led.periodMs) * s_led.dutyCycle) / 100;

    if (cycle < onTime)
    {
        setAllPixels(s_led.color);
    }
    else
    {
        clearAllPixels();
    }
}

// Helper: apply PULSE effect (breathing, triangle wave)
static void applyPulse(unsigned long nowMs)
{
    if (s_led.periodMs == 0)
    {
        clearAllPixels();
        return;
    }

    unsigned long elapsed = nowMs - s_led.phaseStartMs;
    unsigned long cycle = elapsed % s_led.periodMs;

    const uint32_t P = s_led.periodMs;
    const uint32_t half = P / 2;

    // Triangle wave: 0 -> peak -> 0
    uint8_t brightness;
    if (cycle < half)
    {
        // Rising
        brightness = uint8_t((uint32_t(cycle) * 510U) / P);
    }
    else
    {
        // Falling
        brightness = uint8_t(255U - (uint32_t(cycle - half) * 510U) / P);
    }

    // Extract RGB components
    uint8_t r = (s_led.color >> 16) & 0xFF;
    uint8_t g = (s_led.color >> 8) & 0xFF;
    uint8_t b = s_led.color & 0xFF;

    // Scale by brightness
    r = (r * brightness) / 255;
    g = (g * brightness) / 255;
    b = (b * brightness) / 255;

    uint32_t scaledColor = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    setAllPixels(scaledColor);
}

// Helper: apply WIPE effect (single moving pixel)
static void applyWipe(unsigned long nowMs)
{
    if (s_led.periodMs == 0 || LED_COUNT == 0)
    {
        clearAllPixels();
        return;
    }

    unsigned long elapsed = nowMs - s_led.phaseStartMs;

    // periodMs is used as step interval (ms per pixel move)
    uint8_t newPos = (elapsed / s_led.periodMs) % LED_COUNT;

    if (newPos != s_led.wipePosition)
    {
        clearAllPixels();
        s_led.strip.setPixelColor(newPos, s_led.color);
        s_led.wipePosition = newPos;
        s_led.dirty = true;
    }
}

// Apply current mode's effect
static void applyEffect(unsigned long nowMs)
{
    switch (s_led.currentMode)
    {
    case LedMode::OFF:
        clearAllPixels();
        break;

    case LedMode::SOLID:
        applySolid();
        break;

    case LedMode::BLINK:
        applyBlink(nowMs);
        break;

    case LedMode::PULSE:
        applyPulse(nowMs);
        break;

    case LedMode::WIPE:
        applyWipe(nowMs);
        break;

    case LedMode::AUTO:
        // Handled separately in Leds_update
        break;
    }
}

// Composite rendering for AUTO mode: 3 zones (drive left, weapon center, drive right)
static void renderAutoComposite(unsigned long nowMs)
{
    // Weapon colors based on state + throttle
    constexpr uint32_t C_GREEN = 0x00FF00;  // DISARMED (off)
    constexpr uint32_t C_YELLOW = 0xFFFF00; // ARMING (spin up)
    constexpr uint32_t C_RED = 0xFF0000;    // ARMED + full throttle (spinning)
    constexpr uint32_t C_ORANGE = 0xFF8000; // ARMED + idle (spin down)

    // Drive colors
    constexpr uint32_t C_BLUE = 0x0000FF;  // DRIVE
    constexpr uint32_t C_WHITE = 0x202020; // IDLE

    BotState bot = Drive_getState();
    WeaponState wep = Weapon_getState();
    int weaponThrottle = Weapon_getTargetThrottleUs();

    static BotState lastBot = BotState::IDLE;
    static WeaponState lastWep = WeaponState::DISARMED;
    static int lastThrottle = ESC_OFF_US;
    static unsigned long drvPhaseStart = 0;
    static int lastWipePosLeft = -1;
    static int lastWipePosRight = -1;

    bool botChanged = (bot != lastBot);
    bool wepChanged = (wep != lastWep) || (weaponThrottle != lastThrottle);

    if (botChanged)
    {
        drvPhaseStart = nowMs;
        lastWipePosLeft = -1;
        lastWipePosRight = -1;
    }

    // --- Weapon zone (center, 2 pixels) ---
    if (LED_WEAPON_COUNT > 0)
    {
        uint32_t weaponColor = C_GREEN; // Default: DISARMED

        if (wep == WeaponState::ARMING)
        {
            weaponColor = C_YELLOW; // Spin up
        }
        else if (wep == WeaponState::ARMED)
        {
            // Check if full throttle or idle
            const int throttleThreshold = (ESC_ARM_US + ESC_MAX_US) / 2;
            if (weaponThrottle > throttleThreshold)
            {
                weaponColor = C_RED; // Full spin
            }
            else
            {
                weaponColor = C_ORANGE; // Idle/spin down
            }
        }

        if (wepChanged)
        {
            setRange(LED_WEAPON_START, LED_WEAPON_COUNT, weaponColor);
        }
    }

    // --- Left drive zone (0-6) ---
    if (LED_DRIVE_LEFT_COUNT > 0)
    {
        if (bot == BotState::DRIVE)
        {
            const uint32_t stepMs = 60;
            int pos = int(((nowMs - drvPhaseStart) / stepMs) % LED_DRIVE_LEFT_COUNT);
            if (pos != lastWipePosLeft || botChanged)
            {
                clearRange(LED_DRIVE_LEFT_START, LED_DRIVE_LEFT_COUNT);
                setOnePixel(LED_DRIVE_LEFT_START + pos, C_BLUE);
                lastWipePosLeft = pos;
            }
        }
        else
        { // IDLE
            if (botChanged)
            {
                setRange(LED_DRIVE_LEFT_START, LED_DRIVE_LEFT_COUNT, C_WHITE);
            }
        }
    }

    // --- Right drive zone (9-15) ---
    if (LED_DRIVE_RIGHT_COUNT > 0)
    {
        if (bot == BotState::DRIVE)
        {
            const uint32_t stepMs = 60;
            int pos = int(((nowMs - drvPhaseStart) / stepMs) % LED_DRIVE_RIGHT_COUNT);
            if (pos != lastWipePosRight || botChanged)
            {
                clearRange(LED_DRIVE_RIGHT_START, LED_DRIVE_RIGHT_COUNT);
                setOnePixel(LED_DRIVE_RIGHT_START + pos, C_BLUE);
                lastWipePosRight = pos;
            }
        }
        else
        { // IDLE
            if (botChanged)
            {
                setRange(LED_DRIVE_RIGHT_START, LED_DRIVE_RIGHT_COUNT, C_WHITE);
            }
        }
    }

    lastBot = bot;
    lastWep = wep;
    lastThrottle = weaponThrottle;
}

// Update AUTO mode based on bot/weapon state
static void updateAutoMode(unsigned long nowMs)
{
    renderAutoComposite(nowMs);
}

void Leds_init()
{
    s_led.strip.begin();
    s_led.strip.setBrightness(LED_BRIGHTNESS);
    s_led.strip.clear();
    s_led.strip.show();

    s_led.currentMode = LedMode::AUTO;
    s_led.overrideActive = false;
    s_led.color = 0x202020;
    s_led.periodMs = 500;
    s_led.dutyCycle = 50;
    s_led.phaseStartMs = 0;
    s_led.wipePosition = 0;
    s_led.lastBotState = BotState::IDLE;
    s_led.lastWeaponState = WeaponState::DISARMED;
    s_led.dirty = false;
    s_led.lastTickMs = 0;
}

void Leds_update(unsigned long nowMs)
{
    // Throttle updates to LED_TICK_MS interval (non-blocking)
    if (nowMs - s_led.lastTickMs < LED_TICK_MS)
    {
        return;
    }
    s_led.lastTickMs = nowMs;

    s_led.dirty = false; // Reset dirty flag

    // Handle AUTO mode vs override modes
    if (!s_led.overrideActive)
    {
        updateAutoMode(nowMs);
    }
    else
    {
        applyEffect(nowMs);
    }

    // Only call show() if pixels changed
    if (s_led.dirty)
    {
        unsigned long startUs = micros();
        s_led.strip.show();
        unsigned long durationUs = micros() - startUs;

        // Check if show() took too long
        if (durationUs > LED_SHOW_BUDGET_US)
        {
            Diag_incLedShowOverrun();
        }
    }
}

bool Leds_handleCommand(const String &line)
{
    // Expected format:
    // L0              -> OFF
    // L1RRGGBB        -> SOLID (hex RGB)
    // L2RRGGBBPP      -> BLINK (PP = period 01..99 -> 100..9900ms)
    // L3RRGGBBPP      -> PULSE (PP = period 01..99 -> 100..9900ms)
    // L4RRGGBBPP      -> WIPE (PP = stepMs/10)
    // LA              -> AUTO

    if (line.length() < 2 || line[0] != 'L')
    {
        Diag_incInvalidLedCommand();
        return false;
    }

    char cmd = line[1];

    // Handle AUTO mode
    if (cmd == 'A')
    {
        if (line.length() != 2)
        {
            Diag_incInvalidLedCommand();
            return false;
        }
        s_led.currentMode = LedMode::AUTO;
        s_led.overrideActive = false;
        s_led.phaseStartMs = millis();
        s_led.wipePosition = 0;
        return true;
    }

    // Handle OFF mode
    if (cmd == '0')
    {
        if (line.length() != 2)
        {
            Diag_incInvalidLedCommand();
            return false;
        }
        s_led.currentMode = LedMode::OFF;
        s_led.overrideActive = true;
        s_led.phaseStartMs = millis();
        clearAllPixels();

        // Immediate show with budget measurement
        unsigned long startUs = micros();
        s_led.strip.show();
        unsigned long durationUs = micros() - startUs;
        if (durationUs > LED_SHOW_BUDGET_US)
        {
            Diag_incLedShowOverrun();
        }

        return true;
    }

    // Handle SOLID mode (L1RRGGBB)
    if (cmd == '1')
    {
        if (line.length() != 8)
        {
            Diag_incInvalidLedCommand();
            return false;
        }

        uint8_t r, g, b;
        if (!parseHex2(line, 2, r) || !parseHex2(line, 4, g) || !parseHex2(line, 6, b))
        {
            Diag_incInvalidLedCommand();
            return false;
        }

        s_led.currentMode = LedMode::SOLID;
        s_led.overrideActive = true;
        s_led.color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        s_led.phaseStartMs = millis();
        return true;
    }

    // Handle BLINK/PULSE/WIPE modes (L2RRGGBBPP, L3RRGGBBPP, L4RRGGBBPP)
    if (cmd >= '2' && cmd <= '4')
    {
        if (line.length() != 10)
        {
            Diag_incInvalidLedCommand();
            return false;
        }

        uint8_t r, g, b, period;
        if (!parseHex2(line, 2, r) || !parseHex2(line, 4, g) ||
            !parseHex2(line, 6, b) || !parseHex2(line, 8, period))
        {
            Diag_incInvalidLedCommand();
            return false;
        }

        if (period < 1 || period > 99)
        {
            Diag_incInvalidLedCommand();
            return false;
        }

        s_led.color = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
        s_led.overrideActive = true;
        s_led.phaseStartMs = millis();
        s_led.wipePosition = 0;

        if (cmd == '2')
        {
            s_led.currentMode = LedMode::BLINK;
            s_led.periodMs = period * 100; // 01..99 -> 100..9900 ms
            s_led.dutyCycle = 50;
        }
        else if (cmd == '3')
        {
            s_led.currentMode = LedMode::PULSE;
            s_led.periodMs = period * 100; // 01..99 -> 100..9900 ms
        }
        else
        { // cmd == '4'
            s_led.currentMode = LedMode::WIPE;
            s_led.periodMs = period * 10; // 01..99 -> 10..990 ms step time
        }

        return true;
    }

    // Unknown command
    Diag_incInvalidLedCommand();
    return false;
}
