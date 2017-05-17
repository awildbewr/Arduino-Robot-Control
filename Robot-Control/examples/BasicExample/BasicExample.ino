#include <RobotControl.h>

void setup() {
  // automatically sets up Serial for you
  // because Serial is required for wireless module communication
  // it does Serial.begin(57600)
  // 115200 doesn't work so 57600 is the next fastest baud rate
  // we need 20 kbits/s for communication (21 floats * 4 bytes per float * 30 fps * 8 bits per byte)
  // so regular 9600 baud doesn't work too well. If using serial monitor, set baud to 57600
  RobotControl.begin();
}

void loop() {
  // required call at the top of the loop function:
  RobotControl.loop();
  // NB: avoid delay() because it will delay controller input!
  
  float leftXAxis = RobotControl.getAxis(LEFT_X);
  // returns a value from -1.0 to 1.0
  // all axes:
  // LEFT_X
  // LEFT_Y
  // RIGHT_X
  // RIGHT_Y
  
  float buttonA = RobotControl.getButton(A);
  // returns 1.0 for pressed or 0.0 for not pressed
  float leftTrigger = RobotControl.getButton(TRIGGER_LEFT);
  // analog value from 0.0 to 1.0
  // all buttons:
  // A
  // B
  // X
  // Y
  // BUMPER_LEFT
  // BUMPER_RIGHT
  // TRIGGER_LEFT
  // TRIGGER_RIGHT
  // BACK
  // START
  // STICK_LEFT
  // STICK_RIGHT
  // HAT_NORTH
  // HAT_SOUTH
  // HAT_WEST
  // HAT_EAST
  // LOGITECH
  
  // NB: minimize floating-point math operations for maximum speed!
}