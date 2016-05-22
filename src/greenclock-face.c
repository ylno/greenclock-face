#include <pebble.h>

#define COLORS       true
#define ANTIALIASING true

#define HAND_MARGIN  10
#define HAND_MARGIN_HOUR  25

#if defined(PBL_PLATFORM_BASALT)
#define FINAL_RADIUS 70
// XY startpoints where battery is drawn
#define BAT_START_X  112
#define BAT_START_Y  2
#define DATE_X 2
#define DATE_Y -4
#define TIME_X 104
#define TIME_Y 143
#define SHOW_DATE true
#define SHOW_TIME true
#define SHOW_BATTERY true

#elif defined(PBL_PLATFORM_CHALK)
#define FINAL_RADIUS 88
// XY startpoints where battery is drawn
#define BAT_START_X  112
#define BAT_START_Y  50
#define DATE_X 30
#define DATE_Y 40
#define TIME_X 30
#define TIME_Y 55
#define SHOW_DATE false
#define SHOW_TIME false
#define SHOW_BATTERY false

#endif

#define TICK_LENGTH  10

#define ANIMATION_DURATION 1000
#define ANIMATION_DELAY    600


//#define DEBUG true

typedef struct {
  int hours;
  int minutes;
  tm tick_time;
} Time;

static Window *s_main_window;
static Layer *s_canvas_layer;

static GPoint s_center;
static Time s_last_time, s_anim_time;
static int s_radius = 0, s_color_channels[3];
static bool s_animating = true;

static BatteryChargeState batteryState;
static Layer *mydrawings_layer;

typedef enum {
  showDate = 0,
} AppKey;


/*************************** AnimationImplementation **************************/

static void animation_started(Animation *anim, void *context) {
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "animation_started");
  #endif
  s_animating = true;
}

static void animation_stopped(Animation *anim, bool stopped, void *context) {
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "animation_stopped");
  #endif
  s_animating = false;

  // repaint after animation to show the other stuff
  if(s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static void animate(int duration, int delay, AnimationImplementation *implementation, bool handlers) {
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "animate");
  #endif
  Animation *anim = animation_create();
  animation_set_duration(anim, duration);
  animation_set_delay(anim, delay);
  animation_set_curve(anim, AnimationCurveEaseInOut);
  animation_set_implementation(anim, implementation);
  if(handlers) {
    animation_set_handlers(anim, (AnimationHandlers) {
      .started = animation_started,
      .stopped = animation_stopped
    }, NULL);
  }
  animation_schedule(anim);
}

/************************************ UI **************************************/

static void tick_handler(struct tm *tick_time_new, TimeUnits changed) {
  // Store time
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tick_handler");
  #endif

  s_last_time.tick_time = *tick_time_new;
  s_last_time.hours = tick_time_new->tm_hour;
  s_last_time.hours -= (s_last_time.hours > 12) ? 12 : 0;
  s_last_time.minutes = tick_time_new->tm_min;

  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "tick_hanlder hours %i minutes %i", s_last_time.hours, s_last_time.minutes);
  #endif


//  for(int i = 0; i < 3; i++) {
//    s_color_channels[i] = rand() % 256;
//  }

  // Redraw
  if(s_canvas_layer) {
    layer_mark_dirty(s_canvas_layer);
  }
}

static int hours_to_minutes(int hours_out_of_12) {
  return (int)(float)(((float)hours_out_of_12 / 12.0F) * 60.0F);
}

static void update_proc(Layer *layer, GContext *ctx) {
  // Color background?
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "start drawing");
  #endif
  if(COLORS) {
    //graphics_context_set_fill_color(ctx, GColorFromRGB(s_color_channels[0], s_color_channels[1], s_color_channels[2]));
    graphics_context_set_fill_color(ctx, GColorWhite);
    graphics_fill_rect(ctx, GRect(0, 0, 144, 168), 0, GCornerNone);
  }

  //time_t temp = time(NULL); 
  //time_t temp = 1445347103; 
  
  //struct tm *tick_time = localtime(&temp);

  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_context_set_stroke_width(ctx, 2);

  graphics_context_set_antialiased(ctx, ANTIALIASING);

  // Draw a shadow
  graphics_context_set_fill_color(ctx, GColorLightGray);
  GPoint shadow_center = GPoint(s_center.x + 4, s_center.y + 4);
  graphics_fill_circle(ctx, shadow_center, s_radius);

  // White clockface
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, s_center, s_radius);

  // Draw outline
  graphics_context_set_stroke_color(ctx, GColorBlack);
  graphics_draw_circle(ctx, s_center, s_radius);

  // Don't use current time while animating
  Time mode_time = (s_animating) ? s_anim_time : s_last_time;
  struct tm *tick_time = &mode_time.tick_time;

  if(!s_animating) {
    // Draw day ring
     graphics_context_set_stroke_width(ctx, 1);
    
    int minute_ring_radius = (int32_t)((double)(s_radius-4)  / 60 * (tick_time->tm_min + 1));
    int hour_ring_radius = (int32_t)((double)(s_radius-4)  / 24 * (tick_time->tm_hour + 1));

    if(minute_ring_radius > hour_ring_radius) {
      // minute ring then day ring
      graphics_context_set_fill_color(ctx, GColorMintGreen);
      graphics_fill_circle(ctx, s_center, minute_ring_radius);  
      graphics_context_set_fill_color(ctx, GColorMalachite);
      graphics_fill_circle(ctx, s_center, hour_ring_radius);
    } else {
      // hour ring, then day ring
      graphics_context_set_fill_color(ctx, GColorMalachite);
      graphics_fill_circle(ctx, s_center, hour_ring_radius);
      graphics_context_set_fill_color(ctx, GColorMintGreen);
      graphics_fill_circle(ctx, s_center, minute_ring_radius);  
    }



    // Draw ticks
    graphics_context_set_stroke_color(ctx, GColorChromeYellow);
    for(int i=0; i<12; i++) {
      
      float tick_angle = TRIG_MAX_ANGLE * i / 12;
      GPoint tick_start = (GPoint) {
        .x = (int16_t)(sin_lookup(tick_angle) * (int32_t)(s_radius - (TICK_LENGTH)) / TRIG_MAX_RATIO) + s_center.x,
        .y = (int16_t)(-cos_lookup(tick_angle) * (int32_t)(s_radius - (TICK_LENGTH)) / TRIG_MAX_RATIO) + s_center.y,
      };
      GPoint tick_stop = (GPoint) {
        .x = (int16_t)(sin_lookup(tick_angle) * (int32_t)(s_radius - (4)) / TRIG_MAX_RATIO) + s_center.x,
        .y = (int16_t)(-cos_lookup(tick_angle) * (int32_t)(s_radius - (4)) / TRIG_MAX_RATIO) + s_center.y,
      };
      
      if(i==0 || i==3 || i==6 || i==9) {
        graphics_context_set_stroke_width(ctx, 4);
      } else {
        graphics_context_set_stroke_width(ctx, 1);
      }

      
      graphics_draw_line(ctx, tick_start, tick_stop);
    }
  }
  graphics_context_set_stroke_color(ctx, GColorIslamicGreen);
  graphics_context_set_stroke_width(ctx, 1);
  
  // Adjust for minutes through the hour
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "TRIG_MAX_ANGLE %i animating %s hours %i", TRIG_MAX_ANGLE, s_animating ? "true" : "false", mode_time.hours);
  #endif
  float minute_angle = TRIG_MAX_ANGLE * mode_time.minutes / 60;
  float hour_angle = 0.0;
  if(s_animating) {
    // Hours out of 60 for smoothness
    hour_angle = TRIG_MAX_ANGLE * mode_time.hours / 60;
  } else {
    hour_angle = TRIG_MAX_ANGLE * mode_time.hours / 12;
  }
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "minute angle %i", (int)minute_angle);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "hour angle %i", (int)hour_angle);
  #endif
  hour_angle += (minute_angle / TRIG_MAX_ANGLE) * (TRIG_MAX_ANGLE / 12);

  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "hour angle %i", (int)hour_angle);
APP_LOG(APP_LOG_LEVEL_DEBUG, "hour angle1 %i", (int)2.1113);
  #endif
  

  // Plot hands
  GPoint minute_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(TRIG_MAX_ANGLE * mode_time.minutes / 60) * (int32_t)(s_radius - HAND_MARGIN) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(TRIG_MAX_ANGLE * mode_time.minutes / 60) * (int32_t)(s_radius - HAND_MARGIN) / TRIG_MAX_RATIO) + s_center.y,
  };
  GPoint hour_hand = (GPoint) {
    .x = (int16_t)(sin_lookup(hour_angle) * (int32_t)(s_radius - (HAND_MARGIN_HOUR)) / TRIG_MAX_RATIO) + s_center.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)(s_radius - (HAND_MARGIN_HOUR)) / TRIG_MAX_RATIO) + s_center.y,
  };

  
  // Draw hands with positive length only
  if(s_radius > 2 * HAND_MARGIN) {
    graphics_context_set_stroke_width(ctx, 4);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_draw_line(ctx, s_center, hour_hand);
    
    // Fill Hand with inner white
    //graphics_context_set_stroke_width(ctx, 1);
    //graphics_context_set_stroke_color(ctx, GColorWhite);
    //graphics_draw_line(ctx, s_center, hour_hand);
  } 

  if(s_radius > HAND_MARGIN) {
    graphics_context_set_stroke_width(ctx, 3);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_draw_line(ctx, s_center, minute_hand);

    // Fill Hand with inner white
    //graphics_context_set_stroke_width(ctx, 1);
    //graphics_context_set_stroke_color(ctx, GColorWhite);
    //graphics_draw_line(ctx, s_center, minute_hand);
  }

  graphics_context_set_stroke_width(ctx, 1);
  
  if(!s_animating) {
    static char buffer[] = "00:00";
    
    //graphics_context_set_text_color(ctx, GColorIslamicGreen);
    graphics_context_set_text_color(ctx, GColorBlack);
    strftime(buffer, sizeof("00:00"), "%H:%M", tick_time);
    // FONT_KEY_FONT_FALLBACK FONT_KEY_BITHAM_30_BLACK     GColorSpringBud

    graphics_draw_text(ctx, buffer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(TIME_X, TIME_Y,  89, 25), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);

    strftime(buffer, sizeof("01.01.20"), "%d.%m.%y", tick_time);
    graphics_draw_text(ctx, buffer, fonts_get_system_font(FONT_KEY_GOTHIC_18_BOLD), GRect(DATE_X, DATE_Y,  80, 24), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL);
  }

  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "hours %i minutes %i", mode_time.hours, mode_time.minutes);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "finished");
  #endif
  
  
}

static void my_layer_update_proc(Layer *my_layer, GContext* ctx) {

    #ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_DEBUG, "battery update");
    #endif

    if(!SHOW_BATTERY) {
      return;
    }

    //---draw 2 rectangles to represent the battery---

    //graphics_context_set_stroke_color(ctx, GColorMalachite);
    graphics_context_set_stroke_color(ctx, GColorBlack);
    graphics_context_set_fill_color(ctx, GColorMintGreen);

    GRect rect1;
    rect1.origin = GPoint(BAT_START_X, BAT_START_Y);
    rect1.size = GSize(27,10);
    graphics_draw_rect(ctx, rect1);
    
    GRect rect2;
    rect2.origin = GPoint(BAT_START_X+27, BAT_START_Y+3);
    rect2.size = GSize(2,5);
    graphics_draw_rect(ctx, rect2);
    
    GRect rect3;
    rect3.origin = GPoint(BAT_START_X+1,BAT_START_Y+1);
      
    //---change the width of the rect to match the battery level---
    rect3.size = GSize(batteryState.charge_percent/4, 8);
    //---end of statements to add---
      
    graphics_fill_rect(ctx, rect3, 0, GCornerNone);

    #ifdef DEBUG
    APP_LOG(APP_LOG_LEVEL_DEBUG, "battery %i", batteryState.charge_percent);
    #endif

}


static void window_load(Window *window) {
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "window load");
  #endif
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);

  s_center = grect_center_point(&window_bounds);

  s_canvas_layer = layer_create(window_bounds);
  layer_set_update_proc(s_canvas_layer, update_proc);
  layer_add_child(window_layer, s_canvas_layer);

  batteryState = battery_state_service_peek();
  mydrawings_layer = layer_create(window_bounds);
    layer_set_update_proc(mydrawings_layer,
    my_layer_update_proc);
  layer_add_child(window_layer, mydrawings_layer);
}

static void window_unload(Window *window) {
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "window unload");
  #endif
  layer_destroy(s_canvas_layer);
  layer_destroy(mydrawings_layer);
  battery_state_service_unsubscribe();
}

/*********************************** App **************************************/

static int anim_percentage(uint32_t dist_normalized, int max) {
  return (int)(float)(((float)dist_normalized / (float)ANIMATION_NORMALIZED_MAX) * (float)max);
}

static void radius_update(Animation *anim, AnimationProgress dist_normalized) {
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "radius update");
  #endif
  s_radius = anim_percentage(dist_normalized, FINAL_RADIUS);

  layer_mark_dirty(s_canvas_layer);
}

static void hands_update(Animation *anim, AnimationProgress dist_normalized) {
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "hands_update");
  #endif
  s_anim_time.hours = anim_percentage(dist_normalized, hours_to_minutes(s_last_time.hours));
  s_anim_time.minutes = anim_percentage(dist_normalized, s_last_time.minutes);

  layer_mark_dirty(s_canvas_layer);
}

static void battery_state_handler(BatteryChargeState charge) {
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "battery_state_handler");
  #endif
  //---mark the drawing layer as dirty so as to force
  // a redraw---
  
  if(batteryState.charge_percent != charge.charge_percent) {
    batteryState = charge;
    layer_mark_dirty(mydrawings_layer);
  }
  
}

static void init() {
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "init");
  #endif
  srand(time(NULL));

  time_t t = time(NULL);
  struct tm *time_now = localtime(&t);

  tick_handler(time_now, MINUTE_UNIT);

  s_main_window = window_create();
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  window_stack_push(s_main_window, true);

  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

  // Prepare animations
  AnimationImplementation radius_impl = {
    .update = radius_update
  };
  animate(ANIMATION_DURATION, ANIMATION_DELAY, &radius_impl, false);

  AnimationImplementation hands_impl = {
    .update = hands_update
  };
  animate(2 * ANIMATION_DURATION, ANIMATION_DELAY, &hands_impl, true);

  battery_state_service_subscribe(battery_state_handler);

}

static void deinit() {
  tick_timer_service_unsubscribe();
   
  //---add in the following statements---
          //---unsubscribe from the battery charge state event
  // serice---
  battery_state_service_unsubscribe();
  window_destroy(s_main_window);
  
}

int main() {
  init();
  #ifdef DEBUG
  APP_LOG(APP_LOG_LEVEL_DEBUG, "main");
  #endif
  app_event_loop();
  deinit();
}
