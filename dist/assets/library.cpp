////////////////////////////////////////////////////////////////////////////////
// Robot Control Library
#define private public
#include <XBee.h>
#undef private
#include <avr/wdt.h>
#define FRAME_TYPE_PING 0x01
#define FRAME_TYPE_AUTO 0x02
#define FRAME_TYPE_TELEOP 0x03
#define FRAME_TYPE_RESPONSE 0x04
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
robot_control::_RobotControl RobotControl;
extern void loopAuto();
extern void loopTeleop();
namespace robot_control {
XBee theXbee;
volatile float axesAndButtons[21];
uint8_t defaultResponse = FRAME_TYPE_RESPONSE;
unsigned long lastResponseMicros = 0;
bool isBegun = false;
bool hasEnabled = false;
volatile uint8_t lastCode = FRAME_TYPE_PING;
volatile bool readyToSend;
String _ls("\x04");
Tx64Request txr64;
Rx64Response rxr64;
Tx16Request txr16;
Rx16Response rxr16;
void parseFrame(uint8_t* rxData, int) {
  lastCode = rxData[0];
  if (lastCode == FRAME_TYPE_PING || lastCode == FRAME_TYPE_AUTO) {
    for (int i = 0; i < 21; i++) {
      axesAndButtons[i] = 0;
    }
  } else if (lastCode == FRAME_TYPE_TELEOP) {
    if (rxData[1] == 4 && (rxData[2] == 16 || rxData[2] == 17)) {
      axesAndButtons[20] = 0;
      for (int i = 0; i < rxData[1] + rxData[2]; i++) {
        uint8_t fourBytes[4] = { rxData[3 + i*4], rxData[3 + i*4 + 1], rxData[3 + i*4 + 2], rxData[3 + i*4 + 3] };
        float* theFloat = (float*) fourBytes;
        axesAndButtons[i] = *theFloat;
      }
    }    
  }
}
void send(XBeeRequest &request) {
	theXbee.sendByte(START_BYTE, false);
	uint8_t msbLen = ((request.getFrameDataLength() + 2) >> 8) & 0xff;
	uint8_t lsbLen = (request.getFrameDataLength() + 2) & 0xff;
	theXbee.sendByte(msbLen, true);
	theXbee.sendByte(lsbLen, true);
	theXbee.sendByte(request.getApiId(), true);
	theXbee.sendByte(request.getFrameId(), true);
	uint8_t checksum = 0;
	checksum+= request.getApiId();
	checksum+= request.getFrameId();
	for (int i = 0; i < request.getFrameDataLength(); i++) {
		theXbee.sendByte(request.getFrameData(i), true);
		checksum+= request.getFrameData(i);
	}
	checksum = 0xff - checksum;
	theXbee.sendByte(checksum, true);
}
void receive16() {
  lastResponseMicros = micros();
  txr16.setAddress16(rxr16.getRemoteAddress16());
  parseFrame(rxr16.getFrameData() + rxr16.getDataOffset(), rxr16.getDataLength());
  if (!readyToSend) {
    txr16.setPayload(&defaultResponse);
    txr16.setPayloadLength(1);
  } else {
    txr16.setPayload((uint8_t*) _ls.c_str());
    txr16.setPayloadLength(_ls.length());
  }
  send(txr16);
  if (readyToSend) {
    _ls = "\x04";
    readyToSend = false;
  }
}
void receive64() {
  lastResponseMicros = micros();
  txr64.setAddress64(rxr64.getRemoteAddress64());
  parseFrame(rxr64.getFrameData() + rxr64.getDataOffset(), rxr64.getDataLength());
  if (!readyToSend) {
    txr64.setPayload(&defaultResponse);
    txr64.setPayloadLength(1);
  } else {
    txr64.setPayload((uint8_t*) _ls.c_str());
    txr64.setPayloadLength(_ls.length());
  }
  send(txr64);
  if (readyToSend) {
    _ls = "\x04";
    readyToSend = false;
  }
}
void _RobotControl::begin() {
  if (isBegun) {
    return;
  }
  wdt_disable();
  Serial.end();
  Serial.begin(57600);
  theXbee.setSerial(Serial);
  isBegun = true;
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);
}
float _RobotControl::getAxis(ControllerAxis axis) {
  return axesAndButtons[axis];
}
float _RobotControl::getButton(ControllerButton button) {
  return axesAndButtons[button];
}
void _RobotControl::log(String item) {
  if (!readyToSend) {
    if (_ls.length() < 100) {
      _ls.concat(item);
      _ls.concat('\n');
    }
    readyToSend = true;
  }
}
void _RobotControl::loop() {
  unsigned long currentMicros = micros();
  if (currentMicros - lastResponseMicros > 500000) {
    for (int i = 0; i < 21; i++) {
      axesAndButtons[i] = 0;
    }
    lastCode = FRAME_TYPE_PING;
  }
  theXbee.readPacket();
  if (theXbee.getResponse().isAvailable()) {
    if (theXbee.getResponse().getApiId()==RX_16_RESPONSE) {
      theXbee.getResponse().getRx16Response(rxr16);
      receive16();
    } else if (theXbee.getResponse().getApiId()==RX_64_RESPONSE) {
      theXbee.getResponse().getRx64Response(rxr64);
      receive64();
    }
  }
  if (lastCode == FRAME_TYPE_PING) {
    if (hasEnabled) {
      wdt_enable(WDTO_60MS);
      hasEnabled = false;
    }
  } else if(!hasEnabled) {
    hasEnabled = true;
  }
}
}
SIGNAL(TIMER0_COMPA_vect) {
  RobotControl.loop();
}
extern void clearWdt(void) __attribute__ ((naked))
                    __attribute__ ((used))
                    __attribute__ ((section (".init0")));
void clearWdt(void) {
  wdt_reset();
  wdt_disable();
}
void loop() {  
  if (!robot_control::isBegun) {
    RobotControl.begin();
  }
  if (robot_control::lastCode == FRAME_TYPE_AUTO) {
    loopAuto();
  } else if (robot_control::lastCode == FRAME_TYPE_TELEOP) {
    loopTeleop();
  } else {
    analogWrite(LED_BUILTIN, 0);
    analogWrite(5, 0);
    analogWrite(6, 0);
    analogWrite(9, 0);
    analogWrite(10, 0);
    analogWrite(11, 0);
    wdt_disable();
  }
}
////////////////////////////////////////////////////////////////////////////////

void setup() {
    // automatically calls Serial.begin(57600)
    RobotControl.begin();
}

void loopAuto() {
    // add your auto code here
}

void loopTeleop() {
    // add teleop code here
    float ab = RobotControl.getButton(A);
    float lx = RobotControl.getAxis(LEFT_X);
}