# Arduino Robot Control #

This project enables remote control of Arduino-based robots using standard gamepads and wireless XBee (series 1) modules. Chromebooks are supported as remote controllers. It can run a full FRC-style robot match with autonomous and teleoperated modes. It's designed for use with the Logitech F310 but it would likely work with other gamepads with a few modifications. Inspired by the FRC driver station.

![Chrome app screenshot](docs/screenshot.png)

There are two parts:
- A chrome app for remote control
- An Arduino library for recieving remote commands

## Further documentation ##

See docs here: https://erikuhlmann.github.io/Arduino-Robot-Control/

## Building ##

Clone the repo and `npm install`. Then `npm run build`. Load `dist/` as an unpacked Chrome app.