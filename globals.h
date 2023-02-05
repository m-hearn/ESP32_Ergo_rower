#ifndef GLOBALS

#define MAX_STROKE_LEN 25  // maximum number of ticks per stroke  - mine are about 5-6cm apart - so 1.3m total (missing a couple at the start and end)

#define Seconds *1000.0

// Erg sim
extern int erg_sim[];

//  debugable display!        Time   str splt  dist aspl watts
extern char stats_curr[36];  // char "H:MI:SS SM m:ss 12345 A:5m WWW"
extern char stats_disp[36];  //       123456789012345678901234567890

extern char stopwatch[8]; // mssmih:
extern char stopwatch_disp[8]; // :mmmssm but backwards
extern char stopwatch_max[];

extern  int row_hours;
extern  int row_minutes;
extern  double row_secs;

struct stats {
	int    stroke;
	double elapsed;
	int split_secs;
	int asplit_secs;
	double spm;
	int	 watts;
	double distance;
	int pull;
};

extern struct stats curr_stat;

struct analysis {
	double f_eff;
	double b_eff;
	double spread;
	double offset;
};

extern struct analysis curr_score, aver_score, last_score;

extern  int DEBUG;
extern  int rowing;

#define DISPLAY_CPU 0
#define ROWER_CPU 1

#define GLOBALS

extern void record_force(int force);
extern void start_pull();
extern void end_pull();

extern void stroke_analysis();

#endif