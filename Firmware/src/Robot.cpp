#include <Arduino.h>
#include "CytronMotorDriver.h"
#include "Bluepad32.h"

#define PWM1 18
#define DIR1 19
#define PWM2 22
#define DIR2 23

#define MAX_WHEEL_SPEED 13.6
#define WATCHDOG_MS 300
#define JOY_PERIOD_MS 20

#define LEFT_ENCODER_A 32
#define LEFT_ENCODER_B 33
#define RIGHT_ENCODER_A 25
#define RIGHT_ENCODER_B 26

CytronMD motorL(PWM_DIR, PWM1, DIR1);
CytronMD motorR(PWM_DIR, PWM2, DIR2);
GamepadPtr gamepad;

unsigned long lastJoySend = 0;
unsigned long lastVTime = 0;
String rxLine;

int leftEncoder = 0;
int rightEncoder = 0;

void OnConnectedGamepad(GamepadPtr gp) {
    Serial.println("Gamepad connected");
    gamepad = gp;
}

void OnDisconnectedGamepad(GamepadPtr gp) {
    Serial.println("Gamepad disconnected");
    gamepad = nullptr;
    motorL.setSpeed(0);
    motorR.setSpeed(0);
}

void handleLine(const String& line) {
  float left, right;
  if (sscanf(line.c_str(), "V %f %f", &left, &right) == 2) {
    int speedL = constrain((int)(left  / MAX_WHEEL_SPEED * 255), -255, 255);
    int speedR = constrain((int)(right / MAX_WHEEL_SPEED * 255), -255, 255);

    motorL.setSpeed(speedL);
    motorR.setSpeed(speedR);
    lastVTime = millis();
  }
}

void setup() {
  Serial.begin(115200);
  analogWriteFrequency(20000);

  BP32.setup(&OnConnectedGamepad, &OnDisconnectedGamepad);
}

void loop() {
  BP32.update();
 
  while (Serial.available() > 0) {
    char c = Serial.read();
 
    if (c == '\n') {
      float left, right;
      if (sscanf(rxLine.c_str(), "V %f %f", &left, &right) == 2) {
        int speedL = constrain((int)(left  / MAX_WHEEL_SPEED * 255), -255, 255);
        int speedR = constrain((int)(right / MAX_WHEEL_SPEED * 255), -255, 255);
 
        motorL.setSpeed(speedL);
        motorR.setSpeed(speedR);
        lastVTime = millis();
      }
      rxLine = "";
    } else if (c != '\r') {
      rxLine += c;
      if (rxLine.length() > 64) rxLine = "";
    }
  }
 
  if (gamepad && gamepad->isConnected() && millis() - lastJoySend >= JOY_PERIOD_MS) {
    lastJoySend = millis();
 
    float leftY = -(gamepad->axisY()  / 512.0f);
    float rightX = (gamepad->axisRX() / 512.0f);
 
    Serial.print("J ");
    Serial.print(leftY, 3);
    Serial.print(" ");
    Serial.println(rightX, 3);

    Serial.print("E ");
  }
 
  if (millis() - lastVTime > WATCHDOG_MS) {
    motorL.setSpeed(0);
    motorR.setSpeed(0);
  }
}