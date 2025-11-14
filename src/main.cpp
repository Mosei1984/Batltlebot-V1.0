#include <Arduino.h>
#include "Config.h"
#include "State.h"
#include "DebugIO.h"
#include "Drive.h"
#include "Weapon.h"
#include "BluetoothComm.h"
#include "CommandParser.h"
#include "Diagnostics.h"
#include "Failsafe.h"
#include "Leds.h"

unsigned long lastLoopMs = 0;

void setup() {
    Serial.begin(115200);

    pinMode(PIN_LED_ARM, OUTPUT);
    digitalWrite(PIN_LED_ARM, LOW);

    DebugIO_init();
    Diag_init();
    Drive_init();
    Weapon_init();
    BluetoothComm_init();
    Failsafe_init();
    Leds_init();

    lastLoopMs = millis();
    Serial.println(F("[DBG] Setup done. Waiting for commands..."));
}

void loop() {
    unsigned long nowMs = millis();
    unsigned long dtMs  = nowMs - lastLoopMs;

    // Eingaben IMMER erfassen
    String line;
    if (BluetoothComm_poll(line, nowMs)) {
        CommandParser_handleLine(line, nowMs);
    }

    if (dtMs >= LOOP_INTERVAL_MS) {
        lastLoopMs = nowMs;

        Drive_update();
        Weapon_updateArming(nowMs);
        Weapon_update(dtMs, nowMs);
        Failsafe_update(nowMs);
        Leds_update(nowMs);
        Diag_update(nowMs);  // periodische Fehlerstatistik
    }
}
