// http://www.barth-dev.de/online/rgb565-color-picker/  

#include "SPI.h"
#include "TFT_eSPI.h"

#include "WiFi.h"

#include "globals.h"
char stats_curr[36];  // char "H:MI:SS SM m:ss 12345 A:5m WWW"
char stats_disp[36];  //       123456789012345678901234567890

#include "ble.h" // keep time critical stuff on processor 0

int row_hours;
int row_minutes;
double row_secs;


#define GFXFF 1
#include "verdanab18.h"
#define VERD18 &verdanab18
#define VERD22 &verdanab22
#define VERD36 &verdanab36
#define VERD18W 25
#define VERD22W 30
#define VERD36W 55



#define CALIBRATION_FILE "/TouchCalData"
TaskHandle_t Display_t_handle;
void display_loop(void *param);

TFT_eSPI tft = TFT_eSPI((int16_t) 240, (int16_t) 320);
                              
TFT_eSprite spr_chr = TFT_eSprite(&tft);
TFT_eSprite spr_char = TFT_eSprite(&tft);

int last_font;
void char_draw(char ch, int fnt, int colour, int x, int y){
  if(fnt==0) {
    tft.setCursor(x, y);
    tft.setTextColor(colour, TFT_BLACK); 
    tft.setTextSize(2);
    tft.print(ch);
    return;
  }

  switch(fnt) {
    case 1:
      spr_char.createSprite(35,35);
      spr_char.setFreeFont(VERD18);
      break;
    case 2:
      spr_char.createSprite(45,45);
      spr_char.setFreeFont(VERD22);
      break;
    case 3:
      spr_char.createSprite(70,75);
      spr_char.setFreeFont(VERD36);
  }
  spr_char.fillSprite(TFT_ORANGE);
  spr_char.drawRect(0,0,31,31,TFT_ORANGE);
  spr_char.setTextColor(colour,TFT_BLACK);
  if ((ch>='0')&&(ch<='9'))   spr_char.drawNumber(ch-'0', 0, 0);
  else if (ch == ':')         spr_char.drawString(":", 0, 0);
  spr_char.pushSprite(x, y,TFT_ORANGE);  
  spr_char.deleteSprite();
}

                                                                                    
//  #####    ####   #    #  ######  #####        ####   #####     ##    #####   #    # 
//  #    #  #    #  #    #  #       #    #      #    #  #    #   #  #   #    #  #    # 
//  #    #  #    #  #    #  #####   #    #      #       #    #  #    #  #    #  ###### 
//  #####   #    #  # ## #  #       #####       #  ###  #####   ######  #####   #    # 
//  #       #    #  ##  ##  #       #   #       #    #  #   #   #    #  #       #    # 
//  #        ####   #    #  ######  #    #       ####   #    #  #    #  #       #    # 


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

void powergraph_draw(){
  // if (powergraph.draw)
  if (pg_dirty)
    pg_sprite.pushSprite(PG_X,PG_Y);
  pg_dirty = 0;
}

                                                                                    
//  ######   ####   #####    ####   ######       ####   #####     ##    #####   #    # 
//  #       #    #  #    #  #    #  #           #    #  #    #   #  #   #    #  #    # 
//  #####   #    #  #    #  #       #####       #       #    #  #    #  #    #  ###### 
//  #       #    #  #####   #       #           #  ###  #####   ######  #####   #    # 
//  #       #    #  #   #   #    #  #           #    #  #   #   #    #  #       #    # 
//  #        ####   #    #   ####   ######       ####   #    #  #    #  #       #    # 
                                          



#define FG_X 42
#define FG_Y 305
#define FG_H 160
#define FG_W 250
#define force_graph_maxy 2400
#define force_scale_y force_graph_maxy/FG_H


//   480 total
//   <space 60> | <graphx_min 17>  300 wide (-17)  |   260+17
#define GRAPHX_MIN 17
#define GRAPHX_MAX 243
#define FG_X_SCALE 10

// Force Graph
#define FORCE_STROKES 10

volatile int    fg_stroke_draw;
volatile int    fg_stroke_ind;
int ss_dirty =0;
int stats_dirty = 0;
int fg_stroke;
double fga[FORCE_STROKES][MAX_STROKE_LEN];

#define GRAPH_GRID TFT_DARKGREY  
#define GRAPH_AXIS TFT_BLUE
#define GRAPH_LINE TFT_CYAN

#define FORCE_SCALE_X 250.0

TFT_eSprite ss_sprite = TFT_eSprite(&tft);
uint16_t fg_palette[16];

void forcegraph_setup() {
  int d,s;

  tft.drawLine(FG_X-1,FG_Y+FG_H+2,FG_X+FG_W,FG_Y+FG_H+2   ,GRAPH_AXIS);
  tft.drawLine(FG_X-1,FG_Y     +1,FG_X-1   ,FG_Y+FG_H     ,GRAPH_AXIS);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
  tft.setTextSize(2);

  tft.setCursor(10,360);
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
  fg_stroke_draw = 0;
  fg_stroke_ind = 0;

  for(d = 0; d< MAX_STROKE_LEN; d++)
    for(s = 0; s< FORCE_STROKES; s++)
      fga[s][d]=FG_H;

};



// Next stroke starting
void force_graph_ready(){
  
  // clear the rest of this strokes line data
  while (fg_stroke_ind < MAX_STROKE_LEN) {
    fga[fg_stroke][fg_stroke_ind++] = FG_H;
  }

  fg_stroke_ind = 0; 
  fg_stroke_draw = 0;
  if (++fg_stroke == FORCE_STROKES) fg_stroke = 0;

};


void force_graph_plot(int force){
  int force_y;

  // save the force for plotting, and index
  force_y = (int) (force / (force_scale_y));
  if (force_y > FG_H) 
    force_y = FG_H;

  fga[fg_stroke][fg_stroke_ind]=FG_H - force_y;
  fg_stroke_ind++;
};


void force_graph_draw(){
  int x1,y1,x2,y2;
  int d,s,ss;

  if(!rowing) return;

  // start of a new stroke
  // cycle through previous strokes
  // redraw from black through to light grey
  // effectively fade out the oldest stroke

  if (fg_stroke_draw == 0) {
    for(d = 0; d<MAX_STROKE_LEN-1; d++) {
      for(s = 0, ss=fg_stroke; s< FORCE_STROKES; s++, ss++) {
        if (ss == FORCE_STROKES) ss = 0;
        if(fga[ss][d]+fga[ss][d+1]< (2*FG_H))  {
          tft.drawLine(FG_X+d*FG_X_SCALE, FG_Y+fga[ss][d], FG_X+d*FG_X_SCALE+FG_X_SCALE, FG_Y+fga[ss][d+1], fg_palette[s]);
        }
      }
    }
    fg_stroke_draw = 1;
  }
  
  while (fg_stroke_draw < fg_stroke_ind) {
    y1 = fga[fg_stroke][fg_stroke_draw-1] + FG_Y;
    y2 = fga[fg_stroke][fg_stroke_draw  ] + FG_Y;
    if (y1+y2<((FG_Y+FG_H)*2)) {
      x1 = (fg_stroke_draw-1)*FG_X_SCALE        + FG_X;
      x2 = x1+FG_X_SCALE;
      tft.drawLine      (x1, y1, x2, y2, TFT_YELLOW);
    }
    fg_stroke_draw++;
  }
};

//   ####   #####  #####    ####   #    #  ######       ####    ####    ####   #####   ###### 
//  #         #    #    #  #    #  #   #   #           #       #    #  #    #  #    #  #      
//   ####     #    #    #  #    #  ####    #####        ####   #       #    #  #    #  #####  
//       #    #    #####   #    #  #  #    #                #  #       #    #  #####   #      
//  #    #    #    #   #   #    #  #   #   #           #    #  #    #  #    #  #   #   #      
//   ####     #    #    #   ####   #    #  ######       ####    ####    ####   #    #  ###### 

void stroke_score_setup(){
  ss_sprite.createSprite(100,88);
  ss_sprite.setColorDepth(4);
  ss_sprite.createPalette(fg_palette);
  ss_sprite.pushSprite(219,288);
}

void stroke_score_plot(){
  int fx,fy,bx,by;

#define FG_BLOB 8

  ss_sprite.fillRect(0,0,99,87,0);
  ss_sprite.drawLine(49,0,49,87,9);
  ss_sprite.drawLine(0,80-7*8,98,80-7*8,14);

  fx = 46-(int)(last_score.spread*4)-(int)(last_score.offset*5);
  fy = 80-(int)(last_score.f_eff*8);
  bx = 54+(int)(last_score.spread*4)-(int)(last_score.offset*5);
  by = 80-(int)(last_score.b_eff*8);
  ss_sprite.fillRect(fx,fy,FG_BLOB,FG_BLOB,9);
  ss_sprite.fillRect(bx,bx,FG_BLOB,FG_BLOB,9);
  ss_sprite.drawLine(fx+4,fy+4,bx+4,by+4,6);

  fx = 46-(int)(aver_score.spread*4)-(int)(aver_score.offset*5);
  fy = 80-(int)(aver_score.f_eff*8);
  bx = 54+(int)(aver_score.spread*4)-(int)(aver_score.offset*5);
  by = 80-(int)(aver_score.b_eff*8);
  ss_sprite.fillRect(fx,fy,FG_BLOB,FG_BLOB,(int)(10.4+(aver_score.f_eff/2)));
  ss_sprite.fillRect(bx,bx,FG_BLOB,FG_BLOB,(int)(10.4+(aver_score.b_eff/2)));
  ss_sprite.drawLine(fx+4,fy+4,bx+4,by+4,6);
  
  fx = 46-(int)(curr_score.spread*4)-(int)(curr_score.offset*5);
  fy = 80-(int)(curr_score.f_eff*8);
  bx = 54+(int)(curr_score.spread*4)-(int)(curr_score.offset*5);
  by = 80-(int)(curr_score.b_eff*8);
  ss_sprite.drawRect(fx,fy,FG_BLOB,FG_BLOB,(int)(10.4+(curr_score.f_eff/2)));
  ss_sprite.drawRect(bx,by,FG_BLOB,FG_BLOB,(int)(10.4+(curr_score.b_eff/2)));
  ss_sprite.drawRect((fx+bx)/2,(fy+by)/2,5,5,9);
  ss_sprite.drawLine(fx+4,fy+4,bx+4,by+4,(int)(10.4+(curr_score.f_eff+curr_score.b_eff)/4));
  
  ss_dirty = 1;
};

void stroke_score_draw(){
  if (ss_dirty) {
    ss_sprite.pushSprite(219,288);
    ss_dirty = 0;
  }
}


                                     
//   ####   #####    ##    #####   ####  
//  #         #     #  #     #    #      
//   ####     #    #    #    #     ####  
//       #    #    ######    #         # 
//  #    #    #    #    #    #    #    # 
//   ####     #    #    #    #     ####  

#define X_TIME 5  // 160
#define Y_TIME 9  // 54

int stopwatch_chrs[][3] = {
  { VERD22W*6-30,20, 0},  //mS
  { VERD22W*5-33, 1, 2},  //S
  { VERD22W*4-33, 1, 2},  //S
  { VERD22W*2-16, 1, 2},  //M
  { VERD22W*1-16, 1, 2},  //M
  { 1           , 3, 0},  //H
  { VERD22W*3-17, 0, 2}   //:
  //{ VERD22W*2,    0, 2},  //:
};

void time_draw(){
  int c;
  for (c=0;c<7;c++) {
    if (stopwatch[c]>stopwatch_max[c]){
      stopwatch[c+1]++;
      stopwatch[c]='0';
    }
    if (stopwatch[c]!=stopwatch_disp[c]){
      stopwatch_disp[c] = stopwatch[c];
      char_draw(stopwatch[c],stopwatch_chrs[c][2],TFT_WHITE,X_TIME+stopwatch_chrs[c][0],Y_TIME+stopwatch_chrs[c][1]);
    }
  }
};

#define X_DIST 175
#define Y_DIST 17

int distance_chrs[][3] = {
  { VERD18W*0, 0, 1},
  { VERD18W*1, 0, 1},
  { VERD18W*2, 0, 1},
  { VERD18W*3, 0, 1},
  { VERD18W*4, 0, 1}
};
char distance_disp[6];
char distance_now[6];

void distance_draw(){
  int c;
  for (c=0;c<5;c++) {
    if (distance_now[c]!=distance_disp[c]){
      distance_disp[c] = distance_now[c];
      char_draw(distance_now[c],distance_chrs[c][2],TFT_YELLOW,X_DIST+distance_chrs[c][0],Y_DIST+distance_chrs[c][1]);
    }
  }
}


#define X_SPLIT 10
#define X_ASPLIT 210
#define Y_SPLITS 72

#define X_WATT 210
#define Y_WATT 210

#define X_SM 210
#define Y_SM 255



static int disp_loc[][4] = {
  { X_SM,        Y_SM,1,1},  //S
  { X_SM+VERD18W,Y_SM,1,1},  //M
  { 0,0,0,0},
  { X_SPLIT             ,Y_SPLITS+3, 1,3}, //split m
  { X_SPLIT+VERD36W-10  ,Y_SPLITS  , 1,3}, //:
  { X_SPLIT+VERD36W*2-40,Y_SPLITS+3, 1,3}, //s
  { X_SPLIT+VERD36W*3-44,Y_SPLITS+3, 1,3}, //s
  { 0,0,0,0},
  { X_ASPLIT             ,Y_SPLITS+3, 1, 2}, //Asplit m
  { X_ASPLIT+VERD22W  -2 ,Y_SPLITS+2, 1, 2}, //:
  { X_ASPLIT+VERD22W*2-16,Y_SPLITS+3, 1, 2}, //ss
  { X_ASPLIT+VERD22W*3-16,Y_SPLITS+3, 1, 2},
  {0,0,0,0},
  {X_WATT,          Y_WATT,1, 1}, //WWW
  {X_WATT+VERD18W,  Y_WATT,1, 1},
  {X_WATT+VERD18W*2,Y_WATT,1, 1}
};

void elements_draw(){
  int i; int c; int ch=0;

  if (stats_dirty==0) return;
  
  stats_dirty = 0;

  distance_draw();

  for (i=0, c=0; (i<= 16) && (c<4); i++) { //32 for time not 23

    if (stats_disp[i] != stats_curr[i]) {
      ch=stats_curr[i];
      stats_disp[i] = ch;
      c++;
      char_draw(ch, disp_loc[i][3], TFT_WHITE, disp_loc[i][0], disp_loc[i][1]);
    }
  }
}

void update_stats(int level) {
  int split_minutes;
  int split_secs;
  int asplit_minutes;
  int asplit_secs;

  stats_dirty = 1;

  if (level==2){
    // format split
    split_minutes = stats.split_secs/60;
    split_secs   =  stats.split_secs -split_minutes*60;

    asplit_minutes = stats.asplit_secs/60;
    asplit_secs    = stats.asplit_secs - asplit_minutes*60;

    sprintf(stats_curr,"%02d %01d:%02d %1d:%02d %3d"
      , (int) stats.spm
      , split_minutes,  split_secs
      , asplit_minutes, asplit_secs
      , stats.watts);
  }
  if (level==1)
    sprintf(distance_now, "%5.0f", stats.distance);
}



//   ####   ######  #####  #    #  #####       #####   #   ####   #####   #         ##    #   # 
//  #       #         #    #    #  #    #      #    #  #  #       #    #  #        #  #    # #  
//   ####   #####     #    #    #  #    #      #    #  #   ####   #    #  #       #    #    #   
//       #  #         #    #    #  #####       #    #  #       #  #####   #       ######    #   
//  #    #  #         #    #    #  #           #    #  #  #    #  #       #       #    #    #   
//   ####   ######    #     ####   #           #####   #   ####   #       ######  #    #    #   

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
  stroke_score_setup();

  xTaskCreatePinnedToCore(display_loop, "Display", 10000, NULL, 0, &Display_t_handle, DISPLAY_CPU);

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
  tft.setCursor(X_DIST+130,Y_DIST+14);
  tft.print('m');
  tft.setCursor(X_WATT+90,Y_WATT + 13);
  tft.print('W');


  sprintf(distance_disp,"     ");
  sprintf(stats_curr,"%2.0f %1d.%02.0f %1d.%02.0f %3.0f",0.0,9,99.0,9,99.0,0.0);
  sprintf(stats_disp,"%2.0f %1d.%02.0f %1d.%02.0f %3.0f",0.0,9,99.0,9,99.0,0.0);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
};


                                                                                           
//  #####   #   ####   #####   #         ##    #   #           #        ####    ####   #####  
//  #    #  #  #       #    #  #        #  #    # #            #       #    #  #    #  #    # 
//  #    #  #   ####   #    #  #       #    #    #             #       #    #  #    #  #    # 
//  #    #  #       #  #####   #       ######    #             #       #    #  #    #  #####  
//  #    #  #  #    #  #       #       #    #    #             #       #    #  #    #  #      
//  #####   #   ####   #       ######  #    #    #             ######   ####    ####   #    

void display_loop(void *param){
  unsigned long t_now, t_last;
  int tic = 0;

  t_last = millis();

  while(true){
    t_now = millis();

    // update these every cycle

    delay(5);
    time_draw();
    elements_draw();
    force_graph_draw();
    stroke_score_draw();
    //touch_events();

    if ((t_now - t_last) >= 100) {
      t_last += 100;
      if (++tic == 10) {
        tic=0;
        send_BLE();
        powergraph_plot(stats.watts, stats.spm);
        powergraph_scroll();
        powergraph_draw();
      }
    }

  } // while loop
};


//   ####     ##    #       #  #####   #####     ##    #####  ###### 
//  #    #   #  #   #       #  #    #  #    #   #  #     #    #      
//  #       #    #  #       #  #####   #    #  #    #    #    #####  
//  #       ######  #       #  #    #  #####   ######    #    #      
//  #    #  #    #  #       #  #    #  #   #   #    #    #    #      
//   ####   #    #  ######  #  #####   #    #  #    #    #    ###### 


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


