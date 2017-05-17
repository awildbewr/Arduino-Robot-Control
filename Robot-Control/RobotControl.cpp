#include "RobotControl.h"

#undef delay
#undef delayMicroseconds

#define FRAME_TYPE_PING 0x01
#define FRAME_TYPE_AUTO 0x02
#define FRAME_TYPE_TELEOP 0x03
#define FRAME_TYPE_RESPONSE 0x04

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

void parseFrame(uint8_t* rxData, int /*rxLength*/) {
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
	// perform 2s complement
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
    _ls = '\x04';
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
    _ls = '\x04';
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

void _rcdelay(unsigned long ms) {
  _rcudelay(ms * 1000);
}
void _rcudelay(unsigned long us) {
  if (us < 5000) {
    delayMicroseconds(us);
    return;
  }
  unsigned long start = micros();
  while (micros() - start < us) {
    //RobotControl.loop();
  }
}

void loop() {  
  if (!robot_control::isBegun) {
    RobotControl.begin();
  }
  
  //RobotControl.loop();
  
  if (robot_control::lastCode == FRAME_TYPE_AUTO) {
    loopAuto();
  } else if (robot_control::lastCode == FRAME_TYPE_TELEOP) {
    loopTeleop();
  } else {
    analogWrite(LED_BUILTIN, 0);
    //analogWrite(3, 0);
    analogWrite(5, 0);
    analogWrite(6, 0);
    analogWrite(9, 0);
    analogWrite(10, 0);
    analogWrite(11, 0);
    wdt_disable();
  }
}