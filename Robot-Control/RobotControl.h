#pragma once

#include <Arduino.h>
#define private public
#include <XBee.h>
#undef private
#include <avr/wdt.h>

enum ControllerAxis {
  LEFT_X = 0,
  LEFT_Y = 1,
  RIGHT_X = 2,
  RIGHT_Y = 3
};

enum ControllerButton {
  A = 4,
  B = 5,
  X = 6,
  Y = 7,
  BUMPER_LEFT = 8,
  BUMPER_RIGHT = 9,
  TRIGGER_LEFT = 10,
  TRIGGER_RIGHT = 11,
  BACK = 12,
  START = 13,
  STICK_LEFT = 14,
  STICK_RIGHT = 15,
  HAT_NORTH = 16,
  HAT_SOUTH = 17,
  HAT_WEST = 18,
  HAT_EAST = 19,
  LOGITECH = 20
};

namespace robot_control {  
  class _RobotControl {
  public:
    void begin();
    float getAxis(ControllerAxis);
    float getButton(ControllerButton);
    void log(String);
    void loop();
  };
}

extern robot_control::_RobotControl RobotControl;

extern void loop();

extern void loopAuto();
extern void loopTeleop();

extern void _rcdelay(unsigned long);
extern void _rcudelay(unsigned long);
#define delay _rcdelay
#define delayMicroseconds _rcudelay