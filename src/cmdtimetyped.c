#include <pebble.h>

#define TYPE_DELTA 200
#define PROMPT_DELTA 1000	
#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

//Layers
static Window *window;
static TextLayer *time_label, *time_layer, *date_label, *date_layer, *weather_label, *weather_layer;
static AppTimer *timer;

//Buffers
static char time_buffer[] = "XX:XX", date_buffer[] = "XX/XX/XX", weather_buffer[] = "xxxxxxxx, x°C";

//State
static int state = 0;

//Prototypes
static TextLayer* cl_init_text_layer(GRect location, GColor colour, GColor background, ResHandle handle, GTextAlignment alignment);

/****************************** Time Lifecycle **********************************/

static void set_time(struct tm *t)
{
  //Set time
  if(clock_is_24h_style())
  {
    strftime(time_buffer, sizeof("XX:XX"),"%H:%M", t);
  }
  else
  {
    strftime(time_buffer, sizeof("XX:XX"),"%I:%M", t);
  }
  text_layer_set_text(time_layer, time_buffer);

  //Set date
  strftime(date_buffer, sizeof("XX/XX/XXXX"), "%m/%d/%y", t);
  text_layer_set_text(date_layer, date_buffer);
}

static void set_time_anim()
{
  //Time structures -- Cannot be branch declared
  time_t temp;
  struct tm *t;

  //Finally
  state++;
}

static void tick_handler(struct tm *t, TimeUnits units_changed)
{
  app_timer_cancel(timer); 
	
  time_t temp;

  //Start anim cycle
  temp = time(NULL);    
  t = localtime(&temp);
  set_time(t);
  //timer = app_timer_register(PROMPT_DELTA, set_time_anim, 0);

  //Blank before time change
  text_layer_set_text(time_label, "p:~$ time");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));
  text_layer_set_text(date_label, "p:~$ date");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));
  text_layer_set_text(weather_label, "p:~$ weather");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(weather_layer));
      

  //Change time display
  //set_time(t);
}

/***************************** Window Lifecycle *********************************/

static void window_load(Window *window) 
{
  //Font
  ResHandle font_handle = resource_get_handle(RESOURCE_ID_FONT_LUCIDA_16);

  //Time 
  time_label = cl_init_text_layer(GRect(5, 5, 144, 30), GColorWhite, GColorClear, font_handle, GTextAlignmentLeft);
  text_layer_set_text(time_label, "");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_label));

  time_layer = cl_init_text_layer(GRect(5, 24, 144, 30), GColorWhite, GColorClear, font_handle, GTextAlignmentLeft);
  text_layer_set_text(time_layer, "");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(time_layer));

  //Date
  date_label = cl_init_text_layer(GRect(5, 53, 144, 30), GColorWhite, GColorClear, font_handle, GTextAlignmentLeft);
  text_layer_set_text(date_label, "");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_label));

  date_layer = cl_init_text_layer(GRect(5, 70, 144, 30), GColorWhite, GColorClear, font_handle, GTextAlignmentLeft);
  text_layer_set_text(date_layer, "");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(date_layer));

  //Prompt
  weather_label = cl_init_text_layer(GRect(5, 99, 144, 30), GColorWhite, GColorClear, font_handle, GTextAlignmentLeft);
  text_layer_set_text(weather_label, "");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(weather_label));

  weather_layer = cl_init_text_layer(GRect(5, 116, 144, 30), GColorWhite, GColorClear, font_handle, GTextAlignmentLeft);
  text_layer_set_text(weather_layer, "");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(weather_layer));
}

static void window_unload(Window *window) 
{
  //Time
  text_layer_destroy(time_label);
  text_layer_destroy(time_layer);

  //Date
  text_layer_destroy(date_label);
  text_layer_destroy(date_layer);

  //Prompt
  text_layer_destroy(weather_label);
  text_layer_destroy(weather_layer);
}

/******************************** App Lifecycle *********************************/
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[8];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_TEMPERATURE:
      snprintf(temperature_buffer, sizeof(temperature_buffer), "%d °C", (int)t->value->int32);
      break;
    case KEY_CONDITIONS:
      snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  
  // Assemble full string and display
  snprintf(weather_buffer, sizeof(weather_layer_buffer), "%s, %s", conditions_buffer, temperature_buffer);
  text_layer_set_text(weather_layer, weather_buffer);
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


static void init(void) 
{
  window = window_create();
  WindowHandlers handlers = {
    .load = window_load,
    .unload = window_unload
  };
  window_set_window_handlers(window, handlers);
  window_set_background_color(window, GColorBlack);

  //Get tick events
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  //Finally
  window_stack_push(window, true);
	// Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
	
}

static void deinit(void) 
{
  tick_timer_service_unsubscribe();

  window_destroy(window);
}

int main(void) 
{
  init();
  app_event_loop();
  deinit();
}

/****************************** Other functions *********************************/

static TextLayer* cl_init_text_layer(GRect location, GColor colour, GColor background, ResHandle handle, GTextAlignment alignment)
{
  TextLayer *layer = text_layer_create(location);
  text_layer_set_text_color(layer, colour);
  text_layer_set_background_color(layer, background);
  text_layer_set_font(layer, fonts_load_custom_font(handle));
  text_layer_set_text_alignment(layer, alignment);

  return layer;
}
