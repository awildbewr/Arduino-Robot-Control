chrome.app.runtime.onLaunched.addListener(function() {
  chrome.app.window.create('index.html', {
    id: "ds",
    bounds: {
      width: 1200,
      height: 600
    }
  });
});
