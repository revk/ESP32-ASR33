# ESP32 ASR33 controller

Direct control of an ASR33 and connection to MQTT

This uses ESP32-RevK and so has the commands and settings as per that library
https://github.com/revk/ESP32-RevK

Additional settings:-
- uart	UART number
tx	Tx pin
rx	Rx pin
sonoff	The topic to use for power on/off the teletype
idle	The number of seconds before powering off (allow time to print)
TODO
echo	If echo on or off

Commands :-
tx	Raw 8 bit data to send to teletype
TODO
text	Text, adding carriage returns and setting even parity, and skipping utf-8 (print up arrow)

Status :-
rx	Raw 8 bit data from teletype (one character at a time)
TODO
line	Text typed, parity stripped, a whole line of typed text

Copyright © 2019 Adrian Kennard, Andrews & Arnold Ltd. See LICENCE file for details. GPL 3.0
