const EventEmitter = require('events').EventEmitter;


var Controller = new EventEmitter();
Controller.data = { buttons: [], axes: [] };

// need an iframe because of a chrome packaged app bug with gamepads
var iframeCode = `console.log("Controller init");
var selectedGamepad = -1;
var updates = [];

window.addEventListener("message", function(event) {
  event.source.postMessage(updates, "*");
  updates = [];
});

window.addEventListener("gamepadconnected", pollControllers);
window.addEventListener("gamepaddisconnected", pollControllers);

function pollData() {
  if (selectedGamepad < 0) {
    requestAnimationFrame(pollData);
    return;
  }
  var gp = navigator.getGamepads()[selectedGamepad];
  if (!checkGamepad(gp)) {
    updates.push({type:"disconnected"});
    console.log("Disconnected");
    selectedGamepad = -1;
    requestAnimationFrame(pollData);
    return;
  }
  
  var buttons = [];
  for (var i = 0; i < gp.buttons.length; i++) {
    buttons[i] = gp.buttons[i].value;
  }
  var data = {axes: gp.axes, buttons: buttons};
  updates.push({type:"data", data: data});
  requestAnimationFrame(pollData);
};
requestAnimationFrame(pollData);

function checkGamepad(gp) {
  return gp instanceof Gamepad && gp.connected && gp.id.toLowerCase().indexOf("unknown") < 0;
}

var pollTimeout;
function pollControllers() {
  clearTimeout(pollTimeout);
  if (selectedGamepad > -1 && checkGamepad(navigator.getGamepads()[selectedGamepad])) {
    pollTimeout = setTimeout(pollControllers, 1000);
    return;
  }
  var gps = navigator.getGamepads();
  for (var i = 0; i < gps.length; i++) {
    if (checkGamepad(gps[i])) {
      selectedGamepad = i;
      updates.push({type:"connected", data: gps[i].id});
      console.log("Connected");
      pollTimeout = setTimeout(pollControllers, 1000);
      return;
    }
  }
  if (selectedGamepad > -1) {
    updates.push({type:"disconnected", data: {}});
    console.log("Disconnected");
  }
  selectedGamepad = -1;
  pollTimeout = setTimeout(pollControllers, 100);
}

pollControllers();`;

var ifr = document.querySelector("#gamepad-frame");
ifr.setAttribute("src", "data:text/html;base64," + btoa("<script>" + iframeCode + "\n</script>"));

function ping() {
  try {
    ifr.contentWindow.postMessage("ping", "*");
  } catch(e) {}
  requestAnimationFrame(ping);
}
requestAnimationFrame(ping);

window.addEventListener("message", function(evt) {
  var datas = evt.data;
  for (let data of datas) {
    if (data.type == "data") {
      Controller.data = data.data;
    }
    Controller.emit(data.type, data.data);
  }
});

module.exports = window.Controller = Controller;