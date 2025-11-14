#include <Arduino.h>

/*
  ESP32 Battlebot Steuerung
  - basiert auf deinem App-Kommando-Schema (F,B,L,R,G,H,I,J,S,Y,U,u,1..4)
  - 2x DC-Motor über L298N (nur 4x IN)
  - 1x Brushless/ESC als Waffenmotor (Soft Spin-Up)
  - Bluetooth statt USB-Serial

  App-Kommandos (wie dein Beispiel):
    F = vorwärts
    B = rückwärts
    L = links drehen
    R = rechts drehen
    G = vorwärts links
    H = vorwärts rechts
    I = rückwärts links
    J = rückwärts rechts
    S = stopp
    Y = Hupe/Buzzer
    U = Licht an
    u = Licht aus
    1..4 = Speedstufen

  Zusatz für dich:
    W = Waffe an (ESC langsam hoch)
    w = Waffe aus
*/

#include <BluetoothSerial.h>
BluetoothSerial SerialBT;

// ---------- Pins anpassen ----------
const int IN1 = 25;   // Motor links
const int IN2 = 26;   // Motor links
const int IN3 = 27;   // Motor rechts
const int IN4 = 14;   // Motor rechts

const int buzPin = 2;    // Buzzer
const int ledPin = 15;   // LED (achte: 34 ist input-only! -> nimm besser 15 oder 12!)
                        // falls du A5 vom Arduino meintest, setz hier einfach einen freien ESP32-GPIO, z.B. 15

// ---------- Waffen-ESC ----------
const int WEAPON_PIN = 4;
const int WEAPON_CHANNEL = 5;   // eigener PWM-Kanal
const int WEAPON_FREQ = 50;
const int WEAPON_RES = 16;
const int ESC_MIN_US = 1000;
const int ESC_ARM_US = 1100;
const int ESC_MAX_US = 2000;
int currentWeaponUs = ESC_MIN_US;
int targetWeaponUs  = ESC_MIN_US;
int weaponStepUs    = 1;       // wie weich das Hochfahren ist

// ---------- Fahrt ----------
int valSpeed = 255;    // aktuelle Geschwindigkeit (wird von '1'..'4' gesetzt)
int speedLeft = 255;   // aktuelle Geschwindigkeit links
int speedRight = 255;  // aktuelle Geschwindigkeit rechts

// µs -> Duty für ESC
uint32_t usToDuty(int us) {
  uint32_t maxDuty = (1 << WEAPON_RES) - 1;
  return (uint32_t)(((float)us / 20000.0f) * maxDuty);
}

// Motor-Richtungen (wie AFMotor)
enum MotorDirection { FORWARD, BACKWARD, RELEASE };

// Motor links steuern
void runLeftMotor(MotorDirection dir) {
  if (dir == FORWARD) {
    ledcWrite(0, speedLeft);  // IN1 PWM
    ledcWrite(1, 0);          // IN2 LOW
  } else if (dir == BACKWARD) {
    ledcWrite(0, 0);          // IN1 LOW
    ledcWrite(1, speedLeft);  // IN2 PWM
  } else {  // RELEASE
    ledcWrite(0, 0);
    ledcWrite(1, 0);
  }
}

// Motor rechts steuern
void runRightMotor(MotorDirection dir) {
  if (dir == FORWARD) {
    ledcWrite(2, speedRight); // IN3 PWM
    ledcWrite(3, 0);          // IN4 LOW
  } else if (dir == BACKWARD) {
    ledcWrite(2, 0);          // IN3 LOW
    ledcWrite(3, speedRight); // IN4 PWM
  } else {  // RELEASE
    ledcWrite(2, 0);
    ledcWrite(3, 0);
  }
}

// Geschwindigkeit setzen (wie AFMotor)
void setSpeedLeft(int speed) {
  if (speed > 255) speed = 255;
  if (speed < 0) speed = 0;
  speedLeft = speed;
}

void setSpeedRight(int speed) {
  if (speed > 255) speed = 255;
  if (speed < 0) speed = 0;
  speedRight = speed;
}

// Waffe sanft hochfahren, schnell stoppen
void updateWeapon() {
  if (currentWeaponUs < targetWeaponUs) {
    currentWeaponUs += weaponStepUs;
    if (currentWeaponUs > targetWeaponUs) currentWeaponUs = targetWeaponUs;
  } else if (currentWeaponUs > targetWeaponUs) {
    currentWeaponUs -= weaponStepUs * 20;
    if (currentWeaponUs < targetWeaponUs) currentWeaponUs = targetWeaponUs;
  }
  ledcWrite(WEAPON_CHANNEL, usToDuty(currentWeaponUs));
}

// Geschwindigkeitsstufe setzen (wie AFMotor)
void SetSpeed(int val) {
  valSpeed = val;
  setSpeedLeft(val);
  setSpeedRight(val);
}

void setup() {
  Serial.begin(115200);
  SerialBT.begin("BattleBotESP");
  Serial.println("BattleBotESP bereit.");

  pinMode(buzPin, OUTPUT);
  pinMode(ledPin, OUTPUT);

  // Motor-Pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // 4 PWM-Kanäle für 4 INs
  ledcSetup(0, 20000, 8);  // IN1
  ledcAttachPin(IN1, 0);

  ledcSetup(1, 20000, 8);  // IN2
  ledcAttachPin(IN2, 1);

  ledcSetup(2, 20000, 8);  // IN3
  ledcAttachPin(IN3, 2);

  ledcSetup(3, 20000, 8);  // IN4
  ledcAttachPin(IN4, 3);

  // ESC
  ledcSetup(WEAPON_CHANNEL, WEAPON_FREQ, WEAPON_RES);
  ledcAttachPin(WEAPON_PIN, WEAPON_CHANNEL);
  currentWeaponUs = ESC_ARM_US;
  targetWeaponUs  = ESC_ARM_US;
  ledcWrite(WEAPON_CHANNEL, usToDuty(currentWeaponUs));
  delay(1000);

  // Motoren zu Beginn stoppen
  runLeftMotor(RELEASE);
  runRightMotor(RELEASE);
}

void loop() {
  // statt Serial.read() jetzt Bluetooth
  if (SerialBT.available()) {
    char command = SerialBT.read();
    Serial.print("CMD: "); Serial.println(command);

    switch (command) {
      case 'F':   // vorwärts
        setSpeedLeft(valSpeed);
        setSpeedRight(valSpeed);
        runLeftMotor(FORWARD);
        runRightMotor(FORWARD);
        break;

      case 'B':   // rückwärts
        setSpeedLeft(valSpeed);
        setSpeedRight(valSpeed);
        runLeftMotor(BACKWARD);
        runRightMotor(BACKWARD);
        break;

      case 'L':   // links drehen (links rück, rechts vor)
        setSpeedLeft(valSpeed);
        setSpeedRight(valSpeed);
        runLeftMotor(BACKWARD);
        runRightMotor(FORWARD);
        break;

      case 'R':   // rechts drehen (links vor, rechts rück)
        setSpeedLeft(valSpeed);
        setSpeedRight(valSpeed);
        runLeftMotor(FORWARD);
        runRightMotor(BACKWARD);
        break;

      case 'G':   // vorwärts links
        setSpeedLeft(valSpeed / 4);
        setSpeedRight(valSpeed);
        runLeftMotor(FORWARD);
        runRightMotor(FORWARD);
        break;

      case 'H':   // vorwärts rechts
        setSpeedLeft(valSpeed);
        setSpeedRight(valSpeed / 4);
        runLeftMotor(FORWARD);
        runRightMotor(FORWARD);
        break;

      case 'I':   // rückwärts links
        setSpeedLeft(valSpeed / 4);
        setSpeedRight(valSpeed);
        runLeftMotor(BACKWARD);
        runRightMotor(BACKWARD);
        break;

      case 'J':   // rückwärts rechts
        setSpeedLeft(valSpeed);
        setSpeedRight(valSpeed / 4);
        runLeftMotor(BACKWARD);
        runRightMotor(BACKWARD);
        break;

      case 'S':   // stopp
        runLeftMotor(RELEASE);
        runRightMotor(RELEASE);
        break;

      case 'Y':   // hupe
        digitalWrite(buzPin, HIGH);
        delay(200);
        digitalWrite(buzPin, LOW);
        break;

      case 'U':   // licht an
        digitalWrite(ledPin, HIGH);
        break;

      case 'u':   // licht aus
        digitalWrite(ledPin, LOW);
        break;

      // speed presets
      case '1': SetSpeed(65); break;
      case '2': SetSpeed(130); break;
      case '3': SetSpeed(195); break;
      case '4': SetSpeed(255); break;

      // Waffe EIN
      case 'W':
        targetWeaponUs = 1950;   // oder 2000
        break;

      // Waffe AUS
      case 'w':
        targetWeaponUs = ESC_MIN_US;
        break;
    }
  }

  // Waffe smooth up/down
  updateWeapon();

  delay(5);  // Weniger Lag
}
