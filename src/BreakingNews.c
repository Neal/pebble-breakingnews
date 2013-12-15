#include <pebble.h>
#include "libs/pebble-assist.h"

#define MAX_STORIES 20

typedef struct {
	int index;
	char title[160];
	GSize size;
} BNStory;

enum {
	BN_KEY_INDEX,
	BN_KEY_TITLE,
	BN_KEY_ERROR,
};

static BNStory stories[MAX_STORIES];

static int num_stories;
static char error[24];

static Window *window;
static MenuLayer *menu_layer;

static void in_received_handler(DictionaryIterator *iter, void *context) {
	Tuple *index_tuple = dict_find(iter, BN_KEY_INDEX);
	Tuple *title_tuple = dict_find(iter, BN_KEY_TITLE);
	Tuple *error_tuple = dict_find(iter, BN_KEY_ERROR);

	if (index_tuple && title_tuple) {
		BNStory story;
		story.index = index_tuple->value->int16;
		strncpy(story.title, title_tuple->value->cstring, sizeof(story.title));
		stories[story.index] = story;
		num_stories++;
		menu_layer_reload_data_and_mark_dirty(menu_layer);
		APP_LOG(APP_LOG_LEVEL_DEBUG, "received story [%d] %s", story.index, story.title);
	}

	if (error_tuple) {
		strncpy(error, error_tuple->value->cstring, sizeof(error));
		menu_layer_reload_data_and_mark_dirty(menu_layer);
	}
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Incoming AppMessage from Pebble dropped, %d", reason);
}

static void out_sent_handler(DictionaryIterator *sent, void *context) {
	// outgoing message was delivered
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to send AppMessage to Pebble");
}

static void refresh_list() {
	memset(stories, 0x0, sizeof(stories));
	num_stories = 0;
	error[0] = '\0';
	menu_layer_set_selected_index(menu_layer, (MenuIndex) { .row = 0, .section = 0 }, MenuRowAlignBottom, false);
	menu_layer_reload_data_and_mark_dirty(menu_layer);
	app_message_outbox_send();
}

static uint16_t menu_get_num_sections_callback(struct MenuLayer *menu_layer, void *callback_context) {
	return 1;
}

static uint16_t menu_get_num_rows_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return (num_stories) ? num_stories : 1;
}

static int16_t menu_get_header_height_callback(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context) {
	return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static int16_t menu_get_cell_height_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	if (num_stories != 0) {
		return stories[cell_index->row].size.h + 8;
	}
	return 36;
}

static void menu_draw_header_callback(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context) {
	menu_cell_basic_header_draw(ctx, cell_layer, "Breaking News");
}

static void menu_draw_row_callback(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
	if (strlen(error) != 0) {
		menu_cell_title_draw(ctx, cell_layer, "Error!");
	} else if (num_stories == 0) {
		menu_cell_title_draw(ctx, cell_layer, "Loading...");
	} else {
		char label[160] = "";
		strncpy(label, stories[cell_index->row].title, sizeof(label));
		stories[cell_index->row].size = graphics_text_layout_get_max_used_size(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_18), (GRect) { .origin = { 4, 0 }, .size = { PEBBLE_WIDTH - 8, 128 } }, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
		graphics_context_set_text_color(ctx, GColorBlack);
		graphics_draw_text(ctx, label, fonts_get_system_font(FONT_KEY_GOTHIC_18), (GRect) { .origin = { 4, 0 }, .size = { PEBBLE_WIDTH - 8, 128 } }, GTextOverflowModeFill, GTextAlignmentLeft, NULL);
	}
}

static void menu_select_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
}

static void menu_select_long_callback(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	refresh_list();
}

static void appmessage_init(void) {
	app_message_open(192 /* inbound_size */, 2 /* outbound_size */);
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_sent(out_sent_handler);
	app_message_register_outbox_failed(out_failed_handler);
}

static void init(void) {
	appmessage_init();

	window = window_create();

	menu_layer = menu_layer_create_fullscreen(window);
	menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks) {
		.get_num_sections = menu_get_num_sections_callback,
		.get_num_rows = menu_get_num_rows_callback,
		.get_header_height = menu_get_header_height_callback,
		.get_cell_height = menu_get_cell_height_callback,
		.draw_header = menu_draw_header_callback,
		.draw_row = menu_draw_row_callback,
		.select_click = menu_select_callback,
		.select_long_click = menu_select_long_callback,
	});
	menu_layer_set_click_config_onto_window(menu_layer, window);
	menu_layer_add_to_window(menu_layer, window);

	window_stack_push(window, true /* animated */);

	refresh_list();
}

static void deinit(void) {
	menu_layer_destroy_safe(menu_layer);
	window_destroy_safe(window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}
