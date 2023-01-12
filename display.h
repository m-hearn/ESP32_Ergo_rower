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

#define GRAPH_GRID TFT_DARKGREY  
#define GRAPH_AXIS TFT_BLUE
#define GRAPH_LINE TFT_CYAN

#define FORCE_SCALE_X 250.0

#define X_SPLIT 10
#define X_DIST 170
#define Y_DIST 170
#define X_ASPLIT 210
#define X_WATT 210
#define Y_WATT 210
#define X_SM 210
#define Y_SM 255
#define PG_Y 210


#define CALIBRATION_FILE "/TouchCalData"
TaskHandle_t Display_t_handle;
void core0_handler(void *param);

TFT_eSPI tft = TFT_eSPI((int16_t) 240, (int16_t) 320);

TFT_eSprite spr_chr = TFT_eSprite(&tft);

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
    pg_sprite.pushSprite(5,PG_Y);
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



#define X_TIME 10

static int FG_X = 50;
static int FG_Y = 465;
static int FG_H = 160;
static int FG_W = 248;
static int force_graph_maxy = 2400;
static double force_scale_y = 10.0;

void forcegraph_setup() {

  tft.drawLine(FG_X-1,FG_Y+1,FG_X+248,FG_Y+1,GRAPH_AXIS);
  tft.drawLine(FG_X-1,FG_Y+1,FG_X-1,  FG_Y- FG_H,GRAPH_AXIS);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
  tft.setTextSize(2);

  tft.setCursor(18,360);
  tft.printf("Kg");

  force_scale_y = force_graph_maxy / (double) FG_H;
  force_max = ZERO;   

}


//   480 total
//   <space 60> | <graphx_min 17>  300 wide (-17)  |   260+17
#define GRAPHX_MIN 17
#define GRAPHX_MAX 243

int last_graphp = 1;


void forcegraph_draw(){

  if(!rowing) return;

  if(force_line == -1) {
    // starting new stroke
    force_line = 0;
    return;
  }
  
  while (force_line < force_ptr) {
    if (force_graph[force_line][1]> force_graph_maxy) force_graph[force_line][1] = force_graph_maxy;
    if (  (force_graph[force_line][1]   >0)  && (force_graph[force_line+1][1] >0) ){
      tft.drawLine(FG_X+(force_line  )*10, FG_Y-(int)(force_graph[force_line+0][1]/force_scale_y),
                   FG_X+(force_line+1)*10, FG_Y-(int)(force_graph[force_line+1][1]/force_scale_y), TFT_YELLOW);
    }
    force_line++;last_graphp = force_line;
  }
};


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
                       0.0,  0,   0.0,  0,   0.0,  0.0,  0.0,  0,  0,  0.0);
  sprintf(stats_disp,"%2.0f %1d.%02.0f %1d.%02.0f %3.0f %5.0f %1d:%02d:%03.1f",0.0,0,0.0,0,0.0,0.0,0.0,0,0,0.0);

  tft.setTextColor(TFT_WHITE, TFT_BLACK); 
};




static int disp_loc[][4] = {
  { X_SM,        Y_SM,1,1},  //S
  { X_SM+VERD18W,Y_SM,1,1},  //M
  { 0,0,0,0},
  { X_SPLIT,75, 1,3}, //split m
  { X_SPLIT+VERD36W-10,75, 1,3}, //:
  { X_SPLIT+VERD36W*2-40,75, 1,3}, //s
  { X_SPLIT+VERD36W*3-44,75, 1,3}, //s
  { 0,0,0,0},
  { X_ASPLIT,75, 1, 2}, //Asplit m
  { X_ASPLIT+VERD22W  -4,75, 1, 2}, //:
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
  { X_TIME+VERD22W*2, 10, 1, 2},  //:
  { X_TIME+VERD22W*3-16, 10, 1, 2},  //M
  { X_TIME+VERD22W*4-16, 10, 1, 2},  //M
  { X_TIME+VERD22W*5-16, 10, 1, 2},  //:
  { X_TIME+VERD22W*6-32, 10, 1, 2},  //S
  { X_TIME+VERD22W*7-32, 10, 1, 2},  //S
  { 0,0,0,0},  //.
  { X_TIME+VERD22W*8-32, 19, 1, 1},  //S
};

// char "SM m:ss A:5m WWW 12345 H:MI:SS"

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




// redundant code

  // if(!rowing && last_graphp) {
  //   if (force_line == 0) {
  //     dy= 250/force_scale_y;
  //     dl=force_graph_maxy/force_scale_y;
  //     if (dl> FG_H) dl-=dy;
  //     for (; dl>=dy; dl-= dy) {
  //       tft.fillRect(FG_X, FG_Y-dl,                1+GRAPHX_MAX, dy               , TFT_BLACK);
  //       tft.drawLine(FG_X, FG_Y-dl-1,   FG_X+GRAPHX_MAX, FG_Y-dl-1, GRAPH_GRID);
  //     }
  //     force_line++;
  //   }
  // }

// if (force_line == 0)
    //   if (last_graphp>0) {
    //     for (; force_line <= last_graphp; force_line++)
    //       if (  (force_graph[force_line][1]   >10) && (force_graph[force_line+1][1] >10))
    //         tft.drawLine(FG_X+(force_line  )*10, FG_Y-(int)(force_graph[force_line+0][1]/force_scale_y),
    //                      FG_X+(force_line+1)*10, FG_Y-(int)(force_graph[force_line+1][1]/force_scale_y), TFT_DARKGREY);
          
    //   force_line=1;
    // }

// force / time graph
    // while (force_line < force_ptr) {
    //   if (  (force_graph[force_line][0]   > (GRAPHX_MIN/FORCE_SCALE_X)) 
    //      && (force_graph[force_line+1][0] <((GRAPHX_MAX+GRAPHX_MIN)/FORCE_SCALE_X))
    //      && (force_graph[force_line][1]   >10)
    //      && (force_graph[force_line+1][1] >10)
    //      ){
    //     tft.drawLine(FG_X+(int)(force_graph[force_line+0][0]*FORCE_SCALE_X)-GRAPHX_MIN, FG_Y-(int)(force_graph[force_line+0][1]/force_scale_y),
    //                   FG_X+(int)(force_graph[force_line+1][0]*FORCE_SCALE_X)-GRAPHX_MIN, FG_Y-(int)(force_graph[force_line+1][1]/force_scale_y), GRAPH_LINE);
    //   }
    //   force_line++;
    // }
    