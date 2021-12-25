#ifndef GLOBALS

/*number of constants for averaging*/
#define MAX_N	5
#define DELTA 0.00001
#define ZERO 0.0

#define Seconds /1000.0

#ifndef PI
#define PI 3.1415926535897932384626433832795
#endif

#define  ROW_BUFF_SIZE 1023
volatile unsigned long row_buffer[ROW_BUFF_SIZE+1];
volatile int row_buff_head;
int row_buff_tail;


static double K_damp; //related by J_moment*omega_dot = K_damp * omega^2 during non-power part of stroke
static double cal_factor; //distance per rev ... calculated later
static double magic_factor = 2.8; //a heuristic constant people use to relate revs to distance-rowed


static double volatile distance_rowed = ZERO;

static long volatile chop_counter;	

static double stroke_vector_avg;
static double power_vector_avg;
static double calorie_tot = ZERO;
static double K_power = ZERO;
static double J_power = ZERO;
static double K_damp_estimator_vector_avg;

static double K_damp_estimator = ZERO;
// static double power_ratio_vector_avg = ZERO;

static double stroke_elapsed = ZERO;
static double power_elapsed = ZERO;
static double row_elaspsed = ZERO;
static double tmp_elapsed = ZERO;

static double stroke_distance = ZERO;
static double stroke_distance_old = ZERO;

static double W_v[2];
static double Wd_v[2];
static double Wdd_v = ZERO;
static double Wdd_v1 = ZERO;

static double current_dt = ZERO;
static double last_dt  = ZERO;
// static double omega_vector_avg = ZERO;
// static double omega_vector_avg_curr = 0.0;

static int Wd_screen = 0;
static int Wdd_screen = 0;
static int power_stroke_screen[2];

unsigned long t_power, t_stroke;  // start time of power stroke and stroke
unsigned long t_last, t_now;  // timestamps of interrupt ticks in the buffer
unsigned long t_clock, t_real;  // clock counters

int position1 = 0, position2 = 0;

double stroke_vector[MAX_N];
double power_vector[MAX_N];
double power_ratio_vector[MAX_N];
double K_damp_estimator_vector[MAX_N];
double speed_vector[MAX_N];


// Force Graph
#define FORCE_BUF 100
static   int    stroke = 0;
static   double stroke_t = 0;
volatile int    force_line;
volatile int    force_ptr;
volatile double force_graph[FORCE_BUF][2];

static int draw_force_x = 50;
static int draw_force_y = 465;
static int draw_force_h = 150;
static int force_graph_maxy = 1000;
static double FORCE_SCALE_Y = 10.0;

//  debugable display!        Time   str splt  dist aspl watts
char stats_curr[36];  // char "H:MI:SS SM m:ss 12345 A:5m WWW"
char stats_disp[36];  //       123456789012345678901234567890

static int row_hours;
static int row_minutes;
static double row_secs;
static int split_minutes;
static int split_secs;
static int asplit_minutes;
static int asplit_secs;

static int DEBUG = 0;
static int rowing =0;
static int paused = 0;

#define DISPLAY_CPU 0
#define ROWER_CPU 1

#define GLOBALS

#endif