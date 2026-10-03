#include <Arduino.h>
#include "CytronMotorDriver.h"
#include "Bluepad32.h"

#define PWM1 18
#define DIR1 19
#define PWM2 22
#define DIR2 23

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
}

void setup() {
  Serial.begin(115200);

  BP32.setup(&OnConnectedGamepad, &OnDisconnectedGamepad);
}

unsigned long lastUpdate = 0;
void loop() {
  BP32.update();

  if (gamepad && gamepad->isConnected()) {
    if (millis() - lastUpdate >= 20) {
      lastUpdate = millis();
      
      int leftY = -(gamepad->axisY());
      int rightX = gamepad->axisRX();

      int forwardL = constrain(map(leftY, -512, 512, -255, 255), -255, 255);
      int forwardR = constrain(map(leftY, -512, 512, -255, 255), -255, 255);

      if (rightX < 0) { // Turning Left
        forwardL -= constrain(map(rightX, -512, 512, -100, 100), -100, 100);
      }
      else if (rightX > 0) { // Turning Right
        forwardR -= constrain(map(rightX, -512, 512, -100, 100), -100, 100);
      }

      motorL.setSpeed(forwardL);
      motorR.setSpeed(forwardR);
    }
  }
}