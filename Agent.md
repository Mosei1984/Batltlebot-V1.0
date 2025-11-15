Du bist ein erfahrener Embedded-C++-Entwickler für Arduino/ESP32-Systeme.  
Deine Aufgabe ist es, in meinem Battlebot-Projekt eine robuste, nicht blockierende WS2812B-LED-Steuerung zu integrieren – sauber modular, mit Logging/Debug und ohne das bestehende Timing oder die Failsafes zu stören.

### 1. Projektüberblick (bitte zuerst selbst analysieren)
Lies ALLE C++-Dateien im Projektordner (u. a.):

- main.cpp
- Config.h
- State.h
- DebugIO.h / DebugIO.cpp
- Drive.h / Drive.cpp
- Weapon.h / Weapon.cpp
- BluetoothComm.h / BluetoothComm.cpp
- CommandParser.h / CommandParser.cpp
- Diagnostics.h / Diagnostics.cpp
- Failsafe.h / Failsafe.cpp
- (sowie die ältere battlebot_code.txt als Referenz für frühere Licht/Hupe-Kommandos)

Verstehe:
- Wie der Hauptloop in main.cpp mit `LOOP_INTERVAL_MS` getaktet ist.
- Wie Fahrlogik (`Drive_*`), Waffenlogik (`Weapon_*`), Failsafe (`Failsafe_*`), Bluetooth-Kommunikation (`BluetoothComm_*`), CommandParser und Diagnostics zusammenarbeiten.
- Welche Pins in `Config.h` bereits für LEDs (`PIN_LED_ARM` etc.) und Debug benutzt werden.

Dokumentiere dir intern grob:
- Welche Zustände es gibt (`BotState`, `WeaponState`),
- Wie Befehle über Bluetooth reinkommen,
- Wo aktuell LED-Pins nur per `digitalWrite` geschaltet werden.

### 2. Neues LED-Modul WS2812B

Erstelle ein neues, eigenständiges Modul:

- `Leds.h`
- `Leds.cpp`

Anforderungen:
- Nutze eine verbreitete, stabile Library für WS2812B (z. B. Adafruit_NeoPixel oder FastLED – wähle passend zur Zielplattform).
- Lege ALLE WS2812-spezifischen Dinge zentral in diesem Modul ab:
  - Pin-Definition (über `Config.h`),
  - Anzahl LEDs,
  - Helligkeit,
  - Farbdefinitionen und Effekte,
  - Initialisierung, Update-Loop.
- Biete eine kleine, klare API, z. B.:
  - `void Leds_init();`
  - `void Leds_update(unsigned long nowMs);`  // NICHT blockierend
  - `void Leds_setMode(uint8_t mode);`        // z. B. OFF, IDLE, DRIVE, ARMING, ARMED, ERROR
  - ggf. `void Leds_setCustomColor(uint8_t r, uint8_t g, uint8_t b);`
- Die Effekte (Blinken, Wipe, Puls, Strobe usw.) müssen strikt nicht blockierend sein:
  - Keine `delay()`.
  - Nur zustandsbasiert + Zeitdifferenzen (`millis()`) innerhalb von `Leds_update(...)`.
  - Die Zykluszeit des Hauptloops (`LOOP_INTERVAL_MS`) darf NICHT verletzt werden.

### 3. Konfiguration in Config.h

- Ergänze in `Config.h`:
  - `constexpr int PIN_LED_WS2812 = ...;`   // wähle einen sinnvollen, noch freien Pin
  - `constexpr int LED_WS2812_COUNT = ...;` // z. B. Anzahl LEDs im Ring/Strip
  - Optional: Default-Helligkeit, Standardfarben für MODI (Drive, Arming, Error, etc.)
- Lasse bestehende Pins (z. B. `PIN_LED_ARM`) bestehen, aber:
  - Entweder:
    - Nutze sie weiter als einfache Status-LED zusätzlich zu WS2812,
  - Oder:
    - Markiere sie klar als „Legacy“ und route ihre Logik in das neue WS2812-Modul um (z. B. `Leds_setMode` wird statt `digitalWrite` genutzt).

### 4. Integration in main.cpp

- Füge in `setup()` hinzu:
  - `Leds_init();`
- Füge in der Hauptloop eine regelmäßige Aktualisierung ein:
  - z. B. im gleichen Block wie `Drive_update()` / `Weapon_update()`:
    ```cpp
    if (dtMs >= LOOP_INTERVAL_MS) {
        lastLoopMs = nowMs;

        Drive_update();
        Weapon_updateArming(nowMs);
        Weapon_update(dtMs, nowMs);
        Failsafe_update(nowMs);
        Leds_update(nowMs);
        Diag_update(nowMs);
    }
    ```
- Stelle sicher, dass `Leds_update` keine langen Blockaden erzeugt und innerhalb der bestehenden Timing-Architektur sauber läuft.

### 5. Zustands-basierte LED-Modi

Kopple die LED-Modi an die bestehenden Zustände:

- `Drive_getState()`:
  - `IDLE`    → LED-Modus „Idle“ (z. B. schwaches Pulsieren in einer neutralen Farbe).
  - `DRIVE`   → LED-Modus „Drive“ (z. B. laufender Effekt in Fahrtrichtung/Farbe).
- `Weapon_getState()`:
  - `DISARMED` → neutrale Farbe.
  - `ARMING`   → auffälliges Blink/Puls-Pattern (Warnung).
  - `ARMED`    → stabile Warnfarbe (z. B. dauerhaft rot oder schnell pulsierend).
- Failsafe / Fehler:
  - Wenn Failsafe auslöst oder Diagnostik-Fehler gezählt werden:
    - LED-Modus „Error“ (z. B. rotes Doppel-Blinkmuster).
  - Nutze dazu ggf. Informationen aus `Diagnostics` (Fehlerzähler) und `Failsafe`.

Implementiere das so, dass:
- Drive/Weapon/Failsafe über einfache Funktionsaufrufe das LED-Modul informieren:
  - z. B. `Leds_setMode(LED_MODE_ARMING);`
- Die eigentliche Effektlogik vollständig in `Leds.cpp` bleibt (keine Duplikate in den anderen Modulen).

### 6. Steuerung über App / Bluetooth & Debug

Die Steuerung des Bots erfolgt über Bluetooth + CommandParser. Erweitere diese Schicht, um auch LED-Modi konfigurieren zu können:

1. **Bluetooth & CommandParser**:
   - Identifiziere, wie aktuell Befehle eingelesen werden:
     - `BluetoothComm_poll(...)`
     - `CommandParser_handleLine(...)`
   - Ergänze neue Kommandos, z. B.:
     - `LEDx` → LED-Modus x setzen (z. B. LED0 = OFF, LED1 = IDLE, LED2 = DRIVE, LED3 = ARMING, LED4 = ARMED, LED9 = ERROR).
     - Optional: `LEDr,g,b` → benutzerdefinierte Farbe setzen.
   - Implementiere robuste Eingabevalidierung:
     - Ungültige Werte loggen (Diagnostics),
     - Keine Abstürze bei fehlerhaften Strings.
   - Alle neuen Kommandos sollen nicht blockierend sein und sofort in den internen LED-Status übernommen werden.

2. **Debug / Logging**:
   - Nutze `Diagnostics`-Modul, um:
     - Fehler bei LED-Initialisierung / Library-Fehlern zu zählen.
     - Zeitüberschreitungen oder Überläufe im WS2812-Update zu erkennen (falls möglich).
   - Nutze `Serial.println(F(...))` nur sparsam und vorzugsweise im Debug-/Diagnostics-Kontext, damit die Bluetooth-Link-Stabilität nicht leidet.

### 7. Timing, Failsafes und Defensive Programmierung

- Stelle sicher, dass die WS2812-Ansteuerung:
  - keine langen Sperren im Interrupt-Kontext erzeugt,
  - die Taktung des Steuerloops (`LOOP_INTERVAL_MS = 10 ms`) nicht verletzt,
  - Failsafe-Zeitgrenzen NICHT verlängert oder unzuverlässig macht.
- Wenn die gewählte WS2812-Library kritisch bzgl. Timing ist:
  - Implementiere eine klare Kommentar-Sektion in `Leds.cpp`, welche Randbedingungen beschreibt (z. B. „kein Aufruf mehr als X mal pro Sekunde nötig“).
- Defensive Checks:
  - Wenn LED-Anzahl oder Pin falsch konfiguriert sind, soll das System NICHT abstürzen:
    - Fail gracefully: LED-Modul deaktivieren, Warnung in `Diagnostics` setzen und weiterlaufen.
- Bei jedem externen Kommando (von App/Bluetooth) prüfe:
  - Wertebereiche (Mode, Farbwerte 0–255),
  - Zeichensatz (nur erwartete Buchstaben/Zahlen),
  - Ignoriere oder logge alles Unerwartete.

### 8. Bestehende einfache LEDs (Toggle-Pins)

- Wenn bisher simple LEDs per `digitalWrite` gesteuert werden (z. B. Waffen-Arming-LED):
  - Du darfst sie weiter nutzen ODER sie durch WS2812-Anzeigen ersetzen.
- Wichtig: Ändere bestehendes Verhalten NICHT, ohne es funktional ÄQUIVALENT oder VERBESSERT wiederherzustellen.
- Dokumentiere im Code mit klaren Kommentaren, wo Verhalten von „einfachem Pin-Toggle“ auf „WS2812-Effekt“ migriert wurde.

### 9. Code-Qualität

- Schreibe klaren, gut kommentierten C++-Code.
- Nutze `constexpr`, `enum class` und `static` wo sinnvoll.
- Achte auf:
  - keine Magic Numbers – stattdessen Konstanten in `Config.h`,
  - kurze, prägnante Funktionen,
  - keine duplizierte Logik.
- Kompiliere gedanklich das Gesamtprojekt:
  - Keine Kreisabhängigkeiten der Header,
  - Forward Declarations wo nötig,
  - `#pragma once` in neuen Headern.

Ziel: Nach deiner Anpassung hat das Projekt eine professionelle, modulare WS2812B-LED-Ansteuerung, die:
- über App/Bluetooth steuerbar ist,
- den Bot-Status (Fahren, Waffe, Failsafes, Fehler) klar visualisiert,
- das bestehende Timing und die Failsafes nicht stört,
- debug- und diagnosefreundlich ist.

---

## 10. Notch-Filter für Waffenmotor (implementiert)

Das System verfügt über einen dynamischen Notch-Filter zur Vermeidung von Resonanzfrequenzen am ESC-Ausgang.

### Architektur
- **Modul**: `NotchFilter.h` / `NotchFilter.cpp`
- **Typ**: Kennlinien-Transformation (Post-Rampe, nicht-blockierend)
- **Integration**: Weapon.cpp (angewandt nach Rampe, vor PWM-Ausgabe)
- **Aktivierung**: Nur im Zustand ARMED und oberhalb ESC_ARM_US + 5µs

### Funktionsweise
- Mehrere Notches (max. 8) können gleichzeitig definiert werden
- Jeder Notch: centerUs (Zentrum), halfWidthUs (Breite), depth (0.0-1.0 Dämpfung)
- Dreieckfenster-Algorithmus mit multiplicativer Kombination
- Global EIN/AUS schaltbar (Standard: AUS für Sicherheit)
- Kein Einfluss auf Arming-Sequenz oder Failsafes

### Befehle (USB Serial & Bluetooth)
- `NFEN=1` / `NFEN=0` → Filter global aktivieren/deaktivieren (Safety-Feature!)
- `NF+<centerUs>,<halfWidthUs>,<depth>` → Notch hinzufügen (z.B. NF+1500,100,0.5)
- `NF-` → Alle Notches löschen
- `NF?` → Konfiguration anzeigen
- `NF#<id>` → Notch mit ID entfernen

### Beispiel
```
NFEN=1              // Filter aktivieren
NF+1500,100,0.7     // Notch bei 1500µs, ±100µs Breite, 70% Dämpfung
NF+1800,50,0.5      // Zweiter Notch bei 1800µs, ±50µs, 50% Dämpfung
NF?                 // Status prüfen
```

### Performance
- CPU-Last: <30µs pro Loop @ 100Hz (ESP32 @240MHz)
- Keine Blockaden, kein delay()
- Kompatibel mit bestehenden Rampen und Failsafes

### Hinweis: KEIN PID-Controller
Ein PID-Controller wurde bewusst NICHT implementiert, da:
- Das System Open-Loop ohne RPM-Feedback arbeitet
- PID ohne Rückmeldung keinen Nutzen bringt (kann sogar schaden via Windup)
- Notch-Filter + Rampen ausreichend für Resonanzvermeidung
- Bei Bedarf später mit ESC-Telemetrie/Tachometer nachrüstbar

### Input-Erweiterung
`BluetoothComm_poll()` akzeptiert jetzt Befehle von beiden Quellen parallel:
1. USB Serial (z.B. für Debugging/Tuning am Tisch)
2. Bluetooth (App-Steuerung im Einsatz)
