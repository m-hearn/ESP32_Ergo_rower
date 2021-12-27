
/* 
// 40mhz
// TFT_eSPI library test!
// Benchmark                Time (microseconds)
// Screen fill              502035
// Text                     26568
// Lines                    246273
// Horiz/Vert Lines         42505
// Rectangles (outline)     23706
// Rectangles (filled)      1212994
// Circles (filled)         144902
// Circles (outline)        93021
// Triangles (outline)      48744
// Triangles (filled)       410212
// Rounded rects (outline)  45503
// Rounded rects (filled)   1228545
// Done!

// https://www.messletters.com/en/big-text/  thick
*/

#include "SPI.h"
#include "TFT_eSPI.h"


void setup_rower();
void calc_rower_stroke();
void start_rower();
void stop_rower();

void setup_display(); // setup - called from main setup
void handle_display( void *param); // display and touch handler
void draw_elements(); // draw main elements - call subdisplays
void touch_events(); // read and process touch events


#include "globals.h"
#include "ergo.h"
#include "display.h"

/*
                                                                                    
 ad88888ba                                                                          
d8"     "8b    ,d                                                                   
Y8,            88                                                                   
`Y8aaaaa,    MM88MMM   ,adPPYba,   8b,dPPYba,  ,adPPYYba,   ,adPPYb,d8   ,adPPYba,  
  `"""""8b,    88     a8"     "8a  88P'   "Y8  ""     `Y8  a8"    `Y88  a8P_____88  
        `8b    88     8b       d8  88          ,adPPPPP88  8b       88  8PP"""""""  
Y8a     a8P    88,    "8a,   ,a8"  88          88,    ,88  "8a,   ,d88  "8b,   ,aa  
 "Y88888P"     "Y888   `"YbbdP"'   88          `"8bbdP"Y8   `"YbbdP"Y8   `"Ybbd8"'  
                                                            aa,    ,88              
                                                             "Y8bbdP" 
 */

// #include "FS.h"
// #include "SPIFFS.h"

// void setup_storage(){
//   // check file system exists
//   if (!SPIFFS.begin()) {
//     Serial.println("Formating file system");
//     SPIFFS.format();
//     SPIFFS.begin();
//   }

//   Serial.println(SPIFFS.totalBytes());
//   Serial.println(SPIFFS.usedBytes());
// }



/*
 w                    8                                  w        
w8ww .d8b. 8   8 .d8b 8d8b.    .d88b Yb  dP .d88b 8d8b. w8ww d88b 
 8   8' .8 8b d8 8    8P Y8    8.dP'  YbdP  8.dP' 8P Y8  8   `Yb. 
 Y8P `Y8P' `Y8P8 `Y8P 8   8    `Y88P   YP   `Y88P 8   8  Y8P Y88P 
                                                                   */
// void touch_events(){
//   return;
// };

/*
88b           d88                                                   
888b         d888                                                   
88`8b       d8'88                                                   
88 `8b     d8' 88   ,adPPYba,  8b,dPPYba,   88       88  ,adPPYba,  
88  `8b   d8'  88  a8P_____88  88P'   `"8a  88       88  I8[    ""  
88   `8b d8'   88  8PP"""""""  88       88  88       88   `"Y8ba,   
88    `888'    88  "8b,   ,aa  88       88  "8a,   ,a88  aa    ]8I  
88     `8'     88   `"Ybbd8"'  88       88   `"YbbdP'Y8  `"YbbdP"'  
*/


/*
888888888888  88                                  
     88       ""                                  
     88                                           
     88       88  88,dPYba,,adPYba,    ,adPPYba,  
     88       88  88P'   "88"    "8a  a8P_____88  
     88       88  88      88      88  8PP"""""""  
     88       88  88      88      88  "8b,   ,aa  
     88       88  88      88      88   `"Ybbd8"'  
                                                  
*/
// #include <WiFi.h>
// #include "time.h"

// // Replace with your network credentials
// const char* ssid = "SPACE";
// const char* password = "feedbac123";

// // NTP server to request epoch time
// const char* ntpServer = "pool.ntp.org";

// // Variable to save current epoch time
// unsigned long epochTime;

// // Function that gets current epoch time
// unsigned long getTime() {
//   time_t now;
//   struct tm timeinfo;
//   if (!getLocalTime(&timeinfo)) {
//     //Serial.println("Failed to obtain time");
//     return(0);
//   }
//   time(&now);
//   return now;
// }

// void setup_time(void *param) {
//   /* Init WiFi */
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(ssid, password);
//   Serial.print("Connecting to WiFi ..");
//   while (WiFi.status() != WL_CONNECTED) {
//     Serial.print('.');
//     delay(1000);
//   }
//   Serial.println(WiFi.localIP());
//   configTime(0, 0, ntpServer);
//   epochTime = getTime();
  
//   //disconnect WiFi as it's no longer needed
//   WiFi.disconnect(true);
//   WiFi.mode(WIFI_OFF);
// }



/*
88b           d88              88                   88                                         
888b         d888              ""                   88                                         
88`8b       d8'88                                   88                                         
88 `8b     d8' 88  ,adPPYYba,  88  8b,dPPYba,       88   ,adPPYba,    ,adPPYba,   8b,dPPYba,   
88  `8b   d8'  88  ""     `Y8  88  88P'   `"8a      88  a8"     "8a  a8"     "8a  88P'    "8a  
88   `8b d8'   88  ,adPPPPP88  88  88       88      88  8b       d8  8b       d8  88       d8  
88    `888'    88  88,    ,88  88  88       88      88  "8a,   ,a8"  "8a,   ,a8"  88b,   ,a8"  
88     `8'     88  `"8bbdP"Y8  88  88       88      88   `"YbbdP"'    `"YbbdP"'   88`YbbdP"'   
                                                                                  88           
                                                                                  88    
 */

#define DEBUG_PIN 12


void setup() {
  // TaskHandle_t Time_t_handle;
  
#ifdef ARDUINO
  pinMode(DEBUG_PIN, INPUT_PULLUP);
  DEBUG = !(digitalRead(DEBUG_PIN));
#endif

  Serial.begin(250000);
  while (!Serial);

  DEBUG= !(digitalRead(DEBUG_PIN));
  // DEBUG = 1;
  
  if(!DEBUG)
    Serial.printf("T,S#,St,dt,R,W,Wd,Wdd,C,damp,SPM,Watts,D,force\n");  

  //setup_storage();
  
  // xTaskCreatePinnedToCore(setup_time, "time", 1000, NULL, 0, &Time_t_handle, DISPLAY_CPU);

  setup_display();

  setup_rower();
}

int debug_index = 0;


void loop(void) {
  t_real = millis();

  if ((DEBUG) && (t_real >= erg_sim[debug_index])) {
    row_buff_head = (row_buff_head+1)&ROW_BUFF_SIZE;
    row_buffer[row_buff_head] = erg_sim[debug_index++];
  }

  while (row_buff_head != row_buff_tail) {
    row_buff_tail = (row_buff_tail+1)&ROW_BUFF_SIZE; // move the pointer on.

    if (!rowing || paused) {  // not started or paused
      t_last=t_real;          // set a recent start time for strokes etc.  (assume pause only occurs in decay)
      if (!rowing) start_rower(); // setup rower state and begin
      else paused = 0;
      t_clock = t_real;
      t_power = t_last;
      // don't set a next stroke or calculate - this was the first interrupt that got us here.
    } else { // already rowing - 
      t_now = row_buffer[row_buff_tail];
      calc_rower_stroke(); // for t_now  (compare against t_last)
    }
  }

  delay(10);

  if ((t_real - t_power) > 5 Seconds) { paused = rowing;}  // can't be paused and rowing!

  if (rowing && !paused) {
    // 27 2:14 2:13 252   230 0:01:01.1
    //  123456789012345678901234567890
    if ((t_real - t_clock) > 0.1 Seconds) {
      t_clock += 100;
      row_elaspsed+=0.1;
      row_secs    +=0.1;
      powergraph_scroll();

      if (row_secs >= 60) {
        stats_disp[27]='X'; // make the display think that drawing a : is necessary - because it's different
        row_secs-=60.0;
        if (++row_minutes > 60) {
          stats_disp[24]='X';
          row_hours++;
          row_minutes -= 60;
          if (row_hours > 9) row_hours = 0;
          curr_stat.row_mins = row_minutes;
          curr_stat.row_hrs = row_hours;
        }
      }
      sprintf(stats_curr+17,"%5.0f %1d:%02d:%04.1f", distance_rowed, row_hours, row_minutes, row_secs);
      sprintf(curr_stat.distance,"%5.0f", distance_rowed);
      sprintf(curr_stat.row_secs,"%04.1f", row_secs);
    }
  }
};
