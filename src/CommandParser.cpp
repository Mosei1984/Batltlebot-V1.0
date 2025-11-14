#include "CommandParser.h"
#include "Drive.h"
#include "Weapon.h"
#include "Config.h"
#include "Diagnostics.h"
#include "Failsafe.h"
#include "Leds.h"
#include <Arduino.h>

static void handleMotion(const String &input, unsigned long nowMs);
static void handleFunction(char cmd, unsigned long nowMs);

void CommandParser_handleLine(const String &line, unsigned long nowMs)
{
    // LED commands start with 'L'
    if (line.length() >= 2 && line[0] == 'L')
    {
        Leds_handleCommand(line);
        Failsafe_onAnyCommand(nowMs);
        return;
    }

    if (line.length() == 6)
    {
        handleMotion(line, nowMs);
        Failsafe_onAnyCommand(nowMs);
    }
    else if (line.length() == 1)
    {
        handleFunction(line[0], nowMs);
        Failsafe_onAnyCommand(nowMs);
    }
    else
    {
        Serial.print(F("[DBG] Unknown cmd (len!=1/6): \""));
        Serial.print(line);
        Serial.println(F("\""));
        Diag_incInvalidMotionFormat(); // generischer Formatfehler
    }
}

static void handleMotion(const String &input, unsigned long nowMs)
{
    // Format: [0]=F/B, [1..2]=00..99, [3]=L/R, [4..5]=00..99
    if (input.length() != 6)
    {
        Diag_incInvalidMotionFormat();
        Serial.println(F("[ERR] Motion length != 6"));
        Drive_setTargets(0, 0);
        return;
    }

    char moveDir = input[0];
    String spStr = input.substring(1, 3);
    char steerDir = input[3];
    String angStr = input.substring(4, 6);

    if (spStr.length() != 2 || !isDigit(spStr.charAt(0)) || !isDigit(spStr.charAt(1)))
    {
        Diag_incInvalidMotionFormat();
        Serial.println(F("[ERR] Motion speed not numeric"));
        Drive_setTargets(0, 0);
        return;
    }

    if (angStr.length() != 2 || !isDigit(angStr.charAt(0)) || !isDigit(angStr.charAt(1)))
    {
        Diag_incInvalidMotionFormat();
        Serial.println(F("[ERR] Motion angle not numeric"));
        Drive_setTargets(0, 0);
        return;
    }

    int moveSpeed = spStr.toInt();   // 0..99
    int steerAngle = angStr.toInt(); // 0..99

    // Begrenzen
    if (moveSpeed < 0 || moveSpeed > 99)
    {
        Diag_incInvalidMotionFormat();
        Serial.println(F("[ERR] Motion speed out of range"));
        moveSpeed = constrain(moveSpeed, 0, 99);
    }
    if (steerAngle < 0 || steerAngle > 99)
    {
        Diag_incInvalidMotionFormat();
        Serial.println(F("[ERR] Motion angle out of range"));
        steerAngle = constrain(steerAngle, 0, 99);
    }

    float Tsign;
    if (moveDir == 'F')
        Tsign = 1.0f;
    else if (moveDir == 'B')
        Tsign = -1.0f;
    else
    {
        Diag_incInvalidMoveDir();
        Serial.print(F("[ERR] Invalid moveDir: "));
        Serial.println(moveDir);
        // defensive: kein Move -> Stop
        Drive_setTargets(0, 0);
        return;
    }

    float Ssign;
    if (steerDir == 'R')
        Ssign = 1.0f;
    else if (steerDir == 'L')
        Ssign = -1.0f;
    else
    {
        Diag_incInvalidSteerDir();
        Serial.print(F("[ERR] Invalid steerDir: "));
        Serial.println(steerDir);
        // defensive: kein Lenken, aber geradeaus fahren ok:
        Ssign = 0.0f;
    }

    float T = Tsign * (moveSpeed / 99.0f);
    float S = Ssign * (steerAngle / 99.0f);

    float left = T + S;
    float right = T - S;

    if (left > 1.0f)
        left = 1.0f;
    if (left < -1.0f)
        left = -1.0f;
    if (right > 1.0f)
        right = 1.0f;
    if (right < -1.0f)
        right = -1.0f;

    int leftTarget = int(left * MAX_PWM);
    int rightTarget = int(right * MAX_PWM);

    Drive_setTargets(leftTarget, rightTarget);
    Failsafe_onMotionCommand(nowMs);

    Serial.print(F("[DBG] Motion: "));
    Serial.print(input);
    Serial.print(F(" -> L="));
    Serial.print(leftTarget);
    Serial.print(F(" R="));
    Serial.println(rightTarget);
}

static void handleFunction(char cmd, unsigned long nowMs)
{
    (void)nowMs; // aktuell nicht genutzt, aber für spätere Erweiterungen

    switch (cmd)
    {
    case 'U': // Waffe ARM
        Weapon_armRequest();
        break;

    case 'u': // Waffe DISARM
        Weapon_disarm();
        break;

    case 'W': // Vollgas (nur ARMED)
        Weapon_fullThrottle();
        break;

    case 'w': // Idle (ARM)
        Weapon_idle();
        break;

    // LED-Befehle (von App)
    case 'V': // LEDs AN (weiß)
        Leds_handleCommand("L1FFFFFF");
        Serial.println(F("[DBG] LEDs ON (white)"));
        break;

    case 'v': // LEDs AUS
        Leds_handleCommand("L0");
        Serial.println(F("[DBG] LEDs OFF"));
        break;

    case 'X':                             // LED Blink-Effekt (rot)
        Leds_handleCommand("L2FF000010"); // Rot blinken, 1 Sekunde
        Serial.println(F("[DBG] LEDs BLINK (red)"));
        break;

    case 'Z': // LED Auto-Modus
        Leds_handleCommand("LA");
        Serial.println(F("[DBG] LEDs AUTO mode"));
        break;

    case 'Y': // Horn/Hupe (aktuell keine Hardware)
        Serial.println(F("[DBG] Horn triggered (no hardware)"));
        break;

    default:
        Diag_incInvalidFunctionFormat();
        Serial.print(F("[DBG] Function cmd ignored: "));
        Serial.println(cmd);
        break;
    }
}
