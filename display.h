// Display
extern void force_graph_plot(int force);
extern void force_graph_ready();

extern void update_elements();

extern void setup_display(); // setup - called from main setup
extern void handle_display( void *param); // display and touch handler
extern void touch_events(); // read and process touch events

extern void stroke_score_plot();
