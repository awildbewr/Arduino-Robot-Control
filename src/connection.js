const xbee_api = require('./xbee-api');
const Buffer = require('buffer').Buffer;
const C = xbee_api.constants;
const serial = chrome.serial;
const EventEmitter = require('events').EventEmitter;

const Controller = require('./controller.js');

let xbee = new xbee_api.XBeeAPI({
  api_mode: 2
});

var connected = false;
var selectedRemote = null;
var controlEnabled = false;
var matchType = "full";
var matchStart;
var autoLength = 15;
var teleopLength = 135;
let _pollTimeout;

var Connection = new EventEmitter();
Connection.xbee_api = xbee_api;
Connection.xbee = xbee;
Connection.serial = serial;
Connection.Buffer = Buffer;
Connection.isConnected = function() {
  return connected;
}
Connection.setControlEnabled = function(enabled, mt, al, tl) {
  console.info("Setting robot enable", enabled, mt, al, tl);
  controlEnabled = enabled;
  matchType = mt;
  autoLength = al;
  teleopLength = tl;
  
  if (enabled) {
    matchStart = new Date().getTime();
  }
  
  this.emit("status", {enabled: controlEnabled, mode: controlEnabled ? "Get ready..." : "Disabled", time: 0});
}

xbee.on("frame_object", function(frame) {
  console.log("Got frame", frame);
  
  Connection.emit("frame", frame);
});

function toArrayBuffer(buffer) {
    var ab = new ArrayBuffer(buffer.length);
    var view = new Uint8Array(ab);
    for (var i = 0; i < buffer.length; ++i) {
        view[i] = buffer[i];
    }
    return ab;
}

function toBuffer(ab) {
    var buffer = new Buffer(ab.byteLength);
    var view = new Uint8Array(ab);
    for (var i = 0; i < buffer.length; ++i) {
        buffer[i] = view[i];
    }
    return buffer;
}

let connectionId;

function connect(path, cb) {
  console.log("Connecting to", path);
  var bitrate = 57600;
  console.log("baud:", bitrate);
  serial.connect(path, {bitrate: bitrate}, function(connectionInfo) {
    console.log("Connected", connectionInfo.connectionId, connectionInfo);
    connected = true;
    Connection.emit("connected");
    connectionId = connectionInfo.connectionId;
    
    if (cb) cb();
  });
}

function onSentCallback(stat) {
  if (stat.error) {
    console.log("Disconnected");
    Connection.emit("disconnected");
    connected = false;
    selectedRemote = null;
    setPoll();
  }
  console.log("onSent", stat);
}

function sendTest() {
  var data = xbee.buildFrame({
    type: C.FRAME_TYPE.TX_REQUEST_16,
    destination16: "0001",
    options: 0x00,
    data: "Sample Text"
  });
  
  /*{
    type: C.FRAME_TYPE.AT_COMMAND,
    command: "ND",
    commandParameter: []
  }*/
  
  
  /*{
    type: C.FRAME_TYPE.TX_REQUEST_16,
    destination16: "0001",
    options: 0x00,
    data: "Sample Text"
  }*/
  
  /*{
      type: C.FRAME_TYPE.AT_COMMAND,
      command: "NI",
      commandParameter: "Sample Text"
  }*/
  chrome.serial.send(connectionId, toArrayBuffer(data), onSentCallback);
}
window.sendTest = sendTest;

var onReceiveCallback = function(info) {
  if (info.connectionId == connectionId && info.data) {
    console.log("Got data", info);
    xbee.parseRaw(toBuffer(info.data));
  } else {
    console.warn("Bad recieve", info);
  }
};

chrome.serial.onReceive.addListener(onReceiveCallback);

var pollingRemotes = false;
var foundRemotes = [];
var pollRemotesTimeout;

Connection.connectRemote = function(nodeIdentifier) {
  selectedRemote = nodeIdentifier;
  communicationLoop();
};

Connection.disconnectRemote = function() {
  selectedRemote = null;
  clearTimeout(commsInterval);
  setPoll();
};

Connection.on("frame", function(frame) {
  if (!pollingRemotes) return;
  
  if (frame.command != "ND") return;
  
  if (frame.hasOwnProperty("commandData") && frame.commandData.length == 0) {
    pollingRemotes = false;
    clearTimeout(pollRemotesTimeout);
    Connection.emit("scan", foundRemotes);
    setPoll();
  } else if (frame.hasOwnProperty("nodeIdentification")) {
    var ni = frame.nodeIdentification;
    foundRemotes.push(ni);
    setRemotePollAbort();
  }
});

function setRemotePollAbort() {
  clearTimeout(pollRemotesTimeout);
  pollRemotesTimeout = setTimeout(function() {
    pollingRemotes = false;
    foundRemotes = [];
    Connection.emit("scan", foundRemotes);
    setPoll();
  }, 5000);
}

function pollDevices() {
  if (selectedRemote != null) {
    setPoll();
    return;
  }
  
  if (connected) {
    var data = xbee.buildFrame({
      type: C.FRAME_TYPE.AT_COMMAND,
      command: "ND",
      commandParameter: []
    });
    pollingRemotes = true;
    setRemotePollAbort();
    foundRemotes = [];
    chrome.serial.send(connectionId, toArrayBuffer(data), onSentCallback);
    return;
  }
  console.log("Polling devices");
  serial.getDevices(function(devices) {
    console.log("Got devices:", devices);
    for(let device of devices) {
      if(device.productId == 24597 && device.vendorId == 1027) {
        console.log("Found FTDI231x");
        
        Connection.emit("opening_serial");
        
        connect(device.path, function() {
          setPoll();
        });
        
        return;
      }
    }
    setPoll();
  });
}

pollDevices();

function setPoll() {
  clearTimeout(_pollTimeout);
  _pollTimeout = setTimeout(pollDevices, 1000);
}

var controllerData = { buttons: [], axes: [] };
Controller.on("data", function(data) {
  controllerData = data;
});

var commsInterval;

const FRAME_TYPE_PING = 0x01;
const FRAME_TYPE_AUTO = 0x02;
const FRAME_TYPE_TELEOP = 0x03;
const FRAME_TYPE_RESPONSE = 0x04;

Connection.on("frame", function(frame) {
  if ((frame.type == C.FRAME_TYPE.RX_PACKET_64 || frame.type == C.FRAME_TYPE.RX_PACKET_16) && frame.hasOwnProperty("data")) {
    var data = frame.data;
    console.info(data.toString().replace("\x04","RESP "));
    if (data[0] == FRAME_TYPE_RESPONSE) {
      var logString = data.slice(1).toString();
      Connection.emit("ping");
      if (logString.length > 0) {
        console.log("Remote log", logString);
        Connection.emit("log", logString);
      }
    }
  }
});

function communicationLoop() {
  clearTimeout(commsInterval);
  if (selectedRemote == null) return;
  
  var now = new Date().getTime();
  
  var mode;
  var time;
  
  if (controlEnabled) {
    if (matchType == "full") {
      if (now - matchStart <= autoLength*1000) {
        mode = "Auto";
        time = autoLength*1000 - (now - matchStart);
      } else if (now - matchStart <= autoLength*1000 + 2000) {
        mode = "Get ready...";
        time = 0;
      } else if (now - matchStart <= autoLength*1000 + 2000 + teleopLength*1000) {
        mode = "Teleop";
        time = autoLength*1000 + 2000 + teleopLength*1000 - (now - matchStart);
      } else {
        mode = "Disabled";
        controlEnabled = false;
        time = 0;
      }
    } else if (matchType == "auto") {
      if (now - matchStart <= autoLength*1000) {
        mode = "Auto";
        time = autoLength*1000 - (now - matchStart);
      } else {
        mode = "Disabled";
        controlEnabled = false;
        time = 0;
      }
    } else if (matchType == "teleop") {
      if (now - matchStart <= 2000) {
        mode = "Get ready...";
        time = 0;
      } else if (now - matchStart <= teleopLength*1000 + 2000) {
        mode = "Teleop";
        time = teleopLength*1000 + 2000 - (now - matchStart);
      } else {
        mode = "Disabled";
        controlEnabled = false;
        time = 0;
      }
    }
  } else {
    mode = "Disabled";
    time = 0;
  }
  
  Connection.emit("status", {enabled: controlEnabled, mode: mode, time: time});
  
  var data;
  if (controlEnabled) {
    var buffer = new Buffer(3 + controllerData.axes.length * 4 + controllerData.buttons.length * 4);
    buffer.writeUInt8(mode == "Auto" ? FRAME_TYPE_AUTO : mode == "Teleop" ? FRAME_TYPE_TELEOP : FRAME_TYPE_PING, 0);
    buffer.writeUInt8(controllerData.axes.length, 1);
    buffer.writeUInt8(controllerData.buttons.length, 2);
    
    // arduino is little endian
    for (var i = 0; i < controllerData.axes.length; i++) {
      buffer.writeFloatLE(controllerData.axes[i], i * 4 + 3);
    }
    for (var i = 0; i < controllerData.buttons.length; i++) {
      buffer.writeFloatLE(controllerData.buttons[i], i * 4 + 3 + controllerData.axes.length * 4);
    }
    console.log("Send buffer", buffer);
    data = xbee.buildFrame({
      type: C.FRAME_TYPE.TX_REQUEST_64,
      destination64: selectedRemote.remote64,
      options: 0x00,
      data: buffer
    });
  } else {
    var buffer = new Buffer(1);
    buffer.writeUInt8(FRAME_TYPE_PING, 0);
    data = xbee.buildFrame({
      type: C.FRAME_TYPE.TX_REQUEST_64,
      destination64: selectedRemote.remote64,
      options: 0x00,
      data: buffer
    });
  }

  chrome.serial.send(connectionId, toArrayBuffer(data), function(stat) {
    onSentCallback(stat);
    clearTimeout(commsInterval);
    commsInterval = setTimeout(communicationLoop, 33 - (new Date().getTime() - now));
  });
}

module.exports = window.Connection = Connection;