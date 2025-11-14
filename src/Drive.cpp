#include "Drive.h"
#include "Config.h"
#include "DebugIO.h"
#include <Arduino.h>

static int leftCmdTarget   = 0;
static int rightCmdTarget  = 0;
static int leftCmdCurrent  = 0;
static int rightCmdCurrent = 0;
static BotState botState   = BotState::IDLE;

void Drive_init() {
    pinMode(PIN_IN1, OUTPUT);
    pinMode(PIN_IN2, OUTPUT);
    pinMode(PIN_IN3, OUTPUT);
    pinMode(PIN_IN4, OUTPUT);

    ledcSetup(0, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcAttachPin(PIN_IN1, 0);
    ledcSetup(1, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcAttachPin(PIN_IN2, 1);
    ledcSetup(2, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcAttachPin(PIN_IN3, 2);
    ledcSetup(3, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    ledcAttachPin(PIN_IN4, 3);

    leftCmdTarget   = 0;
    rightCmdTarget  = 0;
    leftCmdCurrent  = 0;
    rightCmdCurrent = 0;
    botState        = BotState::IDLE;
}

void Drive_setTargets(int left, int right) {
    // Defensive: Clamp Werte auf -MAX_PWM..+MAX_PWM
    if (left  >  MAX_PWM) left  =  MAX_PWM;
    if (left  < -MAX_PWM) left  = -MAX_PWM;
    if (right >  MAX_PWM) right =  MAX_PWM;
    if (right < -MAX_PWM) right = -MAX_PWM;

    leftCmdTarget  = left;
    rightCmdTarget = right;
}

BotState Drive_getState() {
    return botState;
}

static void setLeftMotor(int speedVal) {
    if (speedVal > 0) {
        DebugIO_setLeftForward(true);
        digitalWrite(PIN_IN2, LOW);
        ledcWrite(0, speedVal);
        ledcWrite(1, 0);
    } else if (speedVal < 0) {
        DebugIO_setLeftForward(false);
        digitalWrite(PIN_IN1, LOW);
        ledcWrite(0, 0);
        ledcWrite(1, -speedVal);
    } else {
        DebugIO_setLeftForward(false);
        digitalWrite(PIN_IN1, LOW);
        digitalWrite(PIN_IN2, LOW);
        ledcWrite(0, 0);
        ledcWrite(1, 0);
    }
}

static void setRightMotor(int speedVal) {
    if (speedVal > 0) {
        DebugIO_setRightForward(true);
        digitalWrite(PIN_IN4, LOW);
        ledcWrite(2, speedVal);
        ledcWrite(3, 0);
    } else if (speedVal < 0) {
        DebugIO_setRightForward(false);
        digitalWrite(PIN_IN3, LOW);
        ledcWrite(2, 0);
        ledcWrite(3, -speedVal);
    } else {
        DebugIO_setRightForward(false);
        digitalWrite(PIN_IN3, LOW);
        digitalWrite(PIN_IN4, LOW);
        ledcWrite(2, 0);
        ledcWrite(3, 0);
    }
}

void Drive_update() {
    if (leftCmdCurrent != leftCmdTarget) {
        leftCmdCurrent = leftCmdTarget;
        setLeftMotor(leftCmdCurrent);
    }
    if (rightCmdCurrent != rightCmdTarget) {
        rightCmdCurrent = rightCmdTarget;
        setRightMotor(rightCmdCurrent);
    }

    BotState newState =
        (leftCmdTarget == 0 && rightCmdTarget == 0) ? BotState::IDLE : BotState::DRIVE;
    botState = newState;
}
