#include "BluetoothComm.h"
#include <BluetoothSerial.h>
#include "DebugIO.h"
#include "Diagnostics.h"

static BluetoothSerial SerialBT;
static String btBuffer;

void BluetoothComm_init() {
    SerialBT.begin("BattleBotESP");
    btBuffer.reserve(32);
}

bool BluetoothComm_poll(String &outLine, unsigned long /*nowMs*/) {
    while (SerialBT.available()) {
        char c = SerialBT.read();
        if (c == '\n' || c == '\r') {
            if (btBuffer.length() > 0) {
                DebugIO_pulseInput();
                outLine = btBuffer;
                btBuffer = "";
                outLine.trim();
                return true;
            }
        } else {
            btBuffer += c;
            if (btBuffer.length() > 16) {
                // Eingabe zu lang / vermutlich Müll -> verwerfen + zählen
                Diag_incBtBufferOverflow();
                Serial.println(F("[ERR] BT buffer overflow, discarding input"));
                btBuffer = "";
            }
        }
    }
    return false;
}
