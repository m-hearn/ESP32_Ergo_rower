#ifndef GLOBALS

#define Seconds *1000.0



// Force Graph
#define FORCE_BUF 35

static   double stroke_t = 0;
volatile int    force_line;
volatile int    force_ptr;
volatile double force_graph[FORCE_BUF][2];



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

struct stats {
	int    stroke;
	double elapsed;
	int split_secs;
	int asplit_secs;
	double spm;
	int	 watts;
	double distance;
};

static int stroke = 0;

struct stats curr_stat, disp_stat;

static int DEBUG = 0;
static int rowing =0;

#define DISPLAY_CPU 0
#define ROWER_CPU 1

#define GLOBALS

#endif