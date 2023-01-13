
#include "SPI.h"
#include "TFT_eSPI.h"



void setup_rower();


void ready_rower();
void stop_rower();

void process_interrupts();
void push_interrupt(unsigned long t_intr); // for debuggingg

void setup_display(); // setup - called from main setup
void handle_display( void *param); // display and touch handler

void touch_events(); // read and process touch events


#include "globals.h"

#include "erg_debug.h"

#include "ble.h"
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

unsigned long fake_intr = 0;
int pErg_sim = 0;

void setup() {
// TaskHandle_t Time_t_handle;
  
#ifdef ARDUINO
  pinMode(DEBUG_PIN, INPUT_PULLUP);
  DEBUG = !(digitalRead(DEBUG_PIN));
#endif

  Serial.begin(250000);
  while (!Serial);
  
  if(!DEBUG)
    Serial.printf("T,S#,St,dt,R,W,Wd,Wdd,C,damp,SPM,Watts,D,force\n");
  
  //setup_storage();
  
  // xTaskCreatePinnedToCore(setup_time, "time", 1000, NULL, 0, &Time_t_handle, DISPLAY_CPU);

#ifdef BLE
  setup_BLE();
#endif
  setup_display();
  setup_rower();

  // auto ready for rowing to begin
  ready_rower();

  t_clock = millis();
  fake_intr = t_clock + erg_sim[0];
}



int tic = 0;

void loop(void) {
  t_real = millis();

  if ((DEBUG) && (t_real >= fake_intr)) {
    push_interrupt(fake_intr);
    fake_intr += erg_sim[pErg_sim++];
    if (erg_sim[pErg_sim]==999999)
      pErg_sim=0;
  }

  process_interrupts();

  // Stopped for > 5 seconds - assume stop rowing - measure from power - as stroke only ends on next pull
  if (rowing) check_slow_down(t_real);

  delay(50); // avoid busy wait?!
};



// https://www.messletters.com/en/big-text/  thick
//#-DTFT_INVERSION_ON=1 #  turn on for the 3" 320x240 version in platformio.ini

