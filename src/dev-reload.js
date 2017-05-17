function registerReloader() {
  var updateSocket = new WebSocket("ws://localhost:1234");
  updateSocket.onclose = function(){
    setTimeout(registerReloader, 5000);
  };
  updateSocket.onmessage = function (event) {
    if (event.data == "reload") chrome.runtime.reload();
  }
}
console.info("Dev reload active");
registerReloader();