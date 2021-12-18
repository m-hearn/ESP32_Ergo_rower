#include "globals.h"
#include "erg_debug.h"

#ifndef ARDUINO
  #include <math.h>
  #include <stdio.h>
#else
  #define ROWER_PIN 13
  void IRAM_ATTR Interrupt_pin_rower();
#endif


#define K_DAMP_START 0.004

#define W_DOT_DOT_MIN -300.0
#define W_DOT_DOT_MAX  50.0
#define W_DOT_MIN 0.0
#define MIN_RETURN 400
#define MIN_PULL 0.400

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



// tricky - as moment of inertia changes as the water moves outwards.  Maxes at .34, and min around .26
						//look at documentation for ways to measure it. Its easy to do.

static double J_moment = 0.32; //kg*m^2 - set this to the moment of inertia of your flywheel. 
static double d_omega_div_omega2 = K_DAMP_START;//erg_constant - to get this for your erg:


// https://www.concept2.com/indoor-rowers/training/calculators/watts-calculator
//  2:00 for 500 =  202.5 watts
//  1:50            263
//  1:40            350
//  1:30  for 500 = 480 watts
//  1:25            570
//  1:20            684
//  0:55  for 500 = 2103 watts


// int force_vector[FORCE_COUNT_MAX];

unsigned long t_start;
static double force;
static double force_max;
static int    force_maxh;

// #### ##    ## ######## ######## ########  ########  ##     ## ########  ######## 
//  ##  ###   ##    ##    ##       ##     ## ##     ## ##     ## ##     ##    ##    
//  ##  ####  ##    ##    ##       ##     ## ##     ## ##     ## ##     ##    ##    
//  ##  ## ## ##    ##    ######   ########  ########  ##     ## ########     ##    
//  ##  ##  ####    ##    ##       ##   ##   ##   ##   ##     ## ##           ##    
//  ##  ##   ###    ##    ##       ##    ##  ##    ##  ##     ## ##           ##    
// #### ##    ##    ##    ######## ##     ## ##     ##  #######  ##           ##   



#ifdef ARDUINO
unsigned long last_interrupt;
int interrupt_count = 0;

#define DEBOUNCE_INT 5
void IRAM_ATTR Interrupt_pin_rower()
{
  unsigned long now = millis();
  interrupt_count++;

  if ((interrupt_count > 1) && (now - last_interrupt > DEBOUNCE_INT)) {
    row_buff_head = (row_buff_head+1)&ROW_BUFF_SIZE;
    row_buffer[row_buff_head] = now;
    last_interrupt = now;
    interrupt_count = 0;
  }
};

#endif

//  ######  ######## ######## ##     ## ########  
// ##    ## ##          ##    ##     ## ##     ## 
// ##       ##          ##    ##     ## ##     ## 
//  ######  ######      ##    ##     ## ########  
//       ## ##          ##    ##     ## ##        
// ##    ## ##          ##    ##     ## ##        
//  ######  ########    ##     #######  ##        


void setup_rower(){

#ifdef ARDUINO
  pinMode(ROWER_PIN, INPUT_PULLUP);
#endif

  row_buff_head = 0;
  row_buff_tail = 0;

  FORCE_SCALE_Y = force_graph_maxy / (double) draw_force_h;
  
};

//  ######  ########    ###    ########  ######## 
// ##    ##    ##      ## ##   ##     ##    ##    
// ##          ##     ##   ##  ##     ##    ##    
//  ######     ##    ##     ## ########     ##    
//       ##    ##    ######### ##   ##      ##    
// ##    ##    ##    ##     ## ##    ##     ##    
//  ######     ##    ##     ## ##     ##    ##  

void start_rower(){
  int j;

#ifdef ARDUINO
  attachInterrupt(ROWER_PIN, Interrupt_pin_rower, RISING);
#endif

  stroke = 0;
  
  calorie_tot = ZERO;
  distance_rowed = ZERO;

  for (j = 0; j<MAX_N; j++) {	
    power_vector[j]= ZERO;
    speed_vector[j]= ZERO;
    power_ratio_vector[j]= ZERO;
    K_damp_estimator_vector[j]= K_DAMP_START;
    stroke_vector[j]= 2.0;  // start off assuming 30 spm
  }
  stroke_vector_avg = 2.0;

  omega_dot_screen= 0;
  omega_dot_dot_screen= 0;
  omega_vector[0]= ZERO;
  omega_vector[1]= ZERO;
  omega_vector[2]= ZERO;
  omega_vector[3]= ZERO;

  Wd_v[0]= ZERO;
  Wd_v[1]= ZERO;
  Wd_v[2]= ZERO;
  Wd_v[3]= ZERO;
	
	power_stroke_screen[0]= 1;
	power_stroke_screen[1]= 0;
	
	K_power = ZERO;
	J_power = ZERO;
	
	stroke_elapsed = ZERO;
	power_elapsed = ZERO;
  row_elaspsed = ZERO;

  asplit_minutes = 0;  asplit_secs = 0;
   split_minutes = 0;   split_secs = 0;

   force_ptr = 0;
   force_graph[0][0] = ZERO;
   force_graph[0][1] = ZERO;
   force = ZERO;
   force_max = ZERO;
   force_maxh = 0;

  row_hours = 0; row_minutes = 0; row_secs = 0.0;

  // reset to default
 	K_damp = J_moment*d_omega_div_omega2; //= 0.0005 Nms^2 

	cal_factor = PI*pow((K_damp/magic_factor), 1.0/3.0); // *(2.0/2.0)      distance per rev = 0.3532


  t_stroke = t_now;
  t_power = t_now;
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

//  ######     ###    ##        ######      ######  ######## ########   #######  ##    ## ######## 
// ##    ##   ## ##   ##       ##    ##    ##    ##    ##    ##     ## ##     ## ##   ##  ##       
// ##        ##   ##  ##       ##          ##          ##    ##     ## ##     ## ##  ##   ##       
// ##       ##     ## ##       ##           ######     ##    ########  ##     ## #####    ######   
// ##       ######### ##       ##                ##    ##    ##   ##   ##     ## ##  ##   ##       
// ##    ## ##     ## ##       ##    ##    ##    ##    ##    ##    ##  ##     ## ##   ##  ##       
//  ######  ##     ## ########  ######      ######     ##    ##     ##  #######  ##    ## ######## 

void calc_rower_stroke() {
    /***********************************************
    calculate omegas
    ************************************************/

    current_dt = (t_now - t_last)/1000.0;

    // save last
    omega_vector[3] = omega_vector[2];
    omega_vector[2] = omega_vector[1];
    omega_vector[1] = omega_vector[0];
    Wd_v[3] = Wd_v[2];    
    Wd_v[2] = Wd_v[1];
    Wd_v[1] = Wd_v[0]; 
    Wdd3 = Wdd2;
    Wdd2 = Wdd1;    
    Wdd1 = omega_dot_dot;    

    // calc W and Wd
    omega_vector[0] = (PI/current_dt + omega_vector[1] + omega_vector[2] + omega_vector[3]) / 4.0;   //omega_vector is rad/s  2*pi rad per rev - 3 of 6 ticks.
    Wd_v[0] = ((omega_vector[0] - omega_vector[1])/(current_dt+DELTA)  + Wd_v[1] + Wd_v[2] + Wd_v[3]) / 4.0;  // omega dot is this speed - last speed / time interval
    
    // if jerk (Wdd) is really high - guess that we got 2 of 2 interrupts instead of 2 of 3.  so pi*2*2/6 instead of pi*2*3/6
    if ((Wd_v[0] - Wd_v[1])/(current_dt+DELTA) > 1000) {
      current_dt = current_dt*1.5;
      omega_vector[0] = (PI/current_dt + omega_vector[1] + omega_vector[2] + omega_vector[3]) / 4.0;   //omega_vector is rad/s  2*pi rad per rev - 3 of 6 ticks.
      Wd_v[0] = ((omega_vector[0] - omega_vector[1])/(current_dt+DELTA)  + Wd_v[1] + Wd_v[2] + Wd_v[3]) / 4.0;  // omega dot is this speed - last speed / time interval
    }
    omega_dot_dot = ((Wd_v[0] - Wd_v[1])/(current_dt+DELTA) + Wdd1+Wdd2+Wdd3) / 4.0;   // omega dot dot is   this dot - last do / time interval

    /***********************************************
    calculate screeners to find power portion of stroke - see spreadsheet if you want to understand this
    ************************************************/
    
    if ((omega_dot_dot > W_DOT_DOT_MIN) && (omega_dot_dot < W_DOT_DOT_MAX))
          omega_dot_dot_screen = 0;
    else  omega_dot_dot_screen = 1;

    if (Wd_v[0] > W_DOT_MIN)
          omega_dot_screen = 1;
    else  omega_dot_screen = 0;

    if ( (omega_dot_dot_screen ==1) || ((omega_dot_screen ==1) && (power_stroke_screen[0] ==1))) {
      power_stroke_screen[1] = power_stroke_screen[0];
      power_stroke_screen[0] = 1;
    }	
    else {
      power_stroke_screen[1] = power_stroke_screen[0];
      power_stroke_screen[0] = 0;
    }
    
    /****************************************************************************************************
    calculate all items depending on where we are in stroke
    ****************************************************************************************************/
    
    /*********************************************************
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
      if ((t_now - t_stroke) < MIN_RETURN) {
        power_stroke_screen[0] = 0;
      } else {
        //start clock1 = power timer
        t_power = t_now;

        J_power = 0.0;
        K_power = 0.0;
        if(stroke > 1) {
          K_damp_estimator_vector[position1] = K_damp_estimator/(stroke_elapsed-power_elapsed+.000001);
          K_damp_estimator_vector_avg = weighted_avg(K_damp_estimator_vector, &position1);
          K_damp = J_moment*K_damp_estimator_vector_avg;
          position1 = (position1 + 1) % MAX_N;
          cal_factor = (2.0/2.0)*PI*pow((K_damp/magic_factor), 1.0/3.0); //distance per rev - trigger every 1/2 rev
        }
        stroke++; stroke_t = 0;
        if (stroke == 1) {
          t_start = t_now;
        }


        force_ptr = 0; 
        force_line = 0;
        if ((force_graph_maxy - force_max) < 125) {
          force_graph_maxy+=250;
        } else {
          if ((force_graph_maxy - force_max) > 375) {
            if (1 == force_maxh) {
              force_graph_maxy-=250;
              force_maxh = 0;
            } else {
              force_maxh = 1;
            }
          }
        }
        printf("%f %d\n", force_max, force_graph_maxy);
        FORCE_SCALE_Y = force_graph_maxy / (double) draw_force_h;
        force_max = ZERO;     
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
      tmp_elapsed = (t_now - t_power)/1000.0;
      // printf("X %f,%f,%ld\n", tmp_elapsed, power_elapsed, stroke);
      if ((tmp_elapsed > MIN_PULL) || (stroke == 0)) {  // stay in Power stroke for a while, avoid a false start/stop
        //end clock1 = power_timer
        power_elapsed = tmp_elapsed;
        //end clock2 = stroke_timer
        stroke_elapsed = (t_now - t_stroke)/1000.0;
        //start clock2 = stroke timer
        t_stroke = t_now;

        K_damp_estimator = 0.0;

        omega_vector_avg_curr = omega_vector_avg/(stroke_elapsed+DELTA);
        omega_vector_avg = 0.0;
        stroke_distance = distance_rowed-stroke_distance_old;
        stroke_distance_old = distance_rowed;
        
        speed_vector[position2] = stroke_distance/(stroke_elapsed +DELTA);
        if (stroke > 5) {
          split_time = 500.0/(weighted_avg(speed_vector, &position2) + DELTA);
          // format split
          split_minutes = (int)(split_time)/60;
          split_secs = ((split_time)-60*split_minutes);

          asplit_secs = 500.0*row_elaspsed/distance_rowed;
          asplit_minutes = (int)(asplit_secs/60);
          asplit_secs-= asplit_minutes*60;
        }
        
        // calc Watts
        power_ratio_vector[position2] = power_elapsed/(stroke_elapsed + DELTA);
        power_ratio_vector_avg = weighted_avg(power_ratio_vector, &position2);

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

  //       Time   str splt  dist aspl watts
  // char "SM m:ss A:5m WWW 12345 H:MI:SS"
  //       123456789012345678901234567890
        // printf("%0.2f", power_elapsed);

        sprintf(stats_curr,"%2.0f %1d %02.0f %1d %02.0f %3.0f %5.0f %1d%02d%02.1f"
          , 60.0/stroke_vector_avg
          , split_minutes, split_secs
          , asplit_minutes, asplit_secs
          , power_vector_avg
          , distance_rowed
          , row_hours, row_minutes, row_secs
          );
        }
    }

    distance_rowed += cal_factor;
    stroke_t+=current_dt;

    /*********************************************************
    if inside power stroke
    **********************************************************/
    if (power_stroke_screen[0] ==1) {
      J_power = J_power + J_moment * omega_vector[0]*Wd_v[0]*current_dt;
      K_power = K_power + K_damp*(omega_vector[0]*omega_vector[0]*omega_vector[0])*current_dt;
      omega_vector_avg = omega_vector_avg + omega_vector[0]*current_dt;

      // setup force plot
      force = (force + (J_moment*Wd_v[0] + K_damp*(omega_vector[0]*omega_vector[0]) ) /(current_dt)) /2;
      if (force > force_max) force_max = force;
      if (force > force_graph_maxy) force = force_graph_maxy;
      force_graph[++force_ptr][0]=stroke_t;
      force_graph[  force_ptr][1]=force;
    }
    
    /*********************************************************
    if in decay stroke
    **********************************************************/
    if (power_stroke_screen[0] ==0) {
      K_damp_estimator = K_damp_estimator-(Wd_v[0]/((omega_vector[0]+DELTA)*(omega_vector[0]+DELTA)))*current_dt;
      omega_vector_avg = omega_vector_avg + omega_vector[0]*current_dt;
    }


    if (stroke_t < 15) {  // if each stroke is less than 10 seconds keep adding.
      row_elaspsed+=current_dt;
      row_secs+=current_dt;
      if (row_secs >= 60) {
        row_secs-=60.0;
        if (++row_minutes > 60) {
          row_hours++;
          row_minutes -= 60;
        }
      }
    }

    t_last = t_now;

    sprintf(stats_curr+17,"%5.0f %1d%02d%02.1f"
      , distance_rowed
      , row_hours, row_minutes, row_secs
      );




    // printf("%ld,%d,%.3f,%4.3f,%d,%.0f,%.0f,%.0f,%4.3f,%5.5f,%2.0f,%.0f,%.0f,%.1f\n"
    //   ,t_now,stroke, stroke_t,current_dt
    //   , power_stroke_screen[0]*9
    //   , omega_vector[0], Wd_v[0], omega_dot_dot
    //   , cal_factor
    //   ,K_damp //, K_damp_estimator);
    //   , 60.0/stroke_vector_avg
    //   ,power_vector_avg
    //   ,distance_rowed
    //   ,force);

// printf("%d,%d,", omega_dot_screen*7, omega_dot_dot_screen*8);
// printf("%.0f,%.0f,", J_power, K_power);
 
};

#ifndef ARDUINO
int main() {
  int ind = 1;

  setup_rower();
  printf("T,S#,St,dt,R,W,Wd,Wdd,C,damp,SPM,Watts,D,force\n"); 

  t_now = erg_sim2[0];
  start_rower();

  while (t_now <= 888888) {
    t_last = t_now;
    t_now = erg_sim2[ind++];
    calc_rower_stroke();
  }
}
#endif