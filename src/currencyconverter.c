/* Copyright 2014 Andreas Muttscheller

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <pebble.h>
#include <stdlib.h>

#include "helper.h"

// *** consts ***
#define CURRENCY_OFFSET 100
#define CURRENCY_STR_LEN 3
#define RATE_STR_LEN 16
// *** *** *** ***

// *** Variables for User interface ***
static Window *window;
static TextLayer *from_layer, *to_layer, *equals_layer, *no_currency_layer;

static char from[64];
static char to[64];
static char no[16];
// *** *** *** *** *** *** *** ***

static int value = 1;
static uint32_t currency_index = 0;
static int currency_length = 0;
static char currencies[PERSIST_STRING_MAX_LENGTH];
static char currency_current[CURRENCY_STR_LEN];
static double currency_current_rate;
static bool currencies_updating = false;

static char from_currency[CURRENCY_STR_LEN];
static double from_currency_rate;

// *** enums for messages ***
enum {
  UPDATE_CURRENCY = 0x0,
  CURRENCY_INDEX = 0x1,
  CURRENCY = 0x2,
  CURRENCY_RATE = 0x3,
  CURRENCY_LENGTH = 0x4,
  CURRENCY_DONE = 0x5,
  ERROR_SENDING = 0x6,
  FROM_CURRENCY = 0xE,
  FROM_CURRENCY_RATE = 0xF,
};
// *** *** *** *** *** ***

static void updateCurrentCurrency() {
  char rate[RATE_STR_LEN];

  strncpy(currency_current, &currencies[currency_index*CURRENCY_STR_LEN], CURRENCY_STR_LEN);
  
  persist_read_string(currency_index+CURRENCY_OFFSET+1, rate, sizeof(rate));
  APP_LOG(APP_LOG_LEVEL_DEBUG, "persist-key: %d : %s (%s)", (int)currency_index+CURRENCY_OFFSET+1, currency_current, rate);
  
  currency_current_rate = strtodbl(rate);
}

static void updateTextFields() {
  if (currency_length > 0) {
    snprintf(from, sizeof(from), "%d %s", value, from_currency);
    text_layer_set_text(from_layer, (const char*) from);
  
    char a[RATE_STR_LEN];
    double d = value * currency_current_rate / from_currency_rate;
    ftoa(a, d, 2);
  
    snprintf(to, sizeof(to), "%s %s", a, currency_current);
    text_layer_set_text(to_layer, to);
    
    text_layer_set_text(equals_layer, "=");

    snprintf(no, sizeof(no), "%d / %d ", (int)currency_index+1, currency_length);
    text_layer_set_text(no_currency_layer, no);
  }
}

static void out_sent_handler(DictionaryIterator *sent, void *context) {
  // outgoing message was delivered
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  // outgoing message failed
}
 
static void in_received_handler(DictionaryIterator *received, void *context) {
  // incoming message received
  Tuple *tuple =  dict_read_first(received);
  
  uint32_t index = 0;
  char rate[RATE_STR_LEN];
  char currency[CURRENCY_STR_LEN];

   
  while (tuple) {
	//APP_LOG(APP_LOG_LEVEL_DEBUG, "Tuple: %d : %s", (int)tuple->key, tuple->value->cstring);
	switch (tuple->key) {
	case CURRENCY_INDEX:
	  index = tuple->value->uint32;
	  break;
	case CURRENCY:
	  strncpy(currency, tuple->value->cstring, CURRENCY_STR_LEN);
	  break;
	case CURRENCY_RATE:
	  strncpy(rate, tuple->value->cstring, RATE_STR_LEN);
	  break;
	case CURRENCY_LENGTH:
	  persist_write_string(CURRENCY, currencies);
	  currency_length = tuple->value->int32;
	  persist_write_int(CURRENCY_LENGTH, currency_length);
	  break;
	case CURRENCY_DONE:
	  // Standard: From EUR
	  strncpy(from_currency, "EUR", CURRENCY_STR_LEN);
      from_currency_rate = 1.0;
	  text_layer_set_text(equals_layer, "=");
	  currencies_updating = false;
	  currency_index = 0;
	  updateCurrentCurrency();
	  updateTextFields();
	  break;
	case ERROR_SENDING:
	  currencies_updating = false;
	  text_layer_set_text(from_layer, "Error!");
	  text_layer_set_text(equals_layer, "Check network");
	  text_layer_set_text(to_layer, "connectivity!");
	  break;
	default:
	  break;
	}
	tuple = dict_read_next(received);
  }
  
  if (currencies_updating) {
	strncat(currencies, currency, CURRENCY_STR_LEN);
    persist_write_string(index + CURRENCY_OFFSET, rate);
    
    snprintf(no, sizeof(no), "%d / %d ", (int)index, currency_length);
    text_layer_set_text(no_currency_layer, no);
  }
}


static void in_dropped_handler(AppMessageResult reason, void *context) {
  // incoming message dropped
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Dropped! %d", reason);
  currency_length--;
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!currencies_updating) {
    currency_index = (currency_index+1) % currency_length;
    updateCurrentCurrency();
    updateTextFields();
  }
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!currencies_updating) {
    value++;
    updateTextFields();
  }
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!currencies_updating) {
    if (value > 1) value--;
    updateTextFields();
  }
}

static void select_multi_click_handler(ClickRecognizerRef recognizer, void *context) {
  const uint16_t count = click_number_of_clicks_counted(recognizer);
  
  // Two clicks, send message to update currency
  if (count == 2) {
	memset(currencies, 0, sizeof(currencies));
	DictionaryIterator *iter;
    app_message_outbox_begin(&iter);
    Tuplet value = TupletInteger(UPDATE_CURRENCY, 1);
    dict_write_tuplet(iter, &value);
    app_message_outbox_send();
    
    text_layer_set_text(from_layer, "");
    text_layer_set_text(equals_layer, "Loading...");
    text_layer_set_text(to_layer, "");
    
    currencies_updating = true;
  }
}

void select_long_click_handler(ClickRecognizerRef recognizer, void *context) {
  if (!currencies_updating) {
    vibes_double_pulse();
    strncpy(from_currency, currency_current, CURRENCY_STR_LEN);
    from_currency_rate = currency_current_rate;
    
    updateCurrentCurrency();
    updateTextFields();
  }
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
  window_multi_click_subscribe(BUTTON_ID_SELECT, 2, 10, 0, true, select_multi_click_handler);
  window_long_click_subscribe(BUTTON_ID_SELECT, 700, select_long_click_handler, NULL);
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);

  // Equals sign
  from_layer = text_layer_create((GRect) { .origin = { 0, 20 }, .size = { bounds.size.w, 35 } });
  equals_layer = text_layer_create((GRect) { .origin = { 0, center.y - 20 }, .size = { bounds.size.w, 35 } });
  to_layer = text_layer_create((GRect) { .origin = { 0, bounds.size.h - 60 }, .size = { bounds.size.w, 35 } });
  no_currency_layer = text_layer_create((GRect) { .origin = { center.x, bounds.size.h - 20 }, .size = { center.x, 20 } });
  
  text_layer_set_text(from_layer, "No currencies");
  text_layer_set_font(from_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(from_layer, GTextAlignmentCenter);
  
  text_layer_set_text(equals_layer, "Doubleclick select");
  text_layer_set_font(equals_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(equals_layer, GTextAlignmentCenter);
  
  text_layer_set_text(to_layer, "to update");
  text_layer_set_font(to_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
  text_layer_set_text_alignment(to_layer, GTextAlignmentCenter);
  
  text_layer_set_text(no_currency_layer, "0 / 0");
  text_layer_set_text_alignment(no_currency_layer, GTextAlignmentRight);
  
  layer_add_child(window_layer, text_layer_get_layer(from_layer));
  layer_add_child(window_layer, text_layer_get_layer(equals_layer));
  layer_add_child(window_layer, text_layer_get_layer(to_layer));
  layer_add_child(window_layer, text_layer_get_layer(no_currency_layer));
}

static void window_unload(Window *window) {
  text_layer_destroy(from_layer);
  text_layer_destroy(equals_layer);
  text_layer_destroy(to_layer);
  text_layer_destroy(no_currency_layer);
}

static void init(void) {
  char rate[RATE_STR_LEN];
  
  window = window_create();
  window_set_click_config_provider(window, click_config_provider);
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  const bool animated = true;
  window_stack_push(window, animated);
  
  app_message_register_inbox_received(in_received_handler);
  app_message_register_inbox_dropped(in_dropped_handler);
  app_message_register_outbox_sent(out_sent_handler);
  app_message_register_outbox_failed(out_failed_handler);
  
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
  
  // Load data from persistent memory
  persist_read_string(CURRENCY, currencies, PERSIST_STRING_MAX_LENGTH);
  currency_length = persist_read_int(CURRENCY_LENGTH);
  currency_index = persist_read_int(CURRENCY_INDEX);
  persist_read_string(FROM_CURRENCY, from_currency, CURRENCY_STR_LEN);
  persist_read_string(FROM_CURRENCY_RATE, rate, RATE_STR_LEN);
  from_currency_rate = strtodbl(rate);
  
  if (currency_length > 0) {
	updateCurrentCurrency();
    updateTextFields();
  }
}

static void deinit(void) {
  char rate[RATE_STR_LEN];
  window_destroy(window);
  
  if (strlen(currencies) > 0) persist_write_string(CURRENCY, currencies);
  
  persist_write_int(CURRENCY_INDEX, currency_index);
  persist_write_string(FROM_CURRENCY, from_currency);
  ftoa(rate, from_currency_rate, 6);
  persist_write_string(FROM_CURRENCY_RATE, rate);
}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}
