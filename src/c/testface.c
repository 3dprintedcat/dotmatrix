#include <pebble.h>
#include <string.h> // Needed for memmove

// Pointers for the main window and text layers.
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_battery_layer;

// Custom fonts.
static GFont s_custom_font;       // For time display.
static GFont s_small_font;        // For battery percentage.

// Update the time display.
static void update_time() {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);
  
  // Buffer must be large enough for the time string.
  static char buffer[] = "00:00";
  
  if(clock_is_24h_style()) {
    strftime(buffer, sizeof(buffer), "%H:%M", tick_time);
  } else {
    strftime(buffer, sizeof(buffer), "%I:%M", tick_time);
    // Remove leading zero if present in 12-hour mode.
    if(buffer[0] == '0') {
      memmove(buffer, buffer + 1, strlen(buffer));
    }
  }
  
  text_layer_set_text(s_time_layer, buffer);
}

// Update the battery display.
static void update_battery() {
  BatteryChargeState state = battery_state_service_peek();
  static char battery_buffer[8]; // Enough to store "100%"
  
  snprintf(battery_buffer, sizeof(battery_buffer), "%d%%", state.charge_percent);
  text_layer_set_text(s_battery_layer, battery_buffer);
}

// Battery state callback.
static void battery_callback(BatteryChargeState state) {
  update_battery();
}

// Tick handler to update the time every minute.
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

// Main window load handler.
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  // Create the time text layer.
  #ifdef PBL_ROUND
    // For round devices, center the time layer vertically.
    s_time_layer = text_layer_create(GRect(0, bounds.size.h/2 - 20, bounds.size.w, 60));
  #else
    s_time_layer = text_layer_create(GRect(0, 60, 144, 50));
  #endif
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, GColorBlack);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Load the larger custom font for time display.
  s_custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_NDOT_45));
  text_layer_set_font(s_time_layer, s_custom_font);
  
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  
  // Create the battery percentage text layer.
  #ifdef PBL_ROUND
    // For round watches, position it slightly higher from the bottom.
    s_battery_layer = text_layer_create(GRect(0, bounds.size.h - 35, bounds.size.w, 30));
  #else
    s_battery_layer = text_layer_create(GRect(0, bounds.size.h - 25, bounds.size.w, 20));
  #endif
  text_layer_set_background_color(s_battery_layer, GColorClear);
  text_layer_set_text_color(s_battery_layer, GColorBlack);
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
  
  // Load the smaller custom font for battery percentage.
  s_small_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_NDOT_20));
  text_layer_set_font(s_battery_layer, s_small_font);
  
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  
  // Set initial values.
  update_time();
  update_battery();
}

// Main window unload handler.
static void main_window_unload(Window *window) {
  fonts_unload_custom_font(s_custom_font);
  fonts_unload_custom_font(s_small_font);
  
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_battery_layer);
}

// Initialize the watchface.
static void init() {
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });
  
  window_stack_push(s_main_window, true);
  
  // Subscribe to services.
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(battery_callback);
}

// Deinitialize the watchface.
static void deinit() {
  window_destroy(s_main_window);
  battery_state_service_unsubscribe();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
  return 0;
}


