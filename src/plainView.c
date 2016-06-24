#include <pebble.h>
//#include "config.h"
#include "text_block.h"
#include "messenger.h"

#define HOUR_HAND_COLOR GColorRed
// #define d(string, ...) APP_LOG (APP_LOG_LEVEL_DEBUG, string, ##__VA_ARGS__)
// #define e(string, ...) APP_LOG (APP_LOG_LEVEL_ERROR, string, ##__VA_ARGS__)
// #define i(string, ...) APP_LOG (APP_LOG_LEVEL_INFO, string, ##__VA_ARGS__)

#ifdef PBL_ROUND
static GPoint ticks_points[12][2] = {
  {{90, 0}  , {90, 6}  },
  {{135,12} , {132,18}  },
  {{168,45} , {162,48} },
  {{180,90} , {174,90} },
  {{168,135}, {162,132}},
  {{135,168}, {132,162}},
  {{90, 180}, {90, 174}},
  {{45, 168}, {48, 162}},
  {{12, 135}, {18, 132}},
  {{0,  90} , {6,  90} },
  {{12, 45} , {18, 48} },
  {{45, 12} , {48, 18}  }
};
static GPoint time_points[12] = {
  {90,  17} ,
  {124, 28} ,
  {150, 50} ,
  {161, 86} ,
  {148, 124},
  {124, 146},
  {90,  157},
  {54,  147},
  {29,  124},
  {18,  87} ,
  {30,  52} ,
  {54,  28} ,
};
static GPoint time_points_large[12] = {
  {90,  12} ,//12
  {124, 23} ,//1
  {154, 48} ,//2
  {163, 74} ,//3
  {150, 108},//4
  {124, 130},//5
  {92,  138},//6
  {63,  128},//7
  {33,  106},//8
  {20,  74} ,//9
  {35,  47} ,//10
  {63,  23} ,//11
};
static GPoint SOUTH_INFO_CENTER = { .x = 90, .y = 118 };
static GPoint NORTH_INFO_CENTER = { .x = 90, .y = 62 };
#else
static GPoint ticks_points[12][2] = {
  {{72, 0}  , {72, 7}  },
  {{120,0}  , {117,7}  },
  {{144,42} , {137,46} },
  {{144,84} , {137,84} },
  {{144,126}, {137,122}},
  {{120,168}, {117,161}},
  {{72, 168}, {72, 161}},
  {{24, 168}, {27, 161}},
  {{0,  126}, {7,  122}},
  {{0,  84} , {7,  84} },
  {{0,  42} , {7,  46} },
  {{24, 0}  , {27, 7}  }
};
static GPoint time_points[12] = {
  {72,  17} ,
  {112, 17} ,
  {126, 47} ,
  {126, 82},
  {126, 115},
  {112, 137} ,
  {72,  143},
  {32,  143},
  {18,  112},
  {18,  80} ,
  {22,  47} ,
  {32,  17} ,
};
static GPoint time_points_large[12] = {
  {72,  15} , //12
  {112, 15} , //1
  {124, 43} , //2
  {125, 68},  //3
  {125, 98},  //4
  {112, 124} ,//5
  {74,  126}, //6
  {42,  126}, //7
  {22,  98}, //8
  {22,  68} , //9
  {26,  43} , //10
  {36,  15} , //11
};
static GPoint SOUTH_INFO_CENTER = { .x = 72, .y = 112 };
static GPoint NORTH_INFO_CENTER = { .x = 72, .y = 56 };
#endif

typedef enum { Hour, Minute } TimeType;
static int TEST_HOUR = -1;


typedef enum {
  PersistKeyConfig = 0,
  PersistKeyWeather,
  PersistKeyCelsius,
  PersistKeyUpateRate,
  PersistKeyExpiredSecs
} PersistKey;

typedef enum {
  date,
  theWeather
} Infos;

typedef enum {no, left, right} ShiftLocation;

typedef struct {
  int hour;
  int minute;
  int day;
} Time;

typedef enum { NoIcon = 0, Bluetooth , Heart } BluetoothIcon;
typedef enum { Celsius = 0, Fahrenheit } TemperatureUnit;

typedef struct {
  int32_t timestamp;
  int8_t icon;
  int8_t temperature;
} __attribute__((__packed__)) Weather;

static const int HOUR_CIRCLE_RADIUS = 6;
static const int HOUR_HAND_STROKE = 8;
static const int HOUR_HAND_RADIUS = 45;
static const int MINUTE_HAND_STROKE = 6;
static const int MINUTE_HAND_RADIUS = 120;
static const int TICK_STROKE = 7;
static int WEATHER_UPDATE_MIN = 20;
static int WEATHER_EXPIRED_SECS = 1800;

static Window * s_main_window;
static Layer * s_root_layer;
static GRect s_root_layer_bounds;
static GPoint s_center;

static TextBlock * s_north_info;
static TextBlock * s_south_info;
static TextBlock * s_hour_text;
static TextBlock * s_nhour_text;

static Layer * s_tick_layer;

static Layer * s_minute_hand_layer;
static Layer * s_hour_hand_layer;
static Layer * s_center_circle_layer;

//static Config * s_config;
static Messenger * s_messenger;
static Weather s_weather;

static bool s_bt_connected;

static AppTimer * s_weather_request_timer;
static int s_weather_request_timeout;

static int s_js_ready;

static GFont s_font;
static GFont s_cHour_font;

static Time s_current_time;

static void update_info_layer();
static void schedule_weather_request(int timeout);
static void update_times();
static void update_date();
static void mark_dirty_minute_hand_layer();

static int s_celsius = 0;


static void update_current_time() {
  const time_t temp = time(NULL);
  const struct tm *tick_time = localtime(&temp);
  int hour = tick_time->tm_hour;
  if(hour > 12){
    hour -= 12;
  }else if(hour == 0){
    hour = 12;
  }
  s_current_time.hour   = hour;
  s_current_time.minute = tick_time->tm_min;
  s_current_time.day    = tick_time->tm_mday;
}


static GPoint gpoint_on_circle(const GPoint center, const int angle, const int radius){
  const int diameter = radius * 2;
  const GRect grect_for_polar = GRect(center.x - radius + 1, center.y - radius + 1, diameter, diameter);
  return gpoint_from_polar(grect_for_polar, GOvalScaleModeFitCircle, angle);
}

static float angle(int time, int max){
  if(time == 0 || time == max){
    return 0;
  }
  return TRIG_MAX_ANGLE * time / max;
}
static ShiftLocation dodge_hands(const Infos field){
  //hour confligs
  ShiftLocation result = no;
  //if hour or minute hands will be in the upper quarter
  const bool move_Weather = ((s_current_time.hour == 10 && s_current_time.minute > 30) ||
                      (s_current_time.hour == 1 && s_current_time.minute < 30) ||
                      s_current_time.hour >= 11 || s_current_time.hour < 1
                      || s_current_time.minute > 53 || s_current_time.minute < 7);
  //if hour or minute hands will be in the lower quarter
  const bool move_Date = ((s_current_time.hour >= 5 && s_current_time.hour < 7)
                   || (s_current_time.minute >= 25 && s_current_time.minute < 35));
  //if the hour or minute hands will be in the left quarter
  const bool left_Blocked = ((s_current_time.hour >= 8 && s_current_time.hour < 10) ||
                       (s_current_time.minute >= 40 && s_current_time.minute < 50));
  const bool choose_Right = (s_current_time.hour > 6 && (s_current_time.minute < 7 || 
                                                        s_current_time.minute > 22));
  //if the date should be moved and requested and left is blocked
  //(can't have left blocked and weather blocked) move right.
  if(move_Date && field == date){
    if(left_Blocked || (!move_Weather && choose_Right)) result = right;  
    else result = left;
  }
  //if the weather is blocked
  if(move_Weather && field == theWeather){
    if(move_Date || left_Blocked || choose_Right) result = right;
    else result = left;
  }
  
  return result;
}
static void send_weather_request_callback(void * context){
  s_weather_request_timer = NULL;
  if(s_js_ready){
    DictionaryIterator *out_iter;
    AppMessageResult result = app_message_outbox_begin(&out_iter);
    if(result == APP_MSG_OK) {
      const int value = 1;
      dict_write_int(out_iter, MESSAGE_KEY_AppKeyWeatherRequest, &value, sizeof(int), true);
      result = app_message_outbox_send();
    }
    else {
      APP_LOG(1, "app message result: %d", result);
    }
  }
  else{
    APP_LOG(0, "JS not ready");
  }
}

static void schedule_weather_request(int timeout){
  int expiration = time(NULL) + timeout/1000;
  if(s_weather_request_timer){
    if(expiration < s_weather_request_timeout){
      s_weather_request_timeout = expiration;
      app_timer_reschedule(s_weather_request_timer, timeout);
    }
  }else{
    s_weather_request_timeout = expiration;
    s_weather_request_timer = app_timer_register(timeout, send_weather_request_callback, NULL);
  }
}


static void js_ready_callback(DictionaryIterator * iter, Tuple * tuple){
  Tuple * js_ready = dict_find(iter, MESSAGE_KEY_AppKeyJsReady);
  if(js_ready){
    s_js_ready = true;
    //s_weather.timestamp = time(NULL);
    //schedule_weather_request(0);
    if(time(NULL) >= (s_weather.timestamp - 60))send_weather_request_callback(NULL);
  }
}
static void config_callback(DictionaryIterator * iter, Tuple * tuple){
  Tuple * celsius = dict_find(iter, MESSAGE_KEY_TempUnits);
  if(celsius){
    s_celsius = celsius->value->int8 - '0';
    persist_write_int(PersistKeyCelsius, s_celsius);
  }
  Tuple * updateRate = dict_find(iter, MESSAGE_KEY_WeatherUpdateRate);
  if(updateRate){
    switch(updateRate->value->int8){
      case '0':
        WEATHER_UPDATE_MIN = 0;
        WEATHER_EXPIRED_SECS = 600;
        break;
      case '1':
        WEATHER_UPDATE_MIN = 10;
        WEATHER_EXPIRED_SECS = 1200;
        break;
      case '2':
        WEATHER_UPDATE_MIN = 20;
        WEATHER_EXPIRED_SECS = 1800;
        break;
      case '3':
        WEATHER_UPDATE_MIN = 30;
        WEATHER_EXPIRED_SECS = 2400;
        break;
      case '4':
        WEATHER_UPDATE_MIN = 60;
        WEATHER_EXPIRED_SECS = 4200;
        break;
    }
    APP_LOG(APP_LOG_LEVEL_DEBUG, "weather update min: %d", WEATHER_UPDATE_MIN);
    persist_write_int(PersistKeyUpateRate, WEATHER_UPDATE_MIN);
    persist_write_int(PersistKeyExpiredSecs, WEATHER_EXPIRED_SECS);
  }
  update_info_layer();
}
static void weather_requested_callback(DictionaryIterator * iter, Tuple * tuple){
  Tuple * icon_tuple = dict_find(iter, MESSAGE_KEY_AppKeyWeatherIcon);;
  Tuple * temp_tuple = dict_find(iter, MESSAGE_KEY_AppKeyWeatherTemperature);;
  APP_LOG(APP_LOG_LEVEL_INFO, "weather received by app");
  if(icon_tuple && temp_tuple){
    //mark weather as old if it hasn't been updated in 30 mins
    s_weather.timestamp = time(NULL) + WEATHER_EXPIRED_SECS;
    s_weather.icon = icon_tuple->value->int8;
    s_weather.temperature = temp_tuple->value->int8;
    //counter=!counter;
  }
  persist_write_data(PersistKeyWeather, &s_weather, sizeof(Weather));
  //schedule_weather_request(90);
  //counter++;
  update_info_layer();
}

static void messenger_callback(DictionaryIterator * iter){
  //if(dict_find(iter, AppKeyConfig)){
    //config_save(s_config, PersistKeyConfig);
   // s_weather.timestamp = 0;
    //schedule_weather_request(0);
    //send_weather_request_callback(NULL);
  //}
  layer_mark_dirty(s_root_layer);
}

// Hands
static void update_times(){
  int hour = s_current_time.hour;
  int nextHour;
  char buffer[] = "00:00";
  if(TEST_HOUR != -1)hour = TEST_HOUR;
  //added large points to make fine adjustments 
  GPoint hour_box_center   = time_points_large[hour % 12];
  //set next hour
  if(hour>11){
   nextHour = 1;
  }else nextHour = hour + 1;
  //set next hour center and position
  GPoint nhour_box_center = time_points[nextHour % 12];
  snprintf(buffer, 3, "%d", nextHour);
  text_block_set_text(s_nhour_text, buffer, GColorMayGreen);
  //set hour text
  snprintf(buffer, 3, "%d", hour);
  text_block_set_text(s_hour_text, buffer, GColorWhite);
  //move text blocks
  text_block_move(s_hour_text, hour_box_center);
  text_block_move(s_nhour_text, nhour_box_center);
}

static void update_date(){
  //default center point
  GPoint center_point = SOUTH_INFO_CENTER;
  char buffer[] = "00";
  //set day text
  snprintf(buffer, sizeof(buffer), "%d", s_current_time.day);
  text_block_set_text(s_south_info, buffer, GColorWhite);
  //TRY TO DODGE ANY HANDS THAT MAY BE IN THE WAY
  const ShiftLocation move = dodge_hands(date);
  if(move == left){
    center_point.y -=30;
    center_point.x -=35;
  }
  else if(move == right){
    center_point.y -=30;
    center_point.x +=35;
  }
  text_block_move(s_south_info, center_point);
}

static void mark_dirty_minute_hand_layer(){
  layer_mark_dirty(s_minute_hand_layer);
  //const float hand_angle = angle(s_current_time.minute, 60);
  //const bool rainbow_mode = config_get_bool(s_config, ConfigKeyRainbowMode);
  //rot_bitmap_layer_set_angle(s_rainbow_hand_layer, hand_angle);
  //layer_set_hidden((Layer*)s_rainbow_hand_layer, true);
}

static void update_minute_hand_layer(Layer *layer, GContext * ctx){
  const float hand_angle = angle(s_current_time.minute, 60);
  const GPoint hand_end = gpoint_on_circle(s_center, hand_angle, MINUTE_HAND_RADIUS);
  graphics_context_set_stroke_width(ctx, MINUTE_HAND_STROKE);
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, s_center, hand_end);
}

static void update_hour_hand_layer(Layer * layer, GContext * ctx){
  const float hand_angle = angle(s_current_time.hour * 50 + s_current_time.minute * 50 / 60, 600);
  const GPoint hand_end = gpoint_on_circle(s_center, hand_angle, HOUR_HAND_RADIUS);
  graphics_context_set_stroke_width(ctx, HOUR_HAND_STROKE);
  graphics_context_set_stroke_color(ctx, GColorBlueMoon);
  graphics_draw_line(ctx, s_center, hand_end);
}

static void update_center_circle_layer(Layer * layer, GContext * ctx){
  graphics_context_set_fill_color(ctx, GColorBabyBlueEyes);
  graphics_fill_circle(ctx, s_center, HOUR_CIRCLE_RADIUS);
}

// Ticks
static void draw_tick(GContext *ctx, const int index){
  int i = 0;
  graphics_context_set_stroke_width(ctx, TICK_STROKE);
  graphics_context_set_stroke_color(ctx, GColorMayGreen);
  for(i=0;i<12;i++){
    graphics_draw_line(ctx, ticks_points[i][0],ticks_points[i][1]);
    
  }
  graphics_context_set_stroke_color(ctx, GColorWhite);
  graphics_draw_line(ctx, ticks_points[index][0], ticks_points[index][1]);
}

static void tick_layer_update_callback(Layer *layer, GContext *ctx) {
  //graphics_context_set_stroke_color(ctx, GColorMayGreen);
  //graphics_context_set_stroke_width(ctx, TICK_STROKE);
  const int hour_tick_index = s_current_time.hour % 12;
  draw_tick(ctx, hour_tick_index);
}

// Infos: bluetooth + weather

static void update_info_layer(){
  char info_buffer[11] = {0};
  GPoint center_point = NORTH_INFO_CENTER;
  if(!s_bt_connected){
    strncat(info_buffer, "z", 2);
  }
  //updated expiration from 25 mins to 60 mins. 6/3/2016
  //s_weather.timestamp = time(NULL)+ 1;
  const bool weather_valid = time(NULL) < s_weather.timestamp;
  //only show weather if last successful update was within expiration
  int temp = s_weather.temperature;
  if (!s_celsius){
    temp = temp * 9 / 5 + 32;
  }
  char temp_buffer[7];
  snprintf(temp_buffer, 7, "%c%d°", s_weather.icon, temp);
  strcat(info_buffer, temp_buffer);

  //const GColor info_color = config_get_color(s_config, ConfigKeyInfoColor);
  //text_block_set_text(s_north_info, info_buffer, info_color);
  if(weather_valid)text_block_set_text(s_north_info, info_buffer, GColorWhite);
  else text_block_set_text(s_north_info, info_buffer, GColorMayGreen);
  //TRY TO DODGE ANY HANDS THAT MAY BE IN THE WAY
  const ShiftLocation move = dodge_hands(theWeather);
  if(move == left){
    center_point.y +=25;
    center_point.x -=35;
  }
  else if(move == right){
    center_point.y +=25;
    center_point.x +=35;
  }
  text_block_move(s_north_info, center_point);
}

static void bt_handler(bool connected){
  s_bt_connected = connected;
  update_info_layer();
  if(!connected)vibes_long_pulse();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
  //schedule_weather_request(10000);
  update_current_time();
  layer_mark_dirty(s_hour_hand_layer);
  mark_dirty_minute_hand_layer();
  update_times();
  if(DAY_UNIT & units_changed || 1){
    update_date();
  }
  layer_mark_dirty(s_tick_layer);
  //if time to update, send weather requestvoid *context
  const int32_t lastChecked = time(NULL) - (s_weather.timestamp - WEATHER_EXPIRED_SECS);
  if ((!(s_current_time.minute % WEATHER_UPDATE_MIN) && (lastChecked > 60) && (WEATHER_UPDATE_MIN > 0))
      || s_weather.timestamp == 0)send_weather_request_callback(NULL);
  update_info_layer();
}

static void main_window_load(Window *window) {
  s_root_layer = window_get_root_layer(window);
  s_root_layer_bounds = layer_get_bounds(s_root_layer);
  s_center = grect_center_point(&s_root_layer_bounds);
  update_current_time();
  window_set_background_color(window, GColorBlack);

  s_south_info = text_block_create(s_root_layer, SOUTH_INFO_CENTER, s_font);
  s_north_info = text_block_create(s_root_layer, NORTH_INFO_CENTER, s_font);
  
  bluetooth_connection_service_subscribe(bt_handler);
  bt_handler(connection_service_peek_pebble_app_connection());
  
  s_hour_text = text_block_create(s_root_layer, time_points[6] , s_cHour_font);
  s_nhour_text = text_block_create(s_root_layer, time_points[6], s_font);

  s_tick_layer = layer_create(s_root_layer_bounds);
  layer_set_update_proc(s_tick_layer, tick_layer_update_callback);
  layer_add_child(s_root_layer, s_tick_layer);

  s_minute_hand_layer   = layer_create(s_root_layer_bounds);
  s_hour_hand_layer     = layer_create(s_root_layer_bounds);
  s_center_circle_layer = layer_create(s_root_layer_bounds);
  layer_set_update_proc(s_hour_hand_layer,     update_hour_hand_layer);
  layer_set_update_proc(s_minute_hand_layer,   update_minute_hand_layer);
  layer_set_update_proc(s_center_circle_layer, update_center_circle_layer);
  layer_add_child(s_root_layer, s_minute_hand_layer);
  layer_add_child(s_root_layer, s_hour_hand_layer);
  layer_add_child(s_root_layer, s_center_circle_layer);
  mark_dirty_minute_hand_layer();

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  update_current_time();
  update_times();
  update_date();
  Message messages[]= {
    { MESSAGE_KEY_TempUnits, config_callback },
    { MESSAGE_KEY_AppKeyJsReady, js_ready_callback },
    { MESSAGE_KEY_AppKeyWeatherTemperature, weather_requested_callback }
  };
  s_messenger = messenger_create(3, messenger_callback, messages);
}

static void main_window_unload(Window *window) {
  layer_destroy(s_hour_hand_layer);
  layer_destroy(s_minute_hand_layer);
  layer_destroy(s_center_circle_layer);

  text_block_destroy(s_hour_text);
  text_block_destroy(s_nhour_text);

  layer_destroy(s_tick_layer);

  bluetooth_connection_service_unsubscribe();
  text_block_destroy(s_south_info);
  text_block_destroy(s_north_info);
}

static void init() {
  s_weather_request_timeout = 0;
  s_js_ready = false;
  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_NUPE_23));
  s_cHour_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_OSTRICH_BLACK_42));
  if(persist_exists(PersistKeyWeather)){
    persist_read_data(PersistKeyWeather, &s_weather, sizeof(Weather));
  }
  else{
    s_weather.temperature = -99;
    s_weather.icon = 0;
    s_weather.timestamp = 0;
  }
  if(persist_exists(PersistKeyCelsius)){
    s_celsius = persist_read_int(PersistKeyCelsius);
  }
  if(persist_exists(PersistKeyUpateRate)){
    WEATHER_UPDATE_MIN = persist_read_int(PersistKeyUpateRate);
  }
  if(persist_exists(PersistKeyExpiredSecs)){
    WEATHER_EXPIRED_SECS = persist_read_int(PersistKeyExpiredSecs);
  }
  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
      .load = main_window_load,
      .unload = main_window_unload
  });
  window_stack_push(s_main_window, true);
}

static void deinit() {
  tick_timer_service_unsubscribe();
  app_message_deregister_callbacks();
  window_destroy(s_main_window);
  //config_destroy(s_config);
  fonts_unload_custom_font(s_font);
  fonts_unload_custom_font(s_cHour_font);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
