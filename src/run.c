/*
 * pebble pedometer
 * @author jathusant
 */

#include <pebble.h>
#include <run.h>
#include <math.h>

// Total Steps (TS)
#define TS 1
// Total Steps Default (TSD)
#define TSD 1
// weather test
#define KEY_P1 0
#define KEY_S1 1
#define KEY_P2 2
#define KEY_S2 3
#define KEY_DATA 5

static Window *window;
static Window *menu_window;
static Window *pedometer;
static Window *dev_info;
static Window *leadership_info;

static SimpleMenuLayer *pedometer_settings;
static SimpleMenuItem menu_items[7];
static SimpleMenuSection menu_sections[1];

// Menu Item names and subtitles
char *item_names[7] = { "Start", "Leaderboard", "Overall Steps", "Overall Calories", "Sensitivity", "Theme", "About" };

char *item_sub[7] = { "Lets Exercise!", "Where do you stand?", "0 in Total", "0 Burned", "", "", "Hack the Planet" };

// Timer used to determine next step check
static AppTimer *timer;

// Text Layers
TextLayer *main_message;
TextLayer *main_message2;
TextLayer *hitBack;
TextLayer *steps;
TextLayer *pedCount;
TextLayer *infor;
TextLayer *calories;

static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;
static TextLayer *leadership_layer;

// Bitmap Layers
static GBitmap *btn_dwn;
static GBitmap *btn_up;
static GBitmap *btn_sel;
static GBitmap *statusBar;
GBitmap *pedometerBack;
BitmapLayer *pedometerBack_layer;
GBitmap *splash;
BitmapLayer *splash_layer;
GBitmap *flame;
BitmapLayer *flame_layer;

// interval to check for next step (in ms)
const int ACCEL_STEP_MS = 475;
// value to auto adjust step acceptance 
const int PED_ADJUST = 2;
// steps required per calorie
const int STEPS_PER_CALORIE = 22;
// values for max/min number of calibration options 
const int MAX_CALIBRATION_SETTINGS = 3;
const int MIN_CALIBRATION_SETTINGS = 1;

int X_DELTA = 35;
int Y_DELTA, Z_DELTA = 185;
int YZ_DELTA_MIN = 175;
int YZ_DELTA_MAX = 195; 
int X_DELTA_TEMP, Y_DELTA_TEMP, Z_DELTA_TEMP = 0;
int lastX, lastY, lastZ, currX, currY, currZ = 0;
int sensitivity = 1;

char *p1 = "dank";
char *p2 = "dank2";
int s1 = 0;
int s2 = 0;

long pedometerCount = 0;
long caloriesBurned = 0;
long tempTotal = 0;

bool did_pebble_vibrate = false;
bool validX, validY, validZ = false;
bool SID;
bool isDark;
bool startedSession = false;

// Strings used to display theme and calibration options
char *theme;
char *cal = "Regular Sensitivity";

// stores total steps since app install
static long totalSteps = TSD;

void start_callback(int index, void *ctx) {
	accel_data_service_subscribe(0, NULL);

	menu_items[0].title = "Continue Run";
	menu_items[0].subtitle = "Ready for more?";
	layer_mark_dirty(simple_menu_layer_get_layer(pedometer_settings));

	pedometer = window_create();

	window_set_window_handlers(pedometer, (WindowHandlers ) { .load = ped_load,
					.unload = ped_unload, });

	window_stack_push(pedometer, true);
	timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

void info_callback(int index, void *ctx) {
	dev_info = window_create();

	window_set_window_handlers(dev_info, (WindowHandlers ) { .load = info_load,
					.unload = info_unload, });

	window_stack_push(dev_info, true);
}


void calibration_callback(int index, void *ctx) {
	
	if (sensitivity >= MIN_CALIBRATION_SETTINGS && sensitivity < MAX_CALIBRATION_SETTINGS){
		sensitivity++;
	} else if (sensitivity == MAX_CALIBRATION_SETTINGS) {
		sensitivity = MIN_CALIBRATION_SETTINGS;
	}

	cal = determineCal(sensitivity);
	
	menu_items[4].subtitle = cal;
	layer_mark_dirty(simple_menu_layer_get_layer(pedometer_settings));
}

void theme_callback(int index, void *ctx) {
	if (isDark) {
		isDark = false;
		theme = "Light";
	} else {
		isDark = true;
		theme = "Dark";
	}

	char* new_string;
	new_string = malloc(strlen(theme) + 10);
	strcpy(new_string, "Current: ");
	strcat(new_string, theme);
	menu_items[4].subtitle = new_string;

	layer_mark_dirty(simple_menu_layer_get_layer(pedometer_settings));
}

char* determineCal(int cal){
	switch(cal){
		case 2:
		X_DELTA = 45;
		Y_DELTA = 235;
		Z_DELTA = 235;
		YZ_DELTA_MIN = 225;
		YZ_DELTA_MAX = 245; 
		return "Not Sensitive";
		case 3:
		X_DELTA = 25;
		Y_DELTA = 110;
		Z_DELTA = 110;
		YZ_DELTA_MIN = 100;
		YZ_DELTA_MAX = 120; 
		return "Very Sensitive";
		default:
		X_DELTA = 35;
		Y_DELTA = 185;
		Z_DELTA = 185;
		YZ_DELTA_MIN = 175;
		YZ_DELTA_MAX = 195; 
		return "Regular Sensitivity";
	}
}

void set_click_handler(ClickRecognizerRef recognizer, void *context) {
	window_stack_pop(true);
}

void leadership_callback(int index, void *ctx) {
  leadership_info = window_create();

	window_set_window_handlers(leadership_info, (WindowHandlers ) { .load = leadership_load,
					.unload = leadership_unload, });

	window_stack_push(leadership_info, true);
}

void setup_menu_items() {
	static char buf[] = "1234567890abcdefg";
	snprintf(buf, sizeof(buf), "%ld in Total", totalSteps);

	static char buf2[] = "1234567890abcdefg";
	snprintf(buf2, sizeof(buf2), "%ld Burned",
			(long) (totalSteps / STEPS_PER_CALORIE));

	for (int i = 0; i < (int) (sizeof(item_names) / sizeof(item_names[0]));
			i++) {
		menu_items[i] = (SimpleMenuItem ) { .title = item_names[i], .subtitle =
						item_sub[i], };

		//Setting Callbacks
		if (i == 0) {
			menu_items[i].callback = start_callback;
		} else if (i == 1) {
      menu_items[i].callback = leadership_callback;
    } else if (i==2) {
			menu_items[i].subtitle = buf;
		} else if (i == 3) {
			menu_items[i].subtitle = buf2;
		} else if (i == 4){
			menu_items[i].subtitle = determineCal(sensitivity);
			menu_items[i].callback = calibration_callback;
		} else if (i == 5) {
			menu_items[i].subtitle = theme;
			menu_items[i].callback = theme_callback;
		} else if (i == 6 || i == 6) {
			menu_items[i].callback = info_callback;
		}
	}
}

void setup_menu_sections() {
	menu_sections[0] = (SimpleMenuSection ) { .items = menu_items, .num_items =
					sizeof(menu_items) / sizeof(menu_items[0]) };
}

void setup_menu_window() {
	menu_window = window_create();

	window_set_window_handlers(menu_window, (WindowHandlers ) { .load =
					settings_load, .unload = settings_unload, });
}


void settings_load(Window *window) {
	Layer *layer = window_get_root_layer(menu_window);
	statusBar = gbitmap_create_with_resource(RESOURCE_ID_STATUS_BAR);

	pedometer_settings = simple_menu_layer_create(layer_get_bounds(layer),
			menu_window, menu_sections, 1, NULL);
	simple_menu_layer_set_selected_index(pedometer_settings, 0, true);
	layer_add_child(layer, simple_menu_layer_get_layer(pedometer_settings));
// 	window_set_status_bar_icon(menu_window, statusBar);
}

void settings_unload(Window *window) {
	layer_destroy(window_get_root_layer(menu_window));
	simple_menu_layer_destroy(pedometer_settings);
// 	window_destroy(menu_window);
}

void ped_load(Window *window) {
	steps = text_layer_create(GRect(0, 120, 150, 170));
	pedCount = text_layer_create(GRect(0, 75, 150, 170));
	calories = text_layer_create(GRect(0, 50, 150, 170));

	if (isDark) {
		window_set_background_color(pedometer, GColorBlack);
		pedometerBack = gbitmap_create_with_resource(RESOURCE_ID_PED_WHITE);
		flame = gbitmap_create_with_resource(RESOURCE_ID_FLAME_WHITE);
		text_layer_set_background_color(steps, GColorClear);
		text_layer_set_text_color(steps, GColorBlack);
		text_layer_set_background_color(pedCount, GColorClear);
		text_layer_set_text_color(pedCount, GColorBlack);
		text_layer_set_background_color(calories, GColorClear);
		text_layer_set_text_color(calories, GColorWhite);
	} else {
		window_set_background_color(pedometer, GColorWhite);
		pedometerBack = gbitmap_create_with_resource(RESOURCE_ID_PED_BLK);
		flame = gbitmap_create_with_resource(RESOURCE_ID_FLAME_BLK);
		text_layer_set_background_color(steps, GColorClear);
		text_layer_set_text_color(steps, GColorWhite);
		text_layer_set_background_color(pedCount, GColorClear);
		text_layer_set_text_color(pedCount, GColorWhite);
		text_layer_set_background_color(calories, GColorClear);
		text_layer_set_text_color(calories, GColorBlack);
	}

	pedometerBack_layer = bitmap_layer_create(GRect(0, 0, 145, 215));
	flame_layer = bitmap_layer_create(GRect(50, 0, 50, 50));

	bitmap_layer_set_bitmap(pedometerBack_layer, pedometerBack);
	bitmap_layer_set_bitmap(flame_layer, flame);
	
	layer_add_child(window_get_root_layer(pedometer),
			bitmap_layer_get_layer(pedometerBack_layer));
	
	layer_add_child(window_get_root_layer(pedometer),
			bitmap_layer_get_layer(flame_layer));

	layer_add_child(window_get_root_layer(pedometer), (Layer*) steps);
	layer_add_child(window_get_root_layer(pedometer), (Layer*) pedCount);
	layer_add_child(window_get_root_layer(pedometer), (Layer*) calories);

	text_layer_set_font(pedCount, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_40)));
	text_layer_set_font(calories, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_15)));
	text_layer_set_font(steps, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_BEBAS_15)));
	
	text_layer_set_text_alignment(steps, GTextAlignmentCenter);
	text_layer_set_text_alignment(pedCount, GTextAlignmentCenter);
	text_layer_set_text_alignment(calories, GTextAlignmentCenter);

	text_layer_set_text(steps, "s t e p s");

	static char buf[] = "1234567890";
	snprintf(buf, sizeof(buf), "%ld", pedometerCount);
	text_layer_set_text(pedCount, buf);

	static char buf2[] = "1234567890abcdefghijkl";
	snprintf(buf2, sizeof(buf2), "%ld Calories", caloriesBurned);
	text_layer_set_text(calories, buf2);
}

void ped_unload(Window *window) {
	app_timer_cancel(timer);
	window_destroy(pedometer);
	text_layer_destroy(pedCount);
	text_layer_destroy(calories);
	text_layer_destroy(steps);
	gbitmap_destroy(pedometerBack);
	accel_data_service_unsubscribe();
}


void info_load(Window *window) {
	s_weather_layer = text_layer_create(GRect(0, 0, 150, 150));

	if (isDark) {
		window_set_background_color(dev_info, GColorBlack);
		text_layer_set_background_color(s_weather_layer, GColorClear);
		text_layer_set_text_color(s_weather_layer, GColorWhite);
	} else {
		window_set_background_color(dev_info, GColorWhite);
		text_layer_set_background_color(s_weather_layer, GColorClear);
		text_layer_set_text_color(s_weather_layer, GColorBlack);
	}

	layer_add_child(window_get_root_layer(dev_info), (Layer*) s_weather_layer);
	text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
  
  char *stepPerson = malloc(strlen("Name: Cody Casey\nSteps: 1224") + sizeof(char));
  
	strcpy(stepPerson, "Made by:\n Bryan Tan\n Sandile Keswa\n Christina Zhu ");
  
	text_layer_set_text(s_weather_layer, stepPerson);
}

void info_unload(Window *window) {
	text_layer_destroy(s_weather_layer);
	window_destroy(dev_info);
}

void leadership_load(Window *window) {
	leadership_layer = text_layer_create(GRect(0, 0, 150, 150));

	if (isDark) {
		window_set_background_color(leadership_info, GColorBlack);
		text_layer_set_background_color(leadership_layer, GColorClear);
		text_layer_set_text_color(leadership_layer, GColorWhite);
	} else {
		window_set_background_color(leadership_info, GColorWhite);
		text_layer_set_background_color(leadership_layer, GColorClear);
		text_layer_set_text_color(leadership_layer, GColorBlack);
	}

	layer_add_child(window_get_root_layer(leadership_info), (Layer*) leadership_layer);
	text_layer_set_text_alignment(leadership_layer, GTextAlignmentCenter);
  
  char *stepPerson = malloc(strlen("Name: Cody Casey\nSteps: 1224Name: Cody Casey\nSteps: 1224Name: Cody Casey\nSteps: 1224") + sizeof(char));
  
  strcpy(stepPerson, p1);
  strcat(stepPerson, "\nSteps: ");

  char stepTemp[strlen("12234") + sizeof(char)];
  snprintf(stepTemp, sizeof(stepTemp), "%d", s1);
  strcat(stepPerson, stepTemp);

  APP_LOG(APP_LOG_LEVEL_DEBUG, "steptemp is %s", stepTemp);

  strcat(stepPerson, "\n\n");
  strcat(stepPerson, p2);
  strcat(stepPerson, "\nSteps: ");
  char stepTemp2[strlen("12234") + sizeof(char)];
  snprintf(stepTemp2, sizeof(stepTemp2), "%d", s2);
  
	strcat(stepPerson, stepTemp2);
  
	text_layer_set_text(leadership_layer, stepPerson);
}

void leadership_unload(Window *window) {
	text_layer_destroy(leadership_layer);
	window_destroy(leadership_info);
}

void window_load(Window *window) {

	splash = gbitmap_create_with_resource(RESOURCE_ID_SPLASH);
	window_set_background_color(window, GColorBlack);

	splash_layer = bitmap_layer_create(GRect(0, 0, 145, 185));
	bitmap_layer_set_bitmap(splash_layer, splash);
	layer_add_child(window_get_root_layer(window),
			bitmap_layer_get_layer(splash_layer));

	main_message = text_layer_create(GRect(0, 0, 150, 170));
	main_message2 = text_layer_create(GRect(3, 30, 150, 170));
	hitBack = text_layer_create(GRect(3, 40, 200, 170));

	text_layer_set_background_color(main_message, GColorClear);
	text_layer_set_text_color(main_message, GColorWhite);
	text_layer_set_font(main_message,
			fonts_load_custom_font(
					resource_get_handle(RESOURCE_ID_ROBOTO_LT_30)));
	layer_add_child(window_get_root_layer(window), (Layer*) main_message);

	text_layer_set_background_color(main_message2, GColorClear);
	text_layer_set_text_color(main_message2, GColorWhite);
	text_layer_set_font(main_message2,
			fonts_load_custom_font(
					resource_get_handle(RESOURCE_ID_ROBOTO_LT_15)));
	layer_add_child(window_get_root_layer(window), (Layer*) main_message2);

	text_layer_set_background_color(hitBack, GColorClear);
	text_layer_set_text_color(hitBack, GColorWhite);
	text_layer_set_font(hitBack,
			fonts_load_custom_font(
					resource_get_handle(RESOURCE_ID_ROBOTO_LT_15)));
	layer_add_child(window_get_root_layer(window), (Layer*) hitBack);

	text_layer_set_text(hitBack, "\n\n\n\n\n\n     << Press Back");
}

void window_unload(Window *window) {
	window_destroy(window);
	text_layer_destroy(main_message);
	text_layer_destroy(main_message2);
	text_layer_destroy(hitBack);
	bitmap_layer_destroy(splash_layer);
}

void autoCorrectZ(){
	if (Z_DELTA > YZ_DELTA_MAX){
		Z_DELTA = YZ_DELTA_MAX; 
	} else if (Z_DELTA < YZ_DELTA_MIN){
		Z_DELTA = YZ_DELTA_MIN;
	}
}

void autoCorrectY(){
	if (Y_DELTA > YZ_DELTA_MAX){
		Y_DELTA = YZ_DELTA_MAX; 
	} else if (Y_DELTA < YZ_DELTA_MIN){
		Y_DELTA = YZ_DELTA_MIN;
	}
}

void pedometer_update() {
	if (startedSession) {
		X_DELTA_TEMP = abs(abs(currX) - abs(lastX));
		if (X_DELTA_TEMP >= X_DELTA) {
			validX = true;
		}
		Y_DELTA_TEMP = abs(abs(currY) - abs(lastY));
		if (Y_DELTA_TEMP >= Y_DELTA) {
			validY = true;
			if (Y_DELTA_TEMP - Y_DELTA > 200){
				autoCorrectY();
				Y_DELTA = (Y_DELTA < YZ_DELTA_MAX) ? Y_DELTA + PED_ADJUST : Y_DELTA;
			} else if (Y_DELTA - Y_DELTA_TEMP > 175){
				autoCorrectY();
				Y_DELTA = (Y_DELTA > YZ_DELTA_MIN) ? Y_DELTA - PED_ADJUST : Y_DELTA;
			}
		}
		Z_DELTA_TEMP = abs(abs(currZ) - abs(lastZ));
		if (abs(abs(currZ) - abs(lastZ)) >= Z_DELTA) {
			validZ = true;
			if (Z_DELTA_TEMP - Z_DELTA > 200){
				autoCorrectZ();
				Z_DELTA = (Z_DELTA < YZ_DELTA_MAX) ? Z_DELTA + PED_ADJUST : Z_DELTA;
			} else if (Z_DELTA - Z_DELTA_TEMP > 175){
				autoCorrectZ();
				Z_DELTA = (Z_DELTA < YZ_DELTA_MAX) ? Z_DELTA + PED_ADJUST : Z_DELTA;
			}
		}
	} else {
		startedSession = true;
	}
}

void resetUpdate() {
	lastX = currX;
	lastY = currY;
	lastZ = currZ;
	validX = false;
	validY = false;
	validZ = false;
}

static void send_int(int key, int value) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int(iter, key, &value, sizeof(int), true);
  app_message_outbox_send();
}

void update_ui_callback() {
	if ((validX && validY && !did_pebble_vibrate) || (validX && validZ && !did_pebble_vibrate)) {
		pedometerCount++;
		tempTotal++;
    
    send_int(5, pedometerCount);

		caloriesBurned = (int) (pedometerCount / STEPS_PER_CALORIE);
		static char calBuf[] = "123456890abcdefghijkl";
		snprintf(calBuf, sizeof(calBuf), "%ld Calories", caloriesBurned);
		text_layer_set_text(calories, calBuf);

		static char buf[] = "123456890abcdefghijkl";
		snprintf(buf, sizeof(buf), "%ld", pedometerCount);
		text_layer_set_text(pedCount, buf);

		static char buf2[] = "123456890abcdefghijkl";
		snprintf(buf2, sizeof(buf2), "%ld in Total", tempTotal);
		menu_items[2].subtitle = buf2;

		static char buf3[] = "1234567890abcdefg";
		snprintf(buf3, sizeof(buf3), "%ld Burned",
				(long) (tempTotal / STEPS_PER_CALORIE));
		menu_items[3].subtitle = buf3;

		layer_mark_dirty(window_get_root_layer(pedometer));
		layer_mark_dirty(window_get_root_layer(menu_window));
	}

	resetUpdate();
}

static void timer_callback(void *data) {
	AccelData accel = (AccelData ) { .x = 0, .y = 0, .z = 0 };
	accel_service_peek(&accel);

	if (!startedSession) {
		lastX = accel.x;
		lastY = accel.y;
		lastZ = accel.z;
	} else {
		currX = accel.x;
		currY = accel.y;
		currZ = accel.z;
	}
	
	did_pebble_vibrate = accel.did_vibrate;

	pedometer_update();
	update_ui_callback();

	layer_mark_dirty(window_get_root_layer(pedometer));
	timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  // Store incoming information
  static char temperature_buffer[32];
  static char conditions_buffer[32];
  static char weather_layer_buffer[32];
  
  // Read first item
  Tuple *t = dict_read_first(iterator);

  // For all items
  while(t != NULL) {
    // Which key was received?
    switch(t->key) {
    case KEY_S1:
//       snprintf(steps2, sizeof(temperature_buffer), "%dC", (int)t->value->int32);
      s1 = (int)t->value->int32;
      break;
    case KEY_P1:
//       snprintf(name, sizeof(conditions_buffer), "%s", t->value->cstring);
      p1 = t->value->cstring;
      break;
    case KEY_P2:
      p2 = t->value->cstring;
      break;
    case KEY_S2:
      s2 = (int)t->value->int32;
      break;
    default:
      APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
      break;
    }

    // Look for next item
    t = dict_read_next(iterator);
  }
  APP_LOG(APP_LOG_LEVEL_DEBUG, "name %s", p1);
  APP_LOG(APP_LOG_LEVEL_DEBUG, "steps %d", s1);

  // Assemble full string and display
//   snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
//   text_layer_set_text(s_weather_layer, weather_layer_buffer);
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


void handle_init(void) {
	tempTotal = totalSteps = persist_exists(TS) ? persist_read_int(TS) : TSD;
	isDark = persist_exists(SID) ? persist_read_bool(SID) : true;

	if (!isDark) {
		theme = "Current: Light";
	} else {
		theme = "Current: Dark";
	}

	window = window_create();

	setup_menu_items();
	setup_menu_sections();
	setup_menu_window();

  window_stack_push(menu_window, true);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void handle_deinit(void) {
	totalSteps += pedometerCount;
  APP_LOG(APP_LOG_LEVEL_DEBUG, "total steps is %li and pedometerCount is %li", totalSteps, pedometerCount);
	persist_write_int(TS, totalSteps);
	persist_write_bool(SID, isDark);
// 	accel_data_service_unsubscribe();
// 	window_destroy(menu_window);
}