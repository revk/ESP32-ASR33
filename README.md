# ESP32 ASR33 controller

Direct control of an ASR33 and connection to MQTT

This uses ESP32-RevK and so has the commands and settings as per that library
https://github.com/revk/ESP32-RevK

The ESP is intended to control the teletype power, so it can turn on, print something and
turn off. This is done when told to print via MQTT, or if pressing a button (mine has a
MOTOR START button). It can also be told to stay on by MQTT. When on it does local echo as
well (which can be disabled via MQTT). The actual power control can be via MQTT using, say, 
a sonoff and tasmota, but can also be done from a direct GPIO. If you have direct control
of the motor separate from power, that allows motor start and stop whilst holding the selector
solenoid and hence avoiding a gash character print when power goes on or off.

There are some other features and settings too.

Additional settings, e.g. setting/ASR33/[name]/[setting] [value]
- uart:	UART number
- tx:	Tx pin
- rx:	Rx pin
- pu:   Extra pulled up input pin, if extra pull up needed on rx, put in parallel to rx
- on:	On pin (a low on this wakes up)
- power: Output pin (low=active) to directly control power
- motor: Output pin (low=active) to directly control motor
- sonoff:	The topic to use for power on/off the teletype
- idle:	The number of seconds before powering off after printing done
- keyidle: The number of seconds before powering off in manual mode (i.e. on function not just printing)
- echo:	If echo on or off
- wru:	String to send for WRU
- ver:	Flag, if set, send version info for WRU
- ack:  Value of ACK - 0x7C in 1963, 0x06 in later editions
- cave: Flag, always play a game
- nodc4: ASR33 does not do DC2/DC4 so don't send to start/stop tape

Commands, e.g. command/ASR33/[name]/[command]
- on: Turn on now (automatically done if message to send)
- off: Turn off now (automatically done after idle delay)
- tx:	Raw 8 bit data to send to teletype
- text:	Text, adding carriage returns and setting even parity, and skipping utf-8 (print up arrow)
- punch: Data to punch, handles DC2/DC4 logic, with DC2/blanks and blanks/DC4 at ends
- punchraw: Punch,handling of any embedded DC2/DC4 but no blanks/DC2/DC4 at ends
- tape: Punch 5 hole lettering, with DC2/blanks and blanks/DC4 at ends
- taperaw: Punch 5 hole lettering, without blanks/DC2/DC4
- cave: Let's play a game

Status, e.g. status/ASR33/[name]/[status]
- power: If power on or off
- busy: If tx queue is getting full - don't send it busy

Events, e.g. event/ASR33/[name]/[event]
- rx:	Raw 8 bit data from teletype (one character at a time)
- line:	Text typed, parity stripped, a whole line of typed text

Can be connected using a 100 ohm resistor directly
(see https://www.revk.uk/2020/12/connecting-asr33-teletype-to-esp32.html)

Note:
- Typing WRU (^R) will response with wru string and version if either is configured/set
- Typing RU (^F) will run Colossal Cave - have fun

Copyright © 2020 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0
