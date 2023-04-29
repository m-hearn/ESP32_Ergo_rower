unsigned long t_last, fake_intr = 0;

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

unsigned long esp_timer_get_time()
{
    return fake_intr + 1;
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
extern void setup_storage();
extern void setup_time();

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
double fg_jd[MAX_STROKE_LEN];
double fg_N_sum;
double fg_N_cnt;

int pErg_sim = 0;

char stopwatch[8]; // mssmihX
char stopwatch_disp[8]; // hmmssm but backwards
char stopwatch_max[] = "995959:";
void stopwatch_reset();
void stopwatch_inc();

#define DEBUG_PIN 12

void setup() {
// TaskHandle_t Time_t_handle;
  
#ifdef ARDUINO
  pinMode(DEBUG_PIN, INPUT_PULLUP);

  Serial.begin(250000);
  while (!Serial);
  
  setup_storage();
  setup_display();
  setup_ota();
  setup_time();
  setup_BLE();  // must be AFTER wifi - otherwise it crashes
  DEBUG = !(digitalRead(DEBUG_PIN));
#endif

  if (DEBUG==2) {
    printf("T, S, Tic, dT,");
    printf("J_moment, Jerk,Acc_R,Vel, ");
    printf("Acc_N, Kdamp, Jerk_D,");
    printf("Spm,Se,St,");
    printf("J, K, FN, W,");
    printf("D\n");
  }
  
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
  }

#ifdef ARDUINO
  delay(5); // avoid busy wait?!
#endif

    // If we're debugging - add some fake interrupts into the queue.
  if ((DEBUG) && (t_now >= fake_intr)) {
    push_interrupt(fake_intr);
    fake_intr += erg_sim[pErg_sim++] * 10;
    if (erg_sim[pErg_sim]==__UINT16_MAX__) {
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

int eip, fip;
double sum_jerk_d;

void start_pull() {
  int i;

  // Tuning params 
  if (DEBUG==1) {
    printf(" /*;");
    //for (i=0;i<25;i++) printf("%d;",fg_N[i]);
    for (i = 0; i < 25; i++) printf("%4.4f;", fg_jd[i]);
    printf("|;");
    for (i = 0; i < 25; i++) if ((i >= eip) && (i <= fip)) printf("1;"); else printf("0;"); // printf("%4.1f;", fg_dy[i]);
    printf("|;");
    printf("%3.1f;%3.1f;%3.1f;%3.1f; */\n", curr_score.f_eff, curr_score.b_eff, curr_score.spread, curr_score.offset);
  }

  force_graph_ready();

  stroke_len = 0;
  fg_N_sum = 0;
  fg_N_cnt = 0;

  eip = 0;
  fip = 0;
  sum_jerk_d = 0.0;
};

void end_pull(){
   int i;
   i = stroke_len;
   while (i < MAX_STROKE_LEN) {
      fg_N[i] = 0;
      fg_dy[i-1] = 0;
      fg_jd[i] = 0;
      i++;
   }
   for (fip = stroke_len -1;fg_jd[fip]>0;fip--)
      fg_jd[fip] = 0;

  update_stats(2);
  stroke_analysis();
  if ((DEBUG == 3) || (DEBUG == 0)) printf(" /* %d */\n",stats.stroke);
};



void record_force(int force, double jerk_d) {
  // check bounds
  if (stroke_len == MAX_STROKE_LEN) return;
  if (force < 0) force = 0;

  force_graph_plot(force);

  fg_N[stroke_len] = force;

  if ((eip == 0) && (jerk_d < 0))
     eip = stroke_len;
  if (eip) {
     fg_jd[stroke_len] = jerk_d;
     if (jerk_d < 0) 
         sum_jerk_d += jerk_d;
  }

  // mean & centre
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

void stroke_analysis() {
   int i;
   // do the analysis here
   double stdev = 0.0;
   double mean = 0.0;  // this wants to be close to stroke_len / 2
   double f_j = 0.0;
   double b_j = 0.0;
   double offset;


   mean = fg_N_sum / fg_N_cnt;

   for (i = 1; i <= stroke_len * .55; i++) {
      if (fg_jd[i] > 0) f_j += fg_jd[i];
      stdev += (mean - (double)(i + 1.0)) * (mean - (double)(i + 1.0)) * (double)fg_N[i];
   }
   for (; i < stroke_len; i++) {
      if (fg_jd[i] > 0) b_j += fg_jd[i];
      stdev += (mean - (double)(i + 1.0)) * (mean - (double)(i + 1.0)) * (double)fg_N[i];
   }

   f_j = 10 + 100 * f_j / sum_jerk_d;
   b_j = 10 + 100 * b_j / sum_jerk_d;

  stdev = stdev / fg_N_cnt;
  stdev -= 15; // max is about 26/27
  stdev *= 1.5;  // 21-15=6 *1.5 = 9 --  non linear might be better!?
  
  offset = mean - (stroke_len / 2.0);  // -ve  front load +ve back load 

  if (f_j > 9.99) f_j = 9.99;  
  if (b_j > 9.99) b_j = 9.99;

  if (stdev>9.99) stdev = 9.99;
  if (f_j<0.1) f_j = 0.1;
  if (b_j<0.1) b_j = 0.1;
  if (stdev<0.1) stdev = 0.1;  // range 1 to 10
  
  //11 - (offset * offset * 9) and min/max at 0,10


  last_score.f_eff = aver_score.f_eff;  
  last_score.b_eff = aver_score.b_eff;  
  last_score.spread = aver_score.spread;  
  last_score.offset = aver_score.offset;

  curr_score.f_eff  = f_j;
  curr_score.b_eff  = b_j;
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

