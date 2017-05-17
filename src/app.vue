<template>
    <div class="app-container">
      <div v-bind:class="instructions">
        <div class="modal-dialog" role="document">
          <div class="modal-content">
            <ul class="nav nav-tabs">
              <li role="presentation" v-bind:class="{active:instructionsTab==0}"><a href="#" v-on:click="setInstructionsTab(0)">UART/DLINE Switch</a></li>
              <li role="presentation" v-bind:class="{active:instructionsTab==1}"><a href="#" v-on:click="setInstructionsTab(1)">Arduino Library / Example</a></li>
            </ul>
            <div class="modal-body">
              <p v-if="instructionsTab==0">
                <img src="assets/uart-dline.png" style="float:left;height:200px;margin-right:1em;" />
                The Arduino IDE and wireless module use the same serial bus.<br/>
                <b>When Arduino is connected to the computer:</b> make sure the switch is in the DLINE position. This disconnects the wireless module from the serial bus.<br/>
                <b>When Arduino is on battery power only:</b> make sure the switch is in the UART position. This enables wireless communication.<br/>
                If the switch is in the UART position, uploading code or using the IDE serial monitor won't work!
              </p>
              <div class="code-page" v-if="instructionsTab==1">
                <p>Paste the following into your Codebender file. <a class="copy-button" href="#" v-on:click="copyCode">Copy</a></p>
                <textarea class="code form-control" v-model="libraryCode" readonly></textarea>
              </div>
              <br style="clear:both" />
            </div>
            <div class="modal-footer">
              <button type="button" class="btn btn-primary" v-on:click="hideInstructions">Got it</button>
            </div>
          </div>
        </div>
      </div>
      
      <div v-bind:class="settings">
        <div class="modal-dialog" role="document">
          <div class="modal-content">
            <div class="modal-header">
              Settings
            </div>
            <div class="modal-body">
              <p>Mode:</p>
              <div class="match-type">
                <label class="radio-inline">
                  <input type="radio" name="matchType" v-model="matchType" value="auto"> Autonomous Only
                </label>
                <label class="radio-inline">
                  <input type="radio" name="matchType" v-model="matchType" value="teleop"> Teleop Only
                </label>
                <label class="radio-inline">
                  <input type="radio" name="matchType" v-model="matchType" value="full"> Full Match
                </label>
              </div>
              <br />
              <p>Settings:</p>
              <div class="settings">
                <label for="autoLength">Autonomous time:</label>
                <div class="input-group">
                  <input type="number" class="form-control" id="autoLength" v-model="autoLength" />
                  <span class="input-group-addon">seconds</span>
                </div>
                <label for="teleopLength">Teleop time:</label>
                <div class="input-group">
                  <input type="number" class="form-control" id="teleopLength" v-model="teleopLength" />
                  <span class="input-group-addon">seconds</span>
                </div>
              </div>
            </div>
            <div class="modal-footer">
              <button type="button" class="btn btn-primary" v-on:click="hideSettings">Done</button>
            </div>
          </div>
        </div>
      </div>
      
      <div class="help">
        <button type="button" class="btn btn-default" aria-label="Show Help" v-on:click="showInstructions">
          <span class="glyphicon glyphicon-question-sign" aria-hidden="true"></span> Important information
        </button>
      </div>
    
        <div class="row h100">
          <div class="col-md-4 h100">
            <div class="panel panel-default h100">
              <div class="panel-heading">
                Connection
              </div>
              <div class="panel-body">
                <div class="alert alert-warning" role="alert" v-if="serialStatus == 0">
                  <span class="glyphicon glyphicon-remove" aria-hidden="true"></span>
                  Plug in XBee module
                </div>
                
                <div class="alert alert-info" role="alert" v-if="serialStatus == 2">
                  <span class="glyphicon glyphicon-signal" aria-hidden="true"></span>
                  Initializing XBee...
                </div>
                
                <div class="alert alert-info" role="alert" v-if="serialStatus == 1 && remotes.length == 0 && !remoteSelected">
                  <span class="glyphicon glyphicon-signal" aria-hidden="true"></span>
                  Detecting robots...
                </div>
                
                <p v-if="serialStatus == 1">
                  <ul id="remotes" class="list-group" v-if="!remoteSelected">
                    <li class="list-group-item remote-item" v-for="node in remotes">
                      <div>
                        Name: {{ node.nodeIdentifier }}<br/>
                        Address: {{ node.remote16 }} / {{ node.remote64 }}<br/>
                        Signal strength: -{{ node.signalStrength }} dBm
                      </div>
                      <button type="button" class="btn btn-primary remote-select" v-on:click="connectRemote(node)">Select</button>
                    </li>
                  </ul>
                  <div v-if="!!remoteSelected">
                    <div class="alert alert-warning" role="alert" v-if="!remotePing">
                      <span class="glyphicon glyphicon-remove" aria-hidden="true"></span>
                      Waiting for robot to respond...
                    </div>
                    Connected robot: {{ remoteSelected.nodeIdentifier }} ({{ remoteSelected.remote16 }} / {{ remoteSelected.remote64 }})<br /><br />
                    <button type="button" class="btn btn-primary" v-on:click="disconnectRemote()">Disconnect</button>
                  </div>
                </p>
              </div>
            </div>
          </div>
          <div class="col-md-4 h100">
            <div class="panel panel-default h100">
              <div class="panel-heading">
                Controller
              </div>
              <div class="panel-body">
                <div class="alert alert-warning" role="alert" v-if="!joystickConnected">
                  <span class="glyphicon glyphicon-remove" aria-hidden="true"></span>
                  Plug in a controller and press a button
                </div>
                <p v-if="joystickConnected">
                  Connected controller: {{ selectedJoystick }}
                </p>
                <div class="joystickData" v-if="joystickConnected">
                  <p class="label">Axes</p>
                  <div class="axis" v-for="(axis, index) in joystickData.axes" v-bind:style="createStyle(axis)">
                    {{ axisName(index) }}: {{ axis.toFixed(2) }}
                  </div>
                  <br /><br />
                  <p class="label">Buttons</p>
                  <div class="button" v-for="(button, index) in joystickData.buttons" v-bind:style="createStyle(button)">
                    {{ buttonName(index) }}: {{ button.toFixed(2) }}
                  </div>
                </div>
              </div>
            </div>
          </div>
          <div class="col-md-4 h100">
            <div class="panel panel-default h100">
              <div class="panel-heading">
                Robot
              </div>
              <div class="panel-body teleop-panel">
                <h2 class="status">
                  {{currentMode}}
                  <br />
                  <span class='monospace'>{{formatTime(timeRemaining)}}</span>
                </h2>
                <div class="btn-group btn-group-lg fill-width" role="group" aria-label="Enable or disable">
                  <button type="button" class="btn btn-default fill-width" v-on:click="toggle" v-bind:disabled="cantEnableControl" v-bind:title="cantEnableControl ? 'Connect to the robot and plug in a controller' : '[]\ to enable, ENTER or SPACE to disable'">{{ robotEnabled ? "Disable" : "Enable" }}</button>
                </div>
                <pre class="robot-log" v-html="log"></pre>
                <button type="button" class="btn btn-default settings-btn" aria-label="Show Settings" v-on:click="showSettings">
                  <span class="glyphicon glyphicon-cog" aria-hidden="true"></span> Settings ({{matchType}}, {{matchType=="full"?autoLength+"s, "+teleopLength+"s":matchType=="auto"?autoLength+"s":teleopLength+"s"}})
                </button>
              </div>
            </div>
          </div>
        </div>
    </div>
</template>

<script>
  import Connection from './connection.js';
  import Controller from './controller.js';

  let copyClickTimeout;
  
  export default {
    data () {
      return {
        serialStatus: 0,
        remoteSelected: false,
        remotePing: false,
        remotes: [],
        joystickConnected: false,
        selectedJoystick: null,
        joystickData: { buttons: [], axes: [] },
        robotEnabled: false,
        log: "Robot Control " + chrome.runtime.getManifest().version,
        currentMode: "Disabled",
        timeRemaining: 0,
        instructions: {
          modal: true,
          fade: true,
          'in': true,
          'instructions-dialog': true
        },
        settings: {
          modal: true,
          fade: true,
          'in': false,
          'settings-dialog': true
        },
        instructionsTab: 0,
        libraryCode: "Loading...",
        matchType: "full",
        autoLength: 15,
        teleopLength: 135
      }
    },
    mounted: function() {
      let self = this;
      window.App = self;
      console.log(self);
      Connection.on("connected", function() {
        self.serialStatus = 1;
        self.log += "\nXBee connected";
      });
      Connection.on("disconnected", function() {
        self.serialStatus = 0;
        self.remotes = [];
        self.remoteSelected = false;
        self.robotEnabled = false;
        self.log += "\nXBee disconnected";
      });
      Connection.on("opening_serial", function() {
        self.serialStatus = 2;
        self.log += "\nStarting XBee communication";
      });
      Connection.on("scan", function(remotes) {
        self.remotes = remotes;
      });
      let timeout;
      Connection.on("ping", function() {
        if (self.remoteSelected) {
          self.remotePing = true;
          clearTimeout(timeout);
          timeout = setTimeout(function() {
            self.remotePing = false;
          }, 500);
        }
      });
      Connection.on("log", function(log) {
        self.log += log;
      });
      Connection.on("status", function(status) {
        self.robotEnabled = status.enabled;
        if (self.currentMode != status.mode) {
          self.log += "\nStatus: " + status.mode + "\n";
        }
        self.currentMode = status.mode;
        self.timeRemaining = status.time;
      });
      
      Controller.on("connected", function(name) {
        self.joystickConnected = true;
        self.selectedJoystick = name;
        self.log += "\nConnected controller:\n\t" + name;
      });
      Controller.on("disconnected", function() {
        self.joystickConnected = false;
        self.selectedJoystick = null;
        self.robotEnabled = false;
        self.joystickData = { buttons: [], axes: [] };
        self.log += "\nDisconnected controller";
      });
      Controller.on("data", function(data) {
        self.joystickData = data;
      });
      
      fetch("assets/library.cpp").then(e=>e.text()).then(function(data) {
        self.libraryCode = data;
      });
      
      let enableKeys = [false, false, false];
      let enableTimeout;
      
      document.addEventListener("keydown", function(e) {
        if (e.which == 13 || e.which == 32) {
          self.log += `\n${e.which==13?'Enter':'Spacebar'} pressed, disabling`;
          self.setEnabled(false);
        }
        if (e.which == 219 || e.which == 220 || e.which == 221) {
          enableKeys[e.which - 219] = true;
          clearTimeout(enableTimeout);
          if (enableKeys[0] && enableKeys[1] && enableKeys[2]) {
            if (self.remoteSelected && self.joystickConnected) {
              self.log += `\nEnabling control`;
              self.setEnabled(true);
            } else {
              self.log += `\nCan't enable control, check controller and xbee`;
            }
          }
          enableTimeout = setTimeout(function() {
            enableKeys = [false, false, false];
          }, 200);
        }
      });
    },
    computed: {
      cantEnableControl: function() {
        return !this.remoteSelected || !this.joystickConnected || !this.remotePing;
      }
    },
    methods: {
      connectRemote: function(node){
        Connection.connectRemote(node);
        this.remoteSelected = node;
        this.log += "\nConnected robot:\n\t" + node.nodeIdentifier;
      },
      disconnectRemote: function() {
        Connection.disconnectRemote();
        this.remoteSelected = false;
        this.remotePing = false;
        this.setEnabled(false);
        this.log += "\nDisconnected robot";
      },
      createStyle: function(value) {
        if (value < 0) {
          return `background: rgba(100, 50, 50, ${Math.abs(value)})`;
        } else {
          return `background: rgba(50, 50, 100, ${value})`;
        }
      },
      axisName: function(index) {
        return (["LX", "LY", "RX", "RY"])[index];
      },
      buttonName: function(index) {
        return (["A", "B", "X", "Y", "LB", "RB", "LT", "RT", "BK", "ST", "LS", "RS", "PN", "PS", "PW", "PE", "L"])[index];
      },
      toggle: function() {
        if (this.remoteSelected && this.joystickConnected && !this.robotEnabled) {
          this.setEnabled(true);
        } else {
          this.setEnabled(false);
        }
      },
      showInstructions: function() {
        this.instructions['in']=true;
      },
      hideInstructions: function() {
        this.instructions['in']=false;
      },
      setInstructionsTab: function(tab) {
        this.instructionsTab = tab;
      },
      showSettings: function() {
        this.settings['in'] = true;
      },
      hideSettings: function() {
        this.settings['in'] = false;
      },
      copyCode: function() {
        var textarea = this.$el.querySelector('.code');
        textarea.focus();
        textarea.select();
        var msg = 'Failed to copy';
        try {
          var successful = document.execCommand('copy');
          if (successful) {
            msg = 'Copied!';
          }
        } catch(err) {
          console.log(err);
        }
        let el = this.$el.querySelector(".copy-button");
        el.textContent = msg;
        clearTimeout(copyClickTimeout);
        copyClickTimeout = setTimeout(function() {
          el.textContent = 'Copy';
        }, 5000);
      },
      setEnabled: function(enabled) {
        Connection.setControlEnabled(enabled, this.matchType, this.autoLength, this.teleopLength);
      },
      formatTime: function(time) {
        if (time == 0) {
          return '--:--.---';
        }
        var allSecs = Math.floor(time / 1000);
        var min = Math.floor(allSecs / 60);
        if (min < 10) min = '0' + min;
        var sec = (time / 1000) % 60;
        if (sec < 10) {
          sec = '0' + sec.toFixed(3);
        } else {
          sec = sec.toFixed(3);
        }
        return min + ":" + sec;
      }
    },
    watch: {
      log: function() {
        if (this.log.length > 512) {
          this.log = this.log.substring(this.log.length - 512);
        }
        var container = this.$el.querySelector(".robot-log");
        setTimeout(function() {
          container.scrollTop = container.scrollHeight + 1000;
        }, 100);
      }
    }
  }
</script>
<style>
  html,
  body,
  .h100 {
    height: 100%;
  }
  
  .app-container {
    height: 100%;
    padding: 1em;
  }
  
  .remote-item {
    display: flex;
    flex-direction: row;
    justify-content: space-between;
    flex-wrap: wrap;
  }
  
  .remote-select {
    align-self: flex-start;
  }
  
  .joystick-select {
    width: 100%;
  }
  
  .joystickData .axis, .joystickData .button {
    display: inline-block;
    border: 1px solid gray;
    border-radius: 2px;
    margin-right: 1em;
    margin-bottom: 1em;
    padding: 0.2em;
    width: 3em;
    text-align: center;
  }
  
  .joystickData .label {
    display: block;
    text-align: left;
  }
  
  #gamepad-frame {
    width: 0;
    height: 0;
    border: 0;
  }
  
  .teleop-panel {
    display: flex;
    flex-direction: column;
    height: calc(100% - 2em);
  }
  
  .robot-log {
    flex: 1;
    margin-top: 1em;
    overflow-y: auto;
    word-wrap: break-word;
    white-space: pre-wrap;
  }
  
  .help {
    position: absolute;
    bottom: 0.5em;
    left: 0.5em;
    z-index: 1000;
  }
  
  .modal.fade {
    pointer-events: none;
    display: block;
    background: transparent;
    transition: opacity 0.3s;
  }
  
  .modal.fade.in {
    background: rgba(0,0,0,0.5);
    pointer-events: all;
  }
  
  .code {
    font-family: monospace;
    flex: 1;
    resize: none;
  }
  
  .form-control.code[readonly] {
    background: #fff;
  }
  
  .code-page {
    display: flex;
    flex-direction: column;
    height: 300px;
  }
  
  .modal.fade .modal-dialog {
    transition: transform 0.3s ease-out;
  }
  
  .modal.fade.settings-dialog .modal-dialog {
    transform: translate(10%, 70%) scale(0.7);
  }
  
  .modal.fade.instructions-dialog .modal-dialog {
    transform: translate(-10%, 70%) scale(0.7);
    width: 90%;
  }
  
  .modal.fade.in.settings-dialog .modal-dialog,
  .modal.fade.in.instructions-dialog .modal-dialog  {
    transform: translate(0, 0) scale(1);
  }
  
  .monospace {
    font-family: monospace;
  }
  
  .fill-width {
    width: 100%;
  }
  
  .settings-btn {
    margin-bottom: 0.5em;
  }
  
  .status {
    margin-top: 0;
  }
</style>