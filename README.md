# dpm8600-esp32-mqtt
Use an ESP32 to control a JT-DPM8600 DC/DC converter using MQTT.

Only use it if you know what you are doing!
Consider it to be a quick and dirty hack. It might damage your DP8600.

Idea ist Based on [@Lotiq's](https://github.com/Lotiq) library: https://github.com/Lotiq/DPM8600 and
[@d4rken's](https://github.com/d4rken) Project: https://github.com/d4rken/jt-dpm8600-psu-mqtt

## Instructions

* Rename `Config.h.sample` to `Config.h`
* Enter your WiFi credentials
* Enter the address and data of the MQTT broker in your local WiFi
* Open the project with VSCode+PlatformIO and flash it to your ESP32
