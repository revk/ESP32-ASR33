# ESP32 ASR33 controller

Hardware and software for ESP32 based controller for ASR33 teletype.

- 20mA current sources for Tx and Rx 110 Baud.
- WiFi connection (IPv4 and IPv6)
- Simple TCP socket to interface to teletype (inbound and outbound)
- MQTT connection
- Includes stand alone Colossal Cave adventure game

See ASR33.pages for more details

![ASR33](https://user-images.githubusercontent.com/996983/170732167-bb23096c-f055-41e3-bca9-a1ce38849a1d.png)

# Building

Git clone this `--recursive` to get all the submodules, and it should build with just `make`. That actually runs the normal `idf.py` to build. `make menuconfig` can be used to fine tune the settings, but the defaults should be mostly sane. `make flash` should work to program.
