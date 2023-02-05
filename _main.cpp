
#include "SPI.h"
#include "TFT_eSPI.h"

#include "ble.h"
#include "ergo.h"
#include "display.h"

void setup_rower();
void ready_rower();
void stop_rower();

#include "globals.h"
// global definitions

struct stats curr_stat;
struct analysis curr_score, aver_score, last_score;

int DEBUG = 0;
int rowing =0;

int stroke_len = 0;
int fg_N[MAX_STROKE_LEN];
double fg_dy[MAX_STROKE_LEN];
double fg_N_sum;
double fg_N_cnt;

char stopwatch[8]; // mssmihX
char stopwatch_disp[8]; // hmmssm but backwards
char stopwatch_max[] = "995959:";
void stopwatch_reset();
void stopwatch_inc();

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




#define DEBUG_PIN 12

unsigned long fake_intr = 0;
unsigned long t_now, t_last;
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

  aver_score.f_eff = 5.0;
  aver_score.b_eff = 5.0;
  aver_score.spread = 5.0;
  aver_score.offset = 0.0;

  stopwatch_reset();

  t_last = millis();
  fake_intr = t_last + erg_sim[0];
}

void stopwatch_reset(){
  int c;

  for(c=0;c<8;c++) {
    stopwatch[c]='0';
    stopwatch_disp[c]='9';
  }
  stopwatch_disp[4]='0'; // don't show tens and hours
  stopwatch_disp[5]='0';
  stopwatch[6]=':';
}

void stopwatch_inc(){
  stopwatch[0]++;
}


                                                               
//  #    #    ##    #  #    #      #        ####    ####   #####  
//  ##  ##   #  #   #  ##   #      #       #    #  #    #  #    # 
//  # ## #  #    #  #  # #  #      #       #    #  #    #  #    # 
//  #    #  ######  #  #  # #      #       #    #  #    #  #####  
//  #    #  #    #  #  #   ##      #       #    #  #    #  #      
//  #    #  #    #  #  #    #      ######   ####    ####   #      

int tic = 0;

void loop(void) {
  unsigned long t_now = millis();

  process_interrupts();

  if ((t_now - t_last) >= 100) {
    t_last += 100;

    if (rowing) check_slow_down(t_now);

    if (rowing) {
      stopwatch_inc();

      curr_stat.elapsed+=0.1;

      // row_secs         +=0.1;
      // if (row_secs >= 60) {
      //   stats_disp[27]='X'; // make the display think that drawing a : is necessary - because it's different
      //   row_secs-=60.0;
      //   if (++row_minutes > 60) {
      //     stats_disp[24]='X';
      //     row_hours++;
      //     row_minutes -= 60;
      //     if (row_hours > 9)
      //       row_hours = 0;
      //   }
      // }
      // sprintf(stats_curr+17,"%05.0f", curr_stat.distance);
    }
  }
  delay(50); // avoid busy wait?!

  // If we're debugging - add some fake interrupts into the queue.
  if ((DEBUG) && (t_now >= fake_intr)) {
    push_interrupt(fake_intr);
    fake_intr += erg_sim[pErg_sim++];
    if (erg_sim[pErg_sim]==999999)
      pErg_sim=0;
  }
};

//   #####                                       #  #######                 
//  #     #  #####    ##    #####   #####       #   #        #    #  #####  
//  #          #     #  #   #    #    #        #    #        ##   #  #    # 
//   #####     #    #    #  #    #    #       #     #####    # #  #  #    # 
//        #    #    ######  #####     #      #      #        #  # #  #    # 
//  #     #    #    #    #  #   #     #     #       #        #   ##  #    # 
//   #####     #    #    #  #    #    #    #        #######  #    #  ##### 

void start_pull() {
  force_graph_ready();

  stroke_len = 0;
  fg_N_sum = 0;
  fg_N_cnt = 0;
};

void end_pull(){
  update_stats(2);
  stroke_analysis();
};

void record_force(int force) {
  double dy;

  // check bounds
  if (stroke_len == MAX_STROKE_LEN) return;
  if (force < 0) force = 0;

  force_graph_plot(force);

  fg_N[stroke_len]= force;

  // do some analysis
  // Doesn't diff the first tick - it's most likely negative gradient - so okay to ignore
  if (stroke_len > 1) {
    if (force) {
      fg_dy[stroke_len-1] = (double)(fg_N[stroke_len-1]) - (double)(fg_N[stroke_len-2]+fg_N[stroke_len])/2.0;
    }
    else fg_dy[stroke_len-1] = 0;
  }
  fg_N_sum += (stroke_len+1) * force;
  fg_N_cnt += force;
  
  stroke_len++;
}



//   #####                                               #####                                  
//  #     #  #####  #####    ####   #    #  ######      #     #   ####    ####   #####   ###### 
//  #          #    #    #  #    #  #   #   #           #        #    #  #    #  #    #  #      
//   #####     #    #    #  #    #  ####    #####        #####   #       #    #  #    #  #####  
//        #    #    #####   #    #  #  #    #                 #  #       #    #  #####   #      
//  #     #    #    #   #   #    #  #   #   #           #     #  #    #  #    #  #   #   #      
//   #####     #    #    #   ####   #    #  ######       #####    ####    ####   #    #  ###### 

void stroke_analysis(){
  int i,fx,fy,bx,by;
  // do the analysis here
  double stroke_mid = stroke_len / 2.0;
  double stdev = 0.0;  // somewhere between 14 and 20?  17.5 maybe good for smooth
  double mean  = 0.0;  // this wants to be close to stroke_len / 2
  double effic_fp = 0.0;
  double effic_fn = 0.0;
  double effic_bp = 0.0;
  double effic_bn = 0.0;
  double f_eff, b_eff;
  double offset;

  
  mean = fg_N_sum / fg_N_cnt;

  // front half of the stroke
  for (i=0;i<=mean;i++){
    stdev += (mean - (double)(i+1.0))*(mean - (double)(i+1.0))*(double)fg_N[i];
    if (fg_dy[i]< 0.0) 
      effic_fn -= fg_dy[i]; 
    else 
      effic_fp += fg_dy[i];
  }
  // first dy already excluded
  if(fg_dy[1]<0) effic_fn+=fg_dy[1];
  
  // back half of the stroke
  for (;(i<MAX_STROKE_LEN) && (fg_N[i]);i++){
    stdev += (mean - (double)(i+1.0))*(mean - (double)(i+1.0))*(double)fg_N[i];
    if (fg_dy[i]< 0.0) 
      effic_bn -= fg_dy[i]; 
    else 
      effic_bp += fg_dy[i];
    
  }
  // don't penalise ramp in / ramp out - a bit.
  if(i>1 && fg_dy[i-2]<0) {
    effic_bn+=fg_dy[i-2];
    if(i>2 && fg_dy[i-3]<0) effic_bn+=fg_dy[i-3];
  }
  
  stdev = stdev / fg_N_cnt;
  stdev -= 17; // max is about 26/27
  if (stdev < 1) stdev = 1;  // range 1 to 10

  f_eff = (1+effic_fp)/(1+effic_fn)   - 2;
  b_eff = (1+effic_bp)/(1+effic_bn)*4 - 4;
  
  if (f_eff>9.9) f_eff = 9.9;
  if (b_eff>9.9) b_eff = 9.9;
  if (f_eff<0.1) f_eff = 0.1;
  if (b_eff<0.1) b_eff = 0.1;

  offset = mean - stroke_mid;  // -ve  front load +ve back load 

  // Tuning params - debug  
  // for (i=0;i<MAX_STROKE_LEN;i++) Serial.printf("%d,",fg_N[i]);
  // for (i=0;i<MAX_STROKE_LEN;i++) Serial.printf("%4.2f,",fg_dy[i]);
  // Serial.printf("%3.3f,%2.2f,%4.1f,%4.1f,%2.1f,%4.1f,%4.1f,%2.1f,%3.1f\n", mean, stroke_mid, effic_fn, effic_fp, effic_bn, effic_bp, f_eff, b_eff, stdev);
  // Serial.printf("%3.1f, %3.1f, %3.1f, %2.1f, %3.1f, %3.1f, %3.1f, %2.1f\n", f_eff, b_eff, stdev, offset, af_eff, ab_eff, astdev, aoffst);

  last_score.f_eff = aver_score.f_eff;  
  last_score.b_eff = aver_score.b_eff;  
  last_score.spread = aver_score.spread;  
  last_score.offset = aver_score.offset;

  curr_score.f_eff  = f_eff;
  curr_score.b_eff  = b_eff;
  curr_score.spread = stdev;
  curr_score.offset = offset;

  // Moving average code slight weighting towards good results
  // aver_score.f_eff = (aver_score.f_eff*(9-f_eff)+(1+f_eff)*f_eff)/10.0;
  // aver_score.b_eff = (aver_score.b_eff*(9-curr_score.b_eff)+(1+curr_score.b_eff)*curr_score.b_eff)/10.0;
  // aver_score.spread = (aver_score.spread*(9-stdev)+(1+stdev)*stdev)/10.0;
  aver_score.f_eff = (aver_score.f_eff*(190-curr_score.f_eff)+(10+curr_score.f_eff)*curr_score.f_eff)/200.0;
  aver_score.b_eff = (aver_score.b_eff*(190-curr_score.b_eff)+(10+curr_score.b_eff)*curr_score.b_eff)/200.0;
  aver_score.spread = (aver_score.spread*(190-curr_score.spread)+(10+curr_score.spread)*curr_score.spread)/200.0;
  aver_score.offset = (aver_score.offset*9+curr_score.offset)/10;

  stroke_score_plot();
}

// https://www.messletters.com/en/big-text/  thick
//#-DTFT_INVERSION_ON=1 #  turn on for the 3" 320x240 version in platformio.ini

