# ESP32_Ergo_rower
ESP32 based Ergo replacement for water rower computer that died from battery failure
-- original Arduino couldn't cope with interrupts and a big display  480x320 tft

ESP32 - dual cores and 240Mhz breathed new life into my attempt.
1 core just handles the interrupts and ergo calculations
the other core deals with the screen and bluetooth - stops the interrupts impacting time critical stuff.

Trying to refactor the code into cleaner separated functions, less global variables, and more 

ERGO handles all events to do with rowing, and stats
Display covers timed updates and notification.






CREDIT due to as follows

Original idea from
https://github.com/dvernooy/ErgWare  - fan based rower. which led me to physics info here
http://eodg.atm.ox.ac.uk/user/dudhia/rowing/physics/ergometer.html

and some nasty water momentum calculations - which have a little arbitrary calculations still to get realistic results.

Garmin connectivity https://github.com/zpukr/ArduRower pushed me to add Bluetooth.
and on to 
Rower side based on sketch WaterRino: https://github.com/adruino-io/waterrino
Bluetooth side on sketch WaterRower S4BL3 Bluetooth BLE for S4: https://github.com/vibr77/wr_s4bl3_samd

