#ifdef ARDUINO
#include "SPI.h"
#include "TFT_eSPI.h"
#include "esp_timer.h"
#else
#include <stdio.h>
#include <time.h>
#include <cstdlib>
#include <stdlib.h>
#include <unistd.h>

// technically 1000 - but we're speeding up time
#define SPEEDUP 100
#define delay(X) usleep(1000*X/SPEEDUP)

unsigned long esp_timer_get_time()
{
    struct timespec now;
    timespec_get(&now, TIME_UTC);
    return (unsigned long) (now.tv_sec * 1000000 * SPEEDUP + (now.tv_nsec / 1000 * SPEEDUP));
    // technically 1000  and 1000000 - but we're speeding up time!
}

void force_graph_ready() {
  return;
};
void update_stats(int l){
  return;
}
void force_graph_plot(int i) {
  return;
}
void stroke_score_plot(){
  return;
}

#endif

#include "ble.h"
#include "ergo.h"
#include "display.h"

extern void setup_ota();

void setup_rower();
void ready_rower();
void stop_rower();

#include "globals.h"
// global definitions

struct rowstat stats;
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

#include "FS.h"
#include "SPIFFS.h"

void setup_storage(){
  // check file system exists
  if (!SPIFFS.begin()) {
    Serial.println("Formating file system");
    SPIFFS.format();
    SPIFFS.begin();
  }

  Serial.println(SPIFFS.totalBytes());
  Serial.println(SPIFFS.usedBytes());
}

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
#include "time.h"

// NTP server to request epoch time
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

// Variable to save current epoch time
time_t epochTime;
char dtime[12];

// Function that gets current epoch time
unsigned long getTime() {
  time_t now;
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    printf("Failed to obtain time\n");
    return(0);
  }
  time(&now);
  strftime(dtime,11,"%y%m%d%H%M",&timeinfo);
  return now;
}


void setup_time() {
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  epochTime = getTime();
  printf("%ld\n",epochTime);
  printf("%s\n",dtime);
}

#define DEBUG_PIN 12

unsigned long fake_intr = 0;
unsigned long t_now, t_last;
int pErg_sim = 0;

void setup() {
// TaskHandle_t Time_t_handle;
  
#ifdef ARDUINO
  pinMode(DEBUG_PIN, INPUT_PULLUP);


  Serial.begin(250000);
  while (!Serial);
#endif

  if (DEBUG==2) {
    printf("T, S, Tic, dT,");
    printf("WddScr, Wdd,Wd,W, ");
    printf("Cal, Kdamp, KDE,");
    printf("Sv,Se,St,");
    printf("J, K, FN, W,");
    printf("D\n");
  }
  
  //setup_storage();
  
  // xTaskCreatePinnedToCore(setup_time, "time", 1000, NULL, 0, &Time_t_handle, DISPLAY_CPU);


#ifdef ARDUINO
  setup_display();
  setup_ota();
  setup_time();
  setup_storage();
#ifdef BLE
  setup_BLE();
#endif
  DEBUG = !(digitalRead(DEBUG_PIN));
#endif
  setup_rower();

  // auto ready for rowing to begin
  ready_rower();

  aver_score.f_eff = 5.0;
  aver_score.b_eff = 5.0;
  aver_score.spread = 5.0;
  aver_score.offset = 0.0;

  stopwatch_reset();

  t_last = esp_timer_get_time();
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


void wifi_mode(){
  stop_rower(); //turn off interrupts
}
                                                               
//  #    #    ##    #  #    #      #        ####    ####   #####  
//  ##  ##   #  #   #  ##   #      #       #    #  #    #  #    # 
//  # ## #  #    #  #  # #  #      #       #    #  #    #  #    # 
//  #    #  ######  #  #  # #      #       #    #  #    #  #####  
//  #    #  #    #  #  #   ##      #       #    #  #    #  #      
//  #    #  #    #  #  #    #      ######   ####    ####   #      



int tic = 0;
int ota_delay = 0;

void loop(void) {
  unsigned long t_now = esp_timer_get_time();

  process_interrupts();

  if ((t_now - t_last) >= 100000) {
    t_last += 100000;

    if (rowing) check_slow_down(t_now);

    if (rowing) {
      stopwatch_inc();

      stats.elapsed+=0.1;
    }
// #ifdef ARDUINO
//     else {
//       ota_delay++;
//       if (ota_delay > 100) 
//         wifi_mode();
//     }
// #endif
  }

  delay(5); // avoid busy wait?!

    // If we're debugging - add some fake interrupts into the queue.
  if ((DEBUG) && (t_now >= fake_intr)) {
    push_interrupt(fake_intr);
    fake_intr += erg_sim[pErg_sim++] * 100;
    if (erg_sim[pErg_sim]==999999) {
#ifdef ARDUINO
      pErg_sim=0;
#else
      exit(0);
#endif
    }
  }
};

#ifndef ARDUINO
int main(int ac, char**av){
   DEBUG = ac - 1;

  setup();

  while(1) {
    loop();
  }
}
#endif

//   #####                                       #  #######                 
//  #     #  #####    ##    #####   #####       #   #        #    #  #####  
//  #          #     #  #   #    #    #        #    #        ##   #  #    # 
//   #####     #    #    #  #    #    #       #     #####    # #  #  #    # 
//        #    #    ######  #####     #      #      #        #  # #  #    # 
//  #     #    #    #    #  #   #     #     #       #        #   ##  #    # 
//   #####     #    #    #  #    #    #    #        #######  #    #  ##### 

void start_pull() {
  int i;

  // Tuning params 
  if (DEBUG==1) {
    printf(" /*;");
    for (i=0;i<MAX_STROKE_LEN;i++) printf("%d;",fg_N[i]);
    printf("|;");
    for (i=0;i<MAX_STROKE_LEN;i++) printf("%4.1f;",fg_dy[i]);
    printf("|;");
    printf("%3.1f;%3.1f;%3.1f;%3.1f; */\n",curr_score.f_eff,curr_score.b_eff,curr_score.spread,curr_score.offset);
  }

  force_graph_ready();

  stroke_len = 0;
  fg_N_sum = 0;
  fg_N_cnt = 0;
};

void end_pull(){
   int i;
   i = stroke_len;
   while (i < MAX_STROKE_LEN) {
      fg_N[i] = 0;
      fg_dy[i-1] = 0;
      i++;
   }
  update_stats(2);
  stroke_analysis();
  if ((DEBUG == 3) || (DEBUG == 0)) printf(" /* %d */\n",stats.stroke);
};

void record_force(int force) {
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
  int i;
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
  stdev -= 15; // max is about 26/27
  stdev *= 1.5;  // 21-15=6 *1.5 = 9 --  non linear might be better!?
  
  f_eff = (1+effic_fp)/(1+effic_fn) -2.0;
  b_eff = (1+effic_bp)/(1+effic_bn) -2.5;

  offset = mean - stroke_mid;  // -ve  front load +ve back load 


  if (f_eff>9.99) f_eff = 9.99;
  if (b_eff>9.99) b_eff = 9.99;
  if (stdev>9.99) stdev = 9.99;
  if (f_eff<0.1) f_eff = 0.1;
  if (b_eff<0.1) b_eff = 0.1;
  if (stdev<0.1) stdev = 0.1;  // range 1 to 10
  
  //11 - (offset * offset * 9) and min/max at 0,10

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

