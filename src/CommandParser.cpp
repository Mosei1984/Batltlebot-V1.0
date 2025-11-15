#include "CommandParser.h"
#include "Drive.h"
#include "Weapon.h"
#include "Config.h"
#include "Diagnostics.h"
#include "Failsafe.h"
#include "Leds.h"
#include "NotchFilter.h"
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

    // Notch Filter commands start with "NF"
    if (line.length() >= 2 && line.startsWith("NF"))
    {
        if (line == "NF?") {
            NotchFilter_dump(Serial);
        }
        else if (line == "NF-") {
            NotchFilter_clear();
            Serial.println(F("[NF] All notches cleared"));
        }
        else if (line.startsWith("NFEN=")) {
            int val = line.substring(5).toInt();
            NotchFilter_setEnabled(val != 0);
            Serial.print(F("[NF] Filter "));
            Serial.println(val != 0 ? F("ENABLED") : F("DISABLED"));
        }
        else if (line.startsWith("NF+")) {
            // Format: NF+centerUs,halfWidthUs,depth
            String params = line.substring(3);
            int comma1 = params.indexOf(',');
            int comma2 = params.indexOf(',', comma1 + 1);
            
            if (comma1 > 0 && comma2 > comma1) {
                int centerUs = params.substring(0, comma1).toInt();
                int halfWidthUs = params.substring(comma1 + 1, comma2).toInt();
                float depth = params.substring(comma2 + 1).toFloat();
                
                uint32_t id;
                if (NotchFilter_add(centerUs, halfWidthUs, depth, &id)) {
                    Serial.print(F("[NF] Added notch ID="));
                    Serial.print(id);
                    Serial.print(F(" center="));
                    Serial.print(centerUs);
                    Serial.print(F("us, width=±"));
                    Serial.print(halfWidthUs);
                    Serial.print(F("us, depth="));
                    Serial.println(depth, 2);
                } else {
                    Serial.println(F("[NF] ERROR: Failed to add notch (check params/max)"));
                }
            } else {
                Serial.println(F("[NF] ERROR: Format NF+centerUs,halfWidthUs,depth"));
            }
        }
        else if (line.startsWith("NF#")) {
            uint32_t id = line.substring(3).toInt();
            if (NotchFilter_removeById(id)) {
                Serial.print(F("[NF] Removed notch ID="));
                Serial.println(id);
            } else {
                Serial.print(F("[NF] ERROR: Notch ID="));
                Serial.print(id);
                Serial.println(F(" not found"));
            }
        }
        else {
            Serial.println(F("[NF] Unknown command. Use: NF?, NF-, NF+, NF#, NFEN="));
        }
        
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
