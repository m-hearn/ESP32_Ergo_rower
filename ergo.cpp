#ifdef ARDUINO
#include "Arduino.h"
#include "esp_timer.h"
#define ROWER_PIN 35
void IRAM_ATTR Interrupt_pin_rower_change();
#else
#include <stdio.h>
#include <math.h>
#endif
#include "globals.h"

#define ERGO
#include "ergo.h"

#include "display.h"

// local globals!
int stroke;
int t_total = 0;


/* 
88888888888                                        
88                                                 
88                                                 
88aaaaa      8b,dPPYba,   ,adPPYb,d8   ,adPPYba,   
88"""""      88P'   "Y8  a8"    `Y88  a8"     "8a  
88           88          8b       88  8b       d8  
88           88          "8a,   ,d88  "8a,   ,a8"  
88888888888  88           `"YbbdP"Y8   `"YbbdP"'   
                          aa,    ,88               
                           "Y8bbdP"   
 */

// http://eodg.atm.ox.ac.uk/user/dudhia/rowing/physics/ergometer.html


// 0x7F to support bit operated wrapping 0b01111111
#define  ROW_BUFF_SIZE 127
volatile unsigned long row_buffer[ROW_BUFF_SIZE+1];
volatile int row_buff_head;
int row_buff_tail;


/*number of constants for averaging*/
#define MAX_N	5
#define DELTA 0.00001
#define ZERO 0.0

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

// tricky - as moment of inertia changes as the water moves outwards.  Maxes at .34, and min around .26
						//look at documentation for ways to measure it. Its easy to do.

static double  J_moment = 0.268; //kg*m^2 - set this to the moment of inertia of your flywheel. 
static double K_damp; //related by J_moment*omega_dot = K_damp * omega^2 during non-power part of stroke
#define K_DAMP_EST 0.0033/J_moment
#define K_DAMP 0.0033

//Used if dynamically working out damping factor
//static double K_damp_estimator_vector_avg;

static double magic_factor = 2.8; //a heuristic constant people use to relate revs to distance-rowed


static double cal_factor; //distance per rev ... calculated later
static double volatile distance_rowed = ZERO;

static double stroke_vector_avg;
static double power_vector_avg;
static double calorie_tot = ZERO;
static double K_power = ZERO;
static double J_power = ZERO;

static double K_damp_estimator = ZERO;
static double power_ratio_vector_avg = ZERO;

static double stroke_elapsed = ZERO;
static double power_elapsed = ZERO;
static double tmp_elapsed = ZERO;

static double stroke_distance = ZERO;
static double stroke_distance_old = ZERO;

// Omega (W) diffs - vel - acc - jerk - snap - crackle - pop!
static double W_vel[2];
static double W_acc_raw[2];
static double W_acc_net[2];
static double W_jerk[2];
static double W_jerk_d;

static double current_dt = ZERO;
static double last_dt  = ZERO;
static int  tick;

static int power_stroke_screen[2];

unsigned long t_power, t_stroke;  // start time of power stroke and stroke
unsigned long t_last_intr;  // timestamps of interrupt ticks in the buffer
//unsigned long t_clock, t_real;  // clock counters

int position1 = 0, position2 = 0;

double stroke_vector[MAX_N];
double power_vector[MAX_N];
double power_ratio_vector[MAX_N];

// If estimating damping - use a moving average
//double K_damp_estimator_vector[MAX_N];
double speed_vector[MAX_N];

int slow_down_buffer = 2; // when slowing down, only apply after delay and do in chunks.


// https://www.concept2.com/indoor-rowers/training/calculators/watts-calculator
// 6st average = 120 watts =  2:22
// 10st avrage = 189       =  2:01
//      good   = 280       =  1:48
//      elite  = 380       =  1:37
// 140 2:15      260 1:50
// 160 2:10      280 1:48
// 180 2:05      300 1:45
// 200 2:01      320 1:43
// 220 1:57      340 1:41
// 240 1:53      360 1:39

// int force_vector[FORCE_COUNT_MAX];

static double force;

// #### ##    ## ######## ######## ########  ########  ##     ## ########  ######## 
//  ##  ###   ##    ##    ##       ##     ## ##     ## ##     ## ##     ##    ##    
//  ##  ####  ##    ##    ##       ##     ## ##     ## ##     ## ##     ##    ##    
//  ##  ## ## ##    ##    ######   ########  ########  ##     ## ########     ##    
//  ##  ##  ####    ##    ##       ##   ##   ##   ##   ##     ## ##           ##    
//  ##  ##   ###    ##    ##       ##    ##  ##    ##  ##     ## ##           ##    
// #### ##    ##    ##    ######## ##     ## ##     ##  #######  ##           ##   

#define RADSperTICK 2*PI/3

#ifdef ARDUINO
unsigned long last_interrupt;
volatile int interrupt_count = 0;

// 2ms is probably good for a circuit bounce,
// but we'll try 24 to beat some other spikes
// #define DEBOUNCE_INT 2
#define DEBOUNCE_INT 24000
void IRAM_ATTR Interrupt_pin_rower_change()
{
  unsigned long now = esp_timer_get_time();

  if ((now - last_interrupt) > DEBOUNCE_INT)
  {
    if (interrupt_count++ > 0) // rise and fall pair 
    {
      row_buff_head = (row_buff_head+1)&ROW_BUFF_SIZE;
      row_buffer[row_buff_head] = now;
      last_interrupt = now;
      interrupt_count = 0;
    }
  }
};
#endif

void push_interrupt(unsigned long intr) {
  row_buff_head = (row_buff_head+1)&ROW_BUFF_SIZE;
  row_buffer[row_buff_head] = intr;
}

//  ######  ######## ######## ##     ## ########  
// ##    ## ##          ##    ##     ## ##     ## 
// ##       ##          ##    ##     ## ##     ## 
//  ######  ######      ##    ##     ## ########  
//       ## ##          ##    ##     ## ##        
// ##    ## ##          ##    ##     ## ##        
//  ######  ########    ##     #######  ##        




void setup_rower() {
   int j;

#ifdef ARDUINO
  pinMode(ROWER_PIN, INPUT);
#endif

  stroke = 0;
  
  calorie_tot = ZERO;
  stats.distance = ZERO;

  for (j = 0; j<MAX_N; j++) {	
    power_vector[j]= ZERO;
    speed_vector[j]= ZERO;
    power_ratio_vector[j]= ZERO;
    //K_damp_estimator_vector[j]= K_DAMP_EST;
    stroke_vector[j]= 3.5;  // start off assuming 25 spm
  }
  stroke_vector_avg = 3.5;

  W_vel[0] = ZERO;
  W_acc_raw[0] = ZERO;
  W_acc_net[0] = ZERO;
  W_jerk[0] = ZERO;
  
  W_vel[1] = ZERO;
  W_acc_raw[1] = ZERO;
  W_acc_net[1] = ZERO;
  W_jerk[1] = ZERO;
  
  W_jerk_d = ZERO;

	
	power_stroke_screen[0]= 1;
	power_stroke_screen[1]= 0;
	
	K_power = ZERO;
	J_power = ZERO;
	
	stroke_elapsed = ZERO;
	power_elapsed = ZERO;

  force = ZERO;

  row_buff_head = 0;
  row_buff_tail = 0;

}

void ready_rower(){
  // rower is reset and put into reading state

  #ifdef ARDUINO
    attachInterrupt(ROWER_PIN, Interrupt_pin_rower_change, CHANGE);
  #endif

  rowing = 0;
}

//  ######  ########    ###    ########  ######## 
// ##    ##    ##      ## ##   ##     ##    ##    
// ##          ##     ##   ##  ##     ##    ##    
//  ######     ##    ##     ## ########     ##    
//       ##    ##    ######### ##   ##      ##    
// ##    ##    ##    ##     ## ##    ##     ##    
//  ######     ##    ##     ## ##     ##    ##  

void start_rower(unsigned long now){
  //int j;
  t_last_intr   = now;
  t_stroke = now;
  t_power  = now;

  // reset to defaults -- assume water is average 
  // for (j = 0; j<MAX_N; j++) {	
  //   K_damp_estimator_vector[j]= K_DAMP_EST;
  // }
 	K_damp = K_DAMP; 
  cal_factor = RADSperTICK*pow((K_damp/magic_factor), 1.0/3.0); //distance per tick
	//cal_factor = RADSperTICK*pow((J_moment*K_DAMP_EST/magic_factor), 1.0/3.0);
  
  stroke=0;
  rowing = 1;
};

//  ######  ########  #######  ########  
// ##    ##    ##    ##     ## ##     ## 
// ##          ##    ##     ## ##     ## 
//  ######     ##    ##     ## ########  
//       ##    ##    ##     ## ##        
// ##    ##    ##    ##     ## ##        
//  ######     ##     #######  ## 

void stop_rower(){
#ifdef ARDUINO
   detachInterrupt(ROWER_PIN);
#endif
   rowing=0;
};



/*********************************************************************************
weighted_avg_function
********************************************************************************/ 
static double  weighted_avg(double *vector, int *position) {
double temp_weight = ZERO, temp_sum = ZERO;
int j;

	for (j =0;j<MAX_N; j++) {
		temp_weight += pow(2,MAX_N-j-1);
		temp_sum += pow(2,MAX_N-j-1) * vector[(*position + MAX_N -j)% MAX_N];
	}
	temp_sum = temp_sum/temp_weight;
return temp_sum;
}


#define MIN_RETURN 880000
#define MIN_PULL 0.400
#define PULL_MIN 4.0

//  ######     ###    ##        ######      ######  ######## ########   #######  ##    ## ######## 
// ##    ##   ## ##   ##       ##    ##    ##    ##    ##    ##     ## ##     ## ##   ##  ##       
// ##        ##   ##  ##       ##          ##          ##    ##     ## ##     ## ##  ##   ##       
// ##       ##     ## ##       ##           ######     ##    ########  ##     ## #####    ######   
// ##       ######### ##       ##                ##    ##    ##   ##   ##     ## ##  ##   ##       
// ##    ## ##     ## ##       ##    ##    ##    ##    ##    ##    ##  ##     ## ##   ##  ##       
//  ######  ##     ## ########  ######      ######     ##    ##     ##  #######  ##    ## ######## 

void calc_rower_stroke(unsigned long t_intr) {
  /***********************************************
   calculate omegas
  ************************************************/

  current_dt = (t_intr - t_last_intr) / 1000000.0;
  if (0 == current_dt) return; // also avoids DIV by 0 -- otherwise use /(current_dt+DELTA)

  // save last
  W_vel[1] = W_vel[0];
  W_acc_raw[1] = W_acc_raw[0];
  W_acc_net[1] = W_acc_net[0];
  W_jerk[1] = W_jerk[0];
  	
  // calc W and Wd
  W_vel[0]  = (2*RADSperTICK/(current_dt+last_dt) + W_vel[1])/2.0;  
  
  W_acc_raw[0] = (W_acc_raw[1] + (W_vel[0] - W_vel[1]) / current_dt) / 2.0;  

  // if you're not pulling, then W_acc_net[0] is less than 0 (give or take a bit)
  W_acc_net[0] = ( W_acc_net[1] + W_acc_raw[0] + (K_damp * W_vel[0] * W_vel[0] / J_moment )) / 2.0;

  W_jerk[0] = (W_jerk[1] + (W_acc_net[0] - W_acc_net[1]) / current_dt) / 2.0;

  // not snap - only interested in Positive jerk! where there are spikes in the net pull
  // record the negative - to scale the positives?
  W_jerk_d = W_jerk[0] - W_jerk[1];

  power_stroke_screen[1] = power_stroke_screen[0];
  power_stroke_screen[0] = (W_acc_net[0] > PULL_MIN);
 
  /****************************************************************************************************
  calculate all items depending on where we are in stroke
  ****************************************************************************************************/
  
  /*********************************************************
   * 
   *   t_stroke   - return -  t_power - W_acc_net[0] -      t_stroke - return
   *                          ^      tmp_elapsed    ^
   *                          ^      power_elapsed  ^
   *   ^                             stoke_elapsed  ^
  if just ended decay stroke, just started power stroke

88888b.   .d88b.  888  888  888  .d88b.  888d888 
888 "88b d88""88b 888  888  888 d8P  Y8b 888P"   
888  888 888  888 888  888  888 88888888 888     
888 d88P Y88..88P Y88b 888 d88P Y8b.     888     
88888P"   "Y88P"   "Y8888888P"   "Y8888  888     
888                                              
888                                     
888 
  **********************************************************/
  if ((power_stroke_screen[0] ==1) && (power_stroke_screen[1] ==0)) {
    if ((t_intr - t_stroke) < MIN_RETURN) {  // put back into decay, ended too soon.
      power_stroke_screen[0] = 0;
    } else {

      //start clock1 = power timer
      t_power = t_intr;
      tick = 0;

      J_power = 0.0;
      K_power = 0.0;

      if(stroke > 1) {
        // ESTIMATE the damping force - used to start, and baseline.
        // K_damp_estimator_vector[position1] = K_damp_estimator/(stroke_elapsed-power_elapsed+DELTA);
        // K_damp_estimator_vector_avg = weighted_avg(K_damp_estimator_vector, &position1);
        // position1 = (position1 + 1) % MAX_N;
        // K_damp = J_moment*K_damp_estimator_vector_avg;
        // cal_factor = RADSperTICK*pow((K_damp/magic_factor), 1.0/3.0); //distance per tick

        // ESTIMATE the inertia - which moves around as the speed changes
        J_moment = 0.5*(J_moment + K_damp/(K_damp_estimator/(stroke_elapsed-power_elapsed+DELTA)));
        cal_factor = RADSperTICK*pow((K_damp/magic_factor), 1.0/3.0);
      }
      stroke++;
      stats.stroke++;

      start_pull();
    }
  }
  
  /*********************************************************
  if just ended power stroke, starting decay stroke
    888                                     
    888                                     
    888                                     
.d88888  .d88b.   .d8888b  8888b.  888  888 
d88" 888 d8P  Y8b d88P"        "88b 888  888 
888  888 88888888 888      .d888888 888  888 
Y88b 888 Y8b.     Y88b.    888  888 Y88b 888 
"Y88888  "Y8888   "Y8888P "Y888888  "Y88888 
                                        888 
                                  Y8b d88P 
                                    "Y88P"  
  **********************************************************/
  if ((power_stroke_screen[0] ==0) && (power_stroke_screen[1] ==1)) {

    tmp_elapsed = (t_intr - t_power) / 1000000.0;
    // printf("X %f,%f,%ld\n", tmp_elapsed, power_elapsed, stroke);
    if ((tmp_elapsed > MIN_PULL) || (stroke == 0)) {  // stay in Power stroke for a while, avoid a false start/stop

      power_elapsed = tmp_elapsed;
      stroke_elapsed = (t_intr - t_stroke) / 1000000.0;
      t_stroke = t_intr;
      force = ZERO;
      slow_down_buffer = 2;

      K_damp_estimator = 0.0;

      // omega_vector_avg_curr = omega_vector_avg/(stroke_elapsed+DELTA);
      // omega_vector_avg = 0.0;
      stroke_distance = stats.distance-stroke_distance_old;
      stroke_distance_old = stats.distance;
      
      speed_vector[position2] = stroke_distance/(stroke_elapsed +DELTA);

      if (stroke > 2) {
        stats.split_secs = (int) 500.0/(weighted_avg(speed_vector, &position2) + DELTA);
        stats.asplit_secs = (int) 500.0*stats.elapsed/stats.distance;

      }
      
      // power vs stroke time ratio
      power_ratio_vector[position2] = power_elapsed/(stroke_elapsed + DELTA);
      power_ratio_vector_avg = weighted_avg(power_ratio_vector, &position2);

      // calc Watts
      power_vector[position2]= (J_power + K_power)/(stroke_elapsed + DELTA);
      if (power_vector[position2] > 999.0) {
        power_vector[position2] = 0.0;
      }
      power_vector_avg = weighted_avg(power_vector, &position2);

      if ((power_vector_avg > 10.0) & (power_vector_avg < 900) & (stroke_elapsed < 6000.0)) {
        calorie_tot = calorie_tot+(4*power_vector_avg+350)*(stroke_elapsed)/(4.1868 * 1000.0);
      }
      
      stroke_vector[position2]= stroke_elapsed;
      stroke_vector_avg = weighted_avg(stroke_vector,&position2);
      position2 = (position2 + 1) % MAX_N;

      // update row stats for graphics

      stats.watts = power_vector_avg;
      stats.spm   = 60.0/(stroke_vector_avg+DELTA);

      end_pull();
    }
  }

  /*********************************************************
   if inside power stroke
  **********************************************************/
  if (power_stroke_screen[0] ==1) {
    J_power = J_power + J_moment * W_vel[0]*W_acc_raw[0]*current_dt;
    K_power = K_power + K_damp   *(W_vel[0]*W_vel[0]*W_vel[0])*current_dt;
    // omega_vector_avg = omega_vector_avg + W_vel[0]*current_dt;

    // setup force plot
    force = (force + (J_moment*W_acc_raw[0] + K_damp*(W_vel[0]*W_vel[0]) ) /(current_dt)) /2;
    record_force(force, W_jerk_d);
  }
  
  /*********************************************************
   if in decay stroke
  **********************************************************/
  if (power_stroke_screen[0] ==0) {
    K_damp_estimator = K_damp_estimator-(W_acc_raw[0]/((W_vel[0]*W_vel[0])+DELTA))*current_dt;
    // omega_vector_avg = omega_vector_avg + W_vel[0]*current_dt;
  }

  stats.distance += cal_factor;
  t_total += t_intr - t_last_intr;

  switch (DEBUG) {
  case 2:
     printf("%7d,%3d,%2d,%3ld, ", t_total, stats.stroke, tick, (t_intr - t_last_intr)/10);
     printf("%5.4f,%6.1f,%6.1f,%6.1f, ", J_moment, W_jerk[0], W_acc_raw[0], W_vel[0]);// skipping W
     printf("%4.3f, %5.5f,%5.5f, ", W_acc_net[0], K_damp, W_jerk_d);
     printf("%3.1f,%5.5f,%4ld, ", stats.spm, stroke_elapsed, t_intr - t_power);
     printf("%6.2f,%6.2f,%4.0f,%3.0f,", J_power, K_power, force, power_vector_avg);
     printf("% 5.0f\n", stats.distance);
     break;
  case 1:
     break;
  case 0:
  case 3:
     printf("%d,", (int) (t_intr - t_last_intr)/100);
  }

  update_stats(1);
    
  t_last_intr = t_intr;
  last_dt = current_dt;
  tick++;
};

//      888                                 
//      888                                 
//      888                                 
//  .d88888  8888b.  88888b.d88b.  88888b.  
// d88" 888     "88b 888 "888 "88b 888 "88b 
// 888  888 .d888888 888  888  888 888  888 
// Y88b 888 888  888 888  888  888 888 d88P 
//  "Y88888 "Y888888 888  888  888 88888P"  
//                                 888      
//                                 888      
//                                 888  

void check_slow_down(unsigned long t_real) {
  double new_elapsed;

  if (stroke <2) return;

  new_elapsed = (t_real - t_power) / 1000000.0;

  if ((new_elapsed - slow_down_buffer) < stroke_elapsed) return;
  
  if (new_elapsed > 6) rowing = 0;

  slow_down_buffer++;

  // Serial.print(".");

  // speed_vector[position2] = stroke_distance/(new_elapsed +DELTA);

  // stats.split_secs = (int) 500.0/(weighted_avg(speed_vector, &position2) + DELTA);
  // stats.asplit_secs = (int) 500.0*stats.elapsed/stats.distance;
  
  // // format split -- should be done in display
  // split_minutes = stats.split_secs/60;
  // split_secs   =  stats.split_secs -split_minutes*60;

  // asplit_minutes = stats.asplit_secs/60;
  // asplit_secs    = stats.asplit_secs - asplit_minutes*60;
  
  // // calc Watts
  // power_ratio_vector[position2] = power_elapsed/(new_elapsed + DELTA);
  // power_ratio_vector_avg = weighted_avg(power_ratio_vector, &position2);

  // power_vector[position2]= (J_power + K_power)/(new_elapsed + DELTA);
  // if (power_vector[position2] > 999.0) {
  //   power_vector[position2] = 0.0;
  // }
  // power_vector_avg = weighted_avg(power_vector, &position2);

  // stroke_vector[position2]= new_elapsed;
  // stroke_vector_avg = weighted_avg(stroke_vector,&position2);
  
  // // // update row stats for graphics

  // // stats.watts = power_vector_avg;
  // stats.spm   = 60.0/(stroke_vector_avg+DELTA);
}

//                                        d8b          888            
//                                        Y8P          888            
//                                                     888            
// 88888b.  888d888  .d88b.   .d8888b     888 88888b.  888888 888d888 
// 888 "88b 888P"   d88""88b d88P"        888 888 "88b 888    888P"   
// 888  888 888     888  888 888          888 888  888 888    888     
// 888 d88P 888     Y88..88P Y88b.        888 888  888 Y88b.  888     
// 88888P"  888      "Y88P"   "Y8888P     888 888  888  "Y888 888     
// 888                                                                
// 888                                                                
// 888         

void process_interrupts(){
  
  while (row_buff_head != row_buff_tail) {
    row_buff_tail = (row_buff_tail+1)&ROW_BUFF_SIZE; // move the pointer on.
    // printf("+");
    if (!rowing) { 
      start_rower(row_buffer[row_buff_tail]); // start rowing calcs - first interrupt received
    } else {
      calc_rower_stroke(row_buffer[row_buff_tail]);
    }
  }
}

