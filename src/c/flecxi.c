#include <pebble.h>

static GColor s_background_color;
static GColor s_foreground_color;

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_steps_layer;
static TextLayer *s_sleep_layer;
static TextLayer *s_restful_sleep_layer;

static void set_config(){
  s_background_color = GColorBlack;
  s_foreground_color = GColorWhite;
}

static void main_window_load(Window *window) {
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  // Create the TextLayer with specific bounds
  s_time_layer = text_layer_create(GRect(0, 52, bounds.size.w, 50));
  // Improve the layout to be more like a watchface
  text_layer_set_background_color(s_time_layer, s_background_color);
  text_layer_set_text_color(s_time_layer, s_foreground_color);
  text_layer_set_text(s_time_layer, "00:00");
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  
  // Create temperature Layer
  s_weather_layer = text_layer_create(GRect(0, 120, bounds.size.w, 25));
  // Style the text
  text_layer_set_background_color(s_weather_layer, s_background_color);
  text_layer_set_text_color(s_weather_layer, s_foreground_color);
  text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  text_layer_set_text(s_weather_layer, "Loading...");

  // Create steps Layer
  s_steps_layer = text_layer_create(GRect(0, 0, bounds.size.w/3, 25));
  // Style the text
  text_layer_set_background_color(s_steps_layer, s_background_color);
  text_layer_set_text_color(s_steps_layer, s_foreground_color);
  text_layer_set_text_alignment(s_steps_layer, GTextAlignmentCenter);
  text_layer_set_text(s_steps_layer, "0");  

  // Create sleep Layer
  s_sleep_layer = text_layer_create(GRect(bounds.size.w/3, 0, bounds.size.w/3, 25));
  // Style the text
  text_layer_set_background_color(s_sleep_layer, s_background_color);
  text_layer_set_text_color(s_sleep_layer, s_foreground_color);
  text_layer_set_text_alignment(s_sleep_layer, GTextAlignmentCenter);
  text_layer_set_text(s_sleep_layer, "1");  

  // Create restful sleep Layer
  s_restful_sleep_layer = text_layer_create(GRect(bounds.size.w/3*2, 0, bounds.size.w/3, 25));
  // Style the text
  text_layer_set_background_color(s_restful_sleep_layer, s_background_color);
  text_layer_set_text_color(s_restful_sleep_layer, s_foreground_color);
  text_layer_set_text_alignment(s_restful_sleep_layer, GTextAlignmentCenter);
  text_layer_set_text(s_restful_sleep_layer, "2");  
  
  // Add layers
  layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_steps_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_sleep_layer));
  layer_add_child(window_layer, text_layer_get_layer(s_restful_sleep_layer));
}

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  // Write the current hours and minutes into a buffer
  static char s_buffer[8];
  strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" : "%I:%M", tick_time);

  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, s_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
  
    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);
  
    // Send the message!
    app_message_outbox_send();
  }
}

static void health_update(){
  time_t start = time_start_of_today();
  time_t end = time(NULL);
  
  static char step_count_layer_buffer[8];
  int step_count = 0;
  HealthMetric step_metric = HealthMetricStepCount;
  HealthServiceAccessibilityMask step_mask = health_service_metric_accessible(step_metric, start, end);
  if (step_mask & HealthServiceAccessibilityMaskAvailable) {
    step_count += (int) health_service_sum_today(step_metric);
  }
  snprintf(step_count_layer_buffer, sizeof(step_count_layer_buffer), "%d", (int)step_count);
  text_layer_set_text(s_steps_layer, step_count_layer_buffer);
}

static void sleep_update(){
  time_t start_today = time_start_of_today();
  time_t end = time(NULL);

  static char sleep_layer_buffer[8];
  int sleep_time = 0;
  HealthMetric sleep_metric = HealthMetricSleepSeconds;
  HealthServiceAccessibilityMask sleep_mask = health_service_metric_accessible(sleep_metric, start_today, end);
  if (sleep_mask & HealthServiceAccessibilityMaskAvailable) {
    sleep_time += (int) health_service_sum_today(sleep_metric);
  }
  float sleep_hours = (float) sleep_time / SECONDS_PER_HOUR;
  snprintf(sleep_layer_buffer, sizeof(sleep_layer_buffer), "%d.%d", (int)sleep_hours, (int)(sleep_hours * 10) % 10);
  text_layer_set_text(s_sleep_layer, sleep_layer_buffer);

  static char restful_sleep_layer_buffer[8];
  int restful_sleep_time = 0;
  HealthMetric resting_sleep_metric = HealthMetricSleepRestfulSeconds;
  HealthServiceAccessibilityMask resting_sleep_mask = health_service_metric_accessible(resting_sleep_metric, start_today, end);
  if (resting_sleep_mask & HealthServiceAccessibilityMaskAvailable) {
    restful_sleep_time += (int) health_service_sum_today(resting_sleep_metric);
  }
  float restful_sleep_hours = (float) restful_sleep_time / SECONDS_PER_HOUR;
  snprintf(restful_sleep_layer_buffer, sizeof(restful_sleep_layer_buffer), "%d.%d", (int)restful_sleep_hours, (int)(restful_sleep_hours * 10) % 10);
  text_layer_set_text(s_restful_sleep_layer, restful_sleep_layer_buffer);  
}

static void health_handler(HealthEventType event, void *context) {
  switch(event) {
    case HealthEventSignificantUpdate:
      sleep_update();
      health_update();
      break;
    case HealthEventMovementUpdate:
      health_update();
      break;
    case HealthEventSleepUpdate:
      sleep_update();
      break;
    case HealthEventHeartRateUpdate:
      // Not supported
      break;
    case HealthEventMetricAlert:
      // Not supported
      break;    
  }
}

static void main_window_unload(Window *window) {
  // Destroy TextLayers
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_weather_layer);
  text_layer_destroy(s_steps_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char location_buffer[32];
  static char weather_layer_buffer[64];
  
  // Read tuples for data
  Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
  Tuple *location_tuple = dict_find(iterator, MESSAGE_KEY_LOCATION);
  
  // If all data is available, use it
  if(temp_tuple && conditions_tuple) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)temp_tuple->value->int32);
    snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
    snprintf(location_buffer, sizeof(location_buffer), "%s", location_tuple->value->cstring);

    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s, %s", location_buffer, temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init(void) {
  // Set constant configuration
  set_config();
  
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Set window background color
  window_set_background_color(s_main_window, s_background_color);
  
  // Make sure the time is displayed from the start
  update_time();
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  const int inbox_size = 128;
  const int outbox_size = 128;
  app_message_open(inbox_size, outbox_size);
  
  #if defined(PBL_HEALTH)
  // Attempt to subscribe 
  if(!health_service_events_subscribe(health_handler, NULL)) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
  }
  #else
  APP_LOG(APP_LOG_LEVEL_ERROR, "Health not available!");
  #endif
}

static void deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
