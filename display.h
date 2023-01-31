/* 
                                                                        
88888888ba,    88                          88                           
88      `"8b   ""                          88                           
88        `8b                              88                           
88         88  88  ,adPPYba,  8b,dPPYba,   88  ,adPPYYba,  8b       d8  
88         88  88  I8[    ""  88P'    "8a  88  ""     `Y8  `8b     d8'  
88         8P  88   `"Y8ba,   88       d8  88  ,adPPPPP88   `8b   d8'   
88      .a8P   88  aa    ]8I  88b,   ,a8"  88  88,    ,88    `8b,d8'    
88888888Y"'    88  `"YbbdP"'  88`YbbdP"'   88  `"8bbdP"Y8      Y88'     
                              88                               d8'      
                              88                              d8'  
 */

// http://www.barth-dev.de/online/rgb565-color-picker/  

#include "SPI.h"
#include "TFT_eSPI.h"

#include "globals.h"

#define GFXFF 1
#include "verdanab18.h"
#define VERD18 &verdanab18
#define VERD22 &verdanab22
#define VERD36 &verdanab36
#define VERD18W 25
#define VERD22W 30
#define VERD36W 55


#define X_SPLIT 10
#define X_DIST 170
#define Y_DIST 165
#define X_ASPLIT 210
#define X_WATT 210
#define Y_WATT 210
#define X_SM 210
#define Y_SM 255



#define CALIBRATION_FILE "/TouchCalData"
TaskHandle_t Display_t_handle;
void core0_handler(void *param);

TFT_eSPI tft = TFT_eSPI((int16_t) 240, (int16_t) 320);


                                                 
// 88888b.   .d88b.  888  888  888  .d88b.  888d888 
// 888 "88b d88""88b 888  888  888 d8P  Y8b 888P"   
// 888  888 888  888 888  888  888 88888888 888     
// 888 d88P Y88..88P Y88b 888 d88P Y8b.     888     
// 88888P"   "Y88P"   "Y8888888P"   "Y8888  888     
// 888                                              
// 888                                              
// 888    

#define PG_Y 210
#define PG_X 5

TFT_eSprite pg_sprite = TFT_eSprite(&tft);
uint16_t pg_palette[16];
int pg_scroll_int =0;
int pg_dirty  = 0;

void powergraph_setup(){
  // Populate the palette table, table must have 16 entries
  pg_palette[0]  = TFT_BLACK;
  pg_palette[1]  = TFT_ORANGE;
  pg_palette[2]  = TFT_YELLOW;
  pg_palette[3]  = TFT_DARKCYAN;
  pg_palette[4]  = TFT_MAROON;
  pg_palette[5]  = TFT_PURPLE;
  pg_palette[6]  = TFT_OLIVE;
  pg_palette[7]  = 0x39c7; //56,56,56
  pg_palette[8]  = TFT_ORANGE;
  pg_palette[9]  = TFT_BLUE;
  pg_palette[10] = TFT_GREEN;
  pg_palette[11] = TFT_CYAN;
  pg_palette[12] = TFT_RED;
  pg_palette[13] = TFT_NAVY;
  pg_palette[14] = TFT_YELLOW;
  pg_palette[15] = TFT_WHITE;

  pg_sprite.createSprite(200,75);
  pg_sprite.setColorDepth(4);
  pg_sprite.createPalette(pg_palette);
  pg_sprite.drawRect(0,0,200,75,7);
  pg_sprite.setScrollRect(1,1,197,72); //,0);
}

void powergraph_draw(){
  // if (powergraph.draw)
  if (pg_dirty)
    pg_sprite.pushSprite(PG_X,PG_Y);
  pg_dirty = 0;
}

void powergraph_plot(double watts, double spm){
  int dw, ds;

  dw = 72- (int) watts*72.0/440.0;
  ds = 72- (int) (spm-15)*72.0/45.0;
  // watts
  if (dw > 2 && dw < 73) {
    pg_sprite.drawPixel(197, dw-1, 1);
    pg_sprite.drawPixel(197, dw  , 1);
    pg_sprite.drawPixel(197, dw+1, 1);
  }
  // spm
  if (ds > 2 && ds < 73) {
    pg_sprite.drawPixel(197, ds-1, 2);
    pg_sprite.drawPixel(197, ds  , 2);
    pg_sprite.drawPixel(197, ds+1, 2);
  }
  pg_dirty = 1;
}

void powergraph_scroll(){
  // update called every 0.1
  if (pg_scroll_int++) {
    pg_scroll_int = 0;
    pg_sprite.scroll(-1,0);
    pg_sprite.drawFastVLine(197,1,73,0);
    pg_dirty = 1;
  }
}

//  .d888                                    
// d88P"                                     
// 888                                       
// 888888  .d88b.  888d888  .d8888b  .d88b.  
// 888    d88""88b 888P"   d88P"    d8P  Y8b 
// 888    888  888 888     888      88888888 
// 888    Y88..88P 888     Y88b.    Y8b.     
// 888     "Y88P"  888      "Y8888P  "Y8888  
                                          

#define X_TIME 10

#define FG_X 50
#define FG_Y 305
#define FG_H 160
#define FG_W 250
#define force_graph_maxy 2400
#define force_scale_y force_graph_maxy/FG_H

// Force Graph
#define FORCE_BUF 25
#define FORCE_STROKES 10

static   double stroke_t = 0;
volatile int    fg_draw_len;
volatile int    fg_stroke_len;
int fg_stroke;
volatile double fga[FORCE_STROKES][FORCE_BUF];


#define GRAPH_GRID TFT_DARKGREY  
#define GRAPH_AXIS TFT_BLUE
#define GRAPH_LINE TFT_CYAN

#define FORCE_SCALE_X 250.0

TFT_eSprite fg_sprite = TFT_eSprite(&tft);
uint16_t fg_palette[16];

void forcegraph_setup() {
  int d,s;

  tft.drawLine(FG_X-1,FG_Y+FG_H+2,FG_X+FG_W,FG_Y+FG_H+2   ,GRAPH_AXIS);
  tft.drawLine(FG_X-1,FG_Y     +1,FG_X-1   ,FG_Y+FG_H     ,GRAPH_AXIS);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
  tft.setTextSize(2);

  tft.setCursor(18,360);
  tft.printf("Kg");

  // force_scale_y = force_graph_maxy / (double) FG_H;

  d=0;
// Grey scale increments in 565 bits
// spread out to fade out to black after 10 strokes
  fg_palette[d++] = TFT_BLACK;
  // fg_palette[d++] = 0x0841;
  // fg_palette[d++] = 0x1082;
  // fg_palette[d++] = 0x18c3;
  fg_palette[d++] = 0x2104;
  // fg_palette[d++] = 0x2945;
  fg_palette[d++] = 0x3186;
  // fg_palette[d++] = 0x39c7;
  fg_palette[d++] = 0x4208;
  // fg_palette[d++] = 0x4a49;
  fg_palette[d++] = 0x528a;
  // fg_palette[d++] = 0x5acb;
  fg_palette[d++] = 0x630c;
  // fg_palette[d++] = 0x6b4d;
  fg_palette[d++] = 0x738e;
  // fg_palette[d++] = 0x76cf
  fg_palette[d++] = 0x8410;
  fg_palette[d++] = 0xa534;  
  fg_palette[d++] = 0xb5b6;  

  fg_palette[d++] = TFT_RED;
  fg_palette[d++] = TFT_ORANGE;
  fg_palette[d++] = TFT_YELLOW;
  fg_palette[d++] = TFT_GREENYELLOW;
  fg_palette[d++] = TFT_GREEN;
  fg_palette[d++] = TFT_SKYBLUE;


  fg_stroke = 0;
  fg_draw_len = 0;
  fg_stroke_len = 0;

  for(d = 0; d< FORCE_BUF; d++)
    for(s = 0; s< FORCE_STROKES; s++)
      fga[s][d]=FG_H;

  fg_sprite.createSprite(100,80);
  fg_sprite.setColorDepth(4);
  fg_sprite.createPalette(fg_palette);
  fg_sprite.drawRect(0,0,99,79,9);
  fg_sprite.pushSprite(219,288);
}


//   480 total
//   <space 60> | <graphx_min 17>  300 wide (-17)  |   260+17
#define GRAPHX_MIN 17
#define GRAPHX_MAX 243
#define FG_X_SCALE 10



int fg[16][FORCE_BUF];
int fg_N[FORCE_BUF];
double fg_dy[FORCE_BUF];
double fg_N_sum;
double fg_N_cnt;
int stroke_len = 1;
int fg_dirty =0;

double af_eff = 5.0;
double ab_eff = 5.0;
double astdev = 5.0;
double aoffst = 0.0;

// coach
// Late arms   // negatives in back half / dips
// Late torso  // negatives in front half
// Too early   // mean < 0.5 stroke
// Weak legs   // mean > 0.5 stroke
// Too loose   // high stddev?
// Too peaky   // low stddev
// Not smooth  // negative curves

// Next stroke starting
void forcegraph_ready(){
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

  // clear the rest of this strokes line data  - jic
  while (fg_stroke_len < FORCE_BUF) {
    fg_N[fg_stroke_len] = 0;
    fg_dy[fg_stroke_len-1] = 0;
    fga[fg_stroke][fg_stroke_len++] = FG_H;
  }

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
  for (;(i<FORCE_BUF) && (fg_N[i]);i++){
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
  // for (i=0;i<FORCE_BUF;i++) Serial.printf("%d,",fg_N[i]);
  // for (i=0;i<FORCE_BUF;i++) Serial.printf("%4.2f,",fg_dy[i]);
  // Serial.printf("%3.3f,%2.2f,%4.1f,%4.1f,%2.1f,%4.1f,%4.1f,%2.1f,%3.1f\n", mean, stroke_mid, effic_fn, effic_fp, effic_bn, effic_bp, f_eff, b_eff, stdev);
  Serial.printf("%3.1f, %3.1f, %3.1f, %2.1f, %3.1f, %3.1f, %3.1f, %2.1f\n", f_eff, b_eff, stdev, offset, af_eff, ab_eff, astdev, aoffst);

#define FG_BLOB 8

  fg_sprite.fillRect(0,0,99,79,0);
  fg_sprite.drawLine(49,0,49,79,9);
  fg_sprite.drawLine(0,80-7*8,98,80-7*8,14);

  fx = 46-(int)(astdev*4)-(int)(aoffst*5);
  fy = 80-(int)(af_eff*8);
  bx = 54+(int)(astdev*4)-(int)(aoffst*5);
  by = 80-(int)(ab_eff*8);
  fg_sprite.fillRect(fx,fy,FG_BLOB,FG_BLOB,9);
  fg_sprite.fillRect(bx,bx,FG_BLOB,FG_BLOB,9);
  fg_sprite.drawLine(fx+4,fy+4,bx+4,by+4,6);
  
  fx = 46-(int)(stdev*4)-(int)(offset*5);
  fy = 80-(int)(f_eff*8);
  bx = 54+(int)(stdev*4)-(int)(offset*5);
  by = 80-(int)(b_eff*8);
  fg_sprite.drawRect(fx,fy,FG_BLOB,FG_BLOB,(int)(10.4+(f_eff/2)));
  fg_sprite.drawRect(bx,by,FG_BLOB,FG_BLOB,(int)(10.4+(b_eff/2)));
  fg_sprite.drawLine(fx+4,fy+4,bx+4,by+4,(int)(10.4+(f_eff+b_eff)/4));
  

  // af_eff = (af_eff*(9-f_eff)+(1+f_eff)*f_eff)/10.0;
  // ab_eff = (ab_eff*(9-b_eff)+(1+b_eff)*b_eff)/10.0;
  // astdev = (astdev*(9-stdev)+(1+stdev)*stdev)/10.0;
  af_eff = (af_eff*(190-f_eff)+(10+f_eff)*f_eff)/200.0;
  ab_eff = (ab_eff*(190-b_eff)+(10+b_eff)*b_eff)/200.0;
  astdev = (astdev*(190-stdev)+(10+stdev)*stdev)/200.0;
  aoffst = (aoffst*9+offset)/10;
  
  fx = 46-(int)(astdev*4)-(int)(aoffst*5);
  fy = 80-(int)(af_eff*8);
  bx = 54+(int)(astdev*4)-(int)(aoffst*5);
  by = 80-(int)(ab_eff*8);
  fg_sprite.fillRect(fx,fy,FG_BLOB,FG_BLOB,(int)(10.4+(af_eff/2)));
  fg_sprite.fillRect(bx,by,FG_BLOB,FG_BLOB,(int)(10.4+(ab_eff/2)));
  fg_sprite.fillRect((fx+bx)/2,(fy+by)/2,5,5,9);
  fg_sprite.drawLine(fx+4,fy+4,bx+4,by+4,9);
  
  fg_dirty = 1;
  

  // effic_p/effic_n from 2 to 4+    4+ is good
  // stdev 16 to 25 +  18 is narrow  22+ is good
  // stroke_mid - mean   +ve is front loaded  -ve is late   under 0.5 is good?

  fg_stroke_len = 0; 
  fg_draw_len = 0;
  if (++fg_stroke == FORCE_STROKES) fg_stroke = 0;

  fg_N_sum = 0;
  fg_N_cnt = 0;
  stroke_len = 1;
}
  
void forcegraph_log(int force) {
  int force_y;
  double dy;

  if (fg_stroke_len == FORCE_BUF) return;
  if (force < 0) force = 0;
  
  fg_N[fg_stroke_len]= force;

// Doesn't diff the first tick - it's most likely negative gradient - so okay to ignore
  if (fg_stroke_len > 1) {
    if (force) {
      stroke_len++;
      fg_dy[fg_stroke_len-1] = (double)(fg_N[fg_stroke_len-1]) - (double)(fg_N[fg_stroke_len-2]+fg_N[fg_stroke_len])/2.0;
      // //  95 10 5  -> 10 - (95+5)/2 = -40  ->  10/-40 = -.25      
      // dy = (double) fg_N[fg_stroke_len -1] / dy;
      // if (dy > +0.1){
      //   fg_dy[fg_stroke_len-1] = dy;
      // }
      // if (dy < -0.1) {
      //   fg_dy[fg_stroke_len-1] = dy;
      // }
    }
    else fg_dy[fg_stroke_len-1] = 0;
  }
  fg_N_sum += (fg_stroke_len+1) * force;
  fg_N_cnt += force;
  
  // save the force for plotting, and index
  force_y = (int) (force / (force_scale_y));
  if (force_y > FG_H) force_y = FG_H;
  // if (force_y < 0)    force_y = 0;
  fga[fg_stroke][fg_stroke_len]=FG_H - force_y;

  fg_stroke_len++;
}


void forcegraph_draw(){
  int x1,y1,x2,y2;
  int d,s,ss;
  int fg_histclr = 1;

  if (fg_dirty) {
    fg_sprite.pushSprite(219,288);
    fg_dirty = 0;
  }

  if(!rowing) return;

  // start of a new stroke
  // cycle through previous strokes
  // redraw from black through to light grey
  // effectively fade out the oldest stroke

  if (fg_draw_len == 0) {
    for(d = 0; d<FORCE_BUF-1; d++) {
      for(s = 0, ss=fg_stroke; s< FORCE_STROKES; s++, ss++) {
        if (ss == FORCE_STROKES) ss = 0;
        if(fga[ss][d]+fga[ss][d+1]< (2*FG_H))  {
          tft.drawLine(FG_X+d*FG_X_SCALE, FG_Y+fga[ss][d], FG_X+d*FG_X_SCALE+FG_X_SCALE, FG_Y+fga[ss][d+1], fg_palette[s]);
        }
      }
    }
    fg_draw_len = 1;
  }
  
  while (fg_draw_len < fg_stroke_len) {
    y1 = fga[fg_stroke][fg_draw_len-1] + FG_Y;
    y2 = fga[fg_stroke][fg_draw_len  ] + FG_Y;
    if (y1+y2<((FG_Y+FG_H)*2)) {
      x1 = (fg_draw_len-1)*FG_X_SCALE        + FG_X;
      x2 = x1+FG_X_SCALE;
      tft.drawLine      (x1, y1, x2, y2, TFT_YELLOW);
    }
    fg_draw_len++;
  }
};

static int disp_loc[][4] = {
  { X_SM,        Y_SM,1,1},  //S
  { X_SM+VERD18W,Y_SM,1,1},  //M
  { 0,0,0,0},
  { X_SPLIT,75, 1,3}, //split m
  { X_SPLIT+VERD36W-10,72, 1,3}, //:
  { X_SPLIT+VERD36W*2-40,75, 1,3}, //s
  { X_SPLIT+VERD36W*3-44,75, 1,3}, //s
  { 0,0,0,0},
  { X_ASPLIT,75, 1, 2}, //Asplit m
  { X_ASPLIT+VERD22W  -2,74, 1, 2}, //:
  { X_ASPLIT+VERD22W*2-16,75, 1, 2}, //ss
  { X_ASPLIT+VERD22W*3-16,75, 1, 2},
  {0,0,0,0},
  {X_WATT,          Y_WATT,1, 1}, //WWW
  {X_WATT+VERD18W,  Y_WATT,1, 1},
  {X_WATT+VERD18W*2,Y_WATT,1, 1},
  {0,0,0,0},
  { X_DIST,           Y_DIST, 1, 1}, //Distn
  { X_DIST+VERD18W,   Y_DIST, 1, 1},
  { X_DIST+VERD18W*2, Y_DIST, 1, 1},
  { X_DIST+VERD18W*3, Y_DIST, 1, 1},
  { X_DIST+VERD18W*4, Y_DIST, 1, 1},
  { 0,0,0,0},
  { X_TIME, 10, 1, 2},  //H
  { X_TIME+VERD22W*2,    9, 1, 2},  //:
  { X_TIME+VERD22W*3-16, 10, 1, 2},  //M
  { X_TIME+VERD22W*4-16, 10, 1, 2},  //M
  { X_TIME+VERD22W*5-17, 9, 1, 2},  //:
  { X_TIME+VERD22W*6-32, 10, 1, 2},  //S
  { X_TIME+VERD22W*7-32, 10, 1, 2},  //S
  { 0,0,0,0},  //.
  { X_TIME+VERD22W*8-32, 19, 1, 1},  //S
};

// char "SM m:ss A:5m WWW 12345 H:MI:SS"
//          888                                          888             
//          888                                          888             
//          888                                          888             
//  .d88b.  888  .d88b.  88888b.d88b.   .d88b.  88888b.  888888 .d8888b  
// d8P  Y8b 888 d8P  Y8b 888 "888 "88b d8P  Y8b 888 "88b 888    88K      
// 88888888 888 88888888 888  888  888 88888888 888  888 888    "Y8888b. 
// Y8b.     888 Y8b.     888  888  888 Y8b.     888  888 Y88b.       X88 
//  "Y8888  888  "Y8888  888  888  888  "Y8888  888  888  "Y888  88888P' 
                                                                      
TFT_eSprite spr_chr = TFT_eSprite(&tft);

void update_elements() {
    //       Time   str splt  dist aspl watts
  // char "SM m:ss A:5m WWW 12345 H:MI:SS"
  //       123456789012345678901234567890
  sprintf(stats_curr,"%02d %01d:%02d %1d:%02d %3.0f %5.0f %1d:%02d:%04.1f"
    , (int) (60.0/stroke_vector_avg)
    , split_minutes,  split_secs
    , asplit_minutes, asplit_secs
    , power_vector_avg // Watts
    , curr_stat.distance
    , row_hours, row_minutes, row_secs
  );
}


void draw_elements(){
  int i; int c; int ch=0;


  for (i=0, c=0; (i<= 32) && (c<4); i++) {

    if (stats_disp[i] != stats_curr[i]) {
      ch=stats_curr[i];
      stats_disp[i] = ch;
      c++;
      if(disp_loc[i][2]>1) {
        tft.setCursor(disp_loc[i][0], disp_loc[i][1]);
        tft.setTextSize(disp_loc[i][2]);
        tft.print(ch);
      }
      if(disp_loc[i][2]==1){
        if       (disp_loc[i][3]==1) {
          spr_chr.createSprite(35,35);
          spr_chr.setFreeFont(VERD18);
        }else if (disp_loc[i][3]==2) {
          spr_chr.createSprite(45,45);
          spr_chr.setFreeFont(VERD22);
        }else if (disp_loc[i][3]==3) {
          spr_chr.createSprite(70,75);
          spr_chr.setFreeFont(VERD36);
        }
        // spr.createSprite(65,51);
        // spr.setFreeFont(VERD22);
        spr_chr.fillSprite(TFT_ORANGE);
        spr_chr.drawRect(0,0,31,31,TFT_ORANGE);
        spr_chr.setTextColor(TFT_WHITE,TFT_BLACK);
        if ((ch>='0')&&(ch<='9'))   spr_chr.drawNumber(ch-'0', 0, 0);
        else if (ch == ':')         spr_chr.drawString(":", 0, 0);
        spr_chr.pushSprite(disp_loc[i][0], disp_loc[i][1],TFT_ORANGE);  
        spr_chr.deleteSprite();
      }
    }
  }
}


void setup_display(){
  // int c;
  tft.init();


  tft.setRotation(2);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
  tft.setTextSize(2);

  //touch_calibrate();
 
  powergraph_setup();
  forcegraph_setup();

  xTaskCreatePinnedToCore(core0_handler, "Display", 10000, NULL, 0, &Display_t_handle, DISPLAY_CPU);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK); 
  tft.setTextSize(2);
  tft.setCursor(X_SM+68,Y_SM+0);
  tft.print('s');
  tft.setCursor(X_SM+68+8,Y_SM+9);
  tft.print('/');
  tft.setCursor(X_SM+68+18,Y_SM+12);
  tft.print('m');
  tft.setCursor(X_SPLIT+105,140);
  tft.printf("/500m");
  tft.setCursor(X_ASPLIT+10,120);
  tft.printf("Avg Splt");
  tft.setCursor(X_DIST+133,Y_DIST+14);
  tft.print('m');
  tft.setCursor(X_WATT+90,Y_WATT + 13);
  tft.print('W');




  sprintf(stats_curr,"%2.0f %1d.%02.0f %1d.%02.0f %3.0f %5.0f %1d:%02d:%03.1f",
                       0.0,  9,   99.0,  9,   99.0,  0.0,  0.0,  0,  0,  0.0);
  sprintf(stats_disp,"%2.0f %1d.%02.0f %1d.%02.0f %3.0f %5.0f %1d:%02d:%03.1f",0.0,9,99.0,9,99.0,0.0,0.0,0,0,0.0);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
};



// 888                             888 888                  
// 888                             888 888                  
// 888                             888 888                  
// 88888b.   8888b.  88888b.   .d88888 888  .d88b.  888d888 
// 888 "88b     "88b 888 "88b d88" 888 888 d8P  Y8b 888P"   
// 888  888 .d888888 888  888 888  888 888 88888888 888     
// 888  888 888  888 888  888 Y88b 888 888 Y8b.     888     
// 888  888 "Y888888 888  888  "Y88888 888  "Y8888  888   

void core0_handler(void *param){
  unsigned long t_real, t_last;
  int tic = 0;

  t_last = millis();

  while(true){
    t_real = millis();
    delay(5);
    draw_elements();
    forcegraph_draw();
    //touch_events();

    if ((t_real - t_last) >= 0.1 Seconds) {
      t_last += 100;
      if (++tic == 10) tic=0;
    
      if (rowing) {
        curr_stat.elapsed+=0.1;

        if (tic==0) {
#ifdef BLE
          send_BLE();
#endif
          powergraph_plot(curr_stat.watts, curr_stat.spm);
          powergraph_scroll();
          powergraph_draw();
        }

        row_secs         +=0.1;
        if (row_secs >= 60) {
          stats_disp[27]='X'; // make the display think that drawing a : is necessary - because it's different
          row_secs-=60.0;
          if (++row_minutes > 60) {
            stats_disp[24]='X';
            row_hours++;
            row_minutes -= 60;
            if (row_hours > 9)
              row_hours = 0;
          }
        }
        sprintf(stats_curr+17,"%5.0f %1d:%02d:%04.1f", curr_stat.distance, row_hours, row_minutes, row_secs);
      }
    }
  }
};


//                   888 d8b 888                       888             
//                   888 Y8P 888                       888             
//                   888     888                       888             
//  .d8888b  8888b.  888 888 88888b.  888d888  8888b.  888888  .d88b.  
// d88P"        "88b 888 888 888 "88b 888P"       "88b 888    d8P  Y8b 
// 888      .d888888 888 888 888  888 888     .d888888 888    88888888 
// Y88b.    888  888 888 888 888 d88P 888     888  888 Y88b.  Y8b.     
//  "Y8888P "Y888888 888 888 88888P"  888     "Y888888  "Y888  "Y8888  


// void touch_calibrate()
// {
//   uint16_t calData[5];
//   uint8_t calDataOK = 0;

//   // check if calibration file exists and size is correct
//   if (SPIFFS.exists(CALIBRATION_FILE)) {
//     File f = SPIFFS.open(CALIBRATION_FILE, "r");
//     if (f) {
//       if (f.readBytes((char *)calData, 14) == 14)
//         calDataOK = 1;
//       f.close();
//     }
//   }

//   if (calDataOK) {
//     // calibration data valid
//     tft.setTouch(calData);
//   } else {
//     // data not valid so recalibrate
//     tft.fillScreen(TFT_BLACK);
//     tft.setCursor(20, 0);
//     tft.setTextFont(2);
//     tft.setTextSize(1);
//     tft.setTextColor(TFT_WHITE, TFT_BLACK);

//     tft.println("Touch corners as indicated");

//     tft.setTextFont(1);
//     tft.println();

//     tft.calibrateTouch(calData, TFT_MAGENTA, TFT_BLACK, 15);

//     tft.setTextColor(TFT_GREEN, TFT_BLACK);
//     tft.println("Calibration complete!");

//     // store data
//     File f = SPIFFS.open(CALIBRATION_FILE, "w");
//     if (f) {
//       f.write((const unsigned char *)calData, 14);
//       f.close();
//     }
//   }
// };

// void recalibrate(){
//       // Delete if we want to re-calibrate
//       SPIFFS.remove(CALIBRATION_FILE);
// };


