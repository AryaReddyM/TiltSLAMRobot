#include <Arduino.h>
#include "CytronMotorDriver.h"
#include "Bluepad32.h"

#define PWM1 18
#define DIR1 19
#define PWM2 22
#define DIR2 23

#define DEADZONE 40

CytronMD motorL(PWM_DIR, PWM1, DIR1);
CytronMD motorR(PWM_DIR, PWM2, DIR2);
GamepadPtr gamepad;

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

void setup() {
  Serial.begin(115200);
  analogWriteFrequency(20000);

  BP32.setup(&OnConnectedGamepad, &OnDisconnectedGamepad);
}

unsigned long lastUpdate = 0;
void loop() {
  BP32.update();

  if (gamepad && gamepad->isConnected()) {
    if (millis() - lastUpdate >= 20) {
      lastUpdate = millis();
      
      int leftY = (gamepad->axisY());
      int rightX = gamepad->axisRX();
    }
  }
}