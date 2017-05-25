#include <RobotControl.h>

void setup() {
  RobotControl.begin();
  
  pinMode(BUILTIN_LED, OUTPUT);
}

void loopAuto() {
  digitalWrite(BUILTIN_LED, HIGH);
  delay(1000);
  digitalWrite(BUILTIN_LED, LOW);
  delay(1000);
}

void loopTeleop() {
  float buttonPressed = RobotControl.getButton(A);
  if (buttonPressed) {
    digitalWrite(BUILTIN_LED, HIGH);
  } else {
    digitalWrite(BUILTIN_LED, LOW);
  }
}