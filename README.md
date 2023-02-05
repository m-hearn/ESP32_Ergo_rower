# ESP32_Ergo_rower
ESP32 based Ergo replacement for water rower computer that died from battery failure
-- original Arduino couldn't cope with interrupts and a big display  480x320 tft

ESP32 - dual cores and 240Mhz breathed new life into my attempt.
1 core just handles the interrupts and ergo calculations
the other core deals with the screen and bluetooth - stops the interrupts impacting time critical stuff.

Trying to refactor the code into cleaner separated functions, less global variables, and more 

ERGO handles all events to do with rowing, and stats
Display covers timed updates and notification.


Display                 main                        ergo                    BLE

setup_display           setup                       setup_rower             setup_ble
                                                    ready_rower

loop(0)                 loop(1)                     
  call drawX               call ->                  process_interrupts      send_ble
  call send BLE

force_graph             start_pull                  start_rower             check_ble
powergraph              end_pull                    stop_rower
elements                record_force                calc_power_stroke
stroke_score            stroke_analysis             check_slow_down*      

                        stop_watch?

display calls send_BLE, from processor 0 and avoids time critical issues between drawing and BLE

*not working




CREDIT due to as follows

Original idea from
https://github.com/dvernooy/ErgWare  - fan based rower. which led me to physics info here
http://eodg.atm.ox.ac.uk/user/dudhia/rowing/physics/ergometer.html

and some nasty water momentum calculations - which have a little arbitrary calculations still to get realistic results.

Garmin connectivity https://github.com/zpukr/ArduRower pushed me to add Bluetooth.
and on to 
Rower side based on sketch WaterRino: https://github.com/adruino-io/waterrino
Bluetooth side on sketch WaterRower S4BL3 Bluetooth BLE for S4: https://github.com/vibr77/wr_s4bl3_samd




; PlatformIO Project Configuration File
;
;   Build options: build flags, source filter
;   Upload options: custom upload port, speed and extra flags
;   Library options: dependencies, extra library storages
;   Advanced options: extra scripting
;
; Please visit documentation for the other options and examples
; https://docs.platformio.org/page/projectconf.html

[platformio]
default_envs = release

[env:release]
platform = espressif32
board = nodemcu-32s
framework = arduino
monitor_speed = 250000
lib_deps = 
	bodmer/TFT_eSPI@^2.3.84
	;h2zero/NimBLE-Arduino@^1.4.1
build_flags = 
	;-DBLE=1
	-DUSER_SETUP_LOADED=1
	-DILI9488_DRIVER=1
	-DTFT_BL=32
	-DTFT_BACKLIGHT_ON=HIGH
	-DTFT_MISO=19
	-DTFT_MOSI=23
	-DTFT_SCLK=18
	-DTFT_RST=4
	-DTFT_CS=15
	-DTFT_DC=2
	-DTOUCH_CS=21
	-DLOAD_GLCD=1
	-DLOAD_FONT2=1
	-DLOAD_FONT4=1
	-DLOAD_GFXFF=1
	-DSMOOTH_FONT=1
	-DSPI_FREQUENCY=60000000