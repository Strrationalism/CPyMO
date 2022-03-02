#include "cpymo_config_ui.h"
#include "cpymo_engine.h"
#include "cpymo_list_ui.h"
#include "cpymo_save_global.h"
#include <assert.h>

typedef struct {
	cpymo_backend_text show_name;
	cpymo_backend_text show_value;
	float show_value_width;
	int value, max_value, min_value;
} cpymo_config_ui_item;

typedef struct {
	cpymo_config_ui_item items[5];
	float font_size;

	cpymo_key_pluse left, right;
} cpymo_config_ui;

#define ITEM_BGM_VOL 0
#define ITEM_SE_VOL 1
#define ITEM_VO_VOL 2
#define ITEM_TEXT_SPEED 3
#define ITEM_FONT_SIZE 4

#define LR_PADDING 16

static void *cpymo_config_ui_get_next_item(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	uintptr_t x = CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(cur);

	if (x >= 4) return NULL;
	else return CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC((x + 1));
}

static void *cpymo_config_ui_get_prev_item(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	uintptr_t x = CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(cur);
	if (x == 0) return NULL;
	else return CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC((x - 1));
}

static void cpymo_config_ui_draw_node(const cpymo_engine *e, const void *node_to_draw, float y)
{
	const cpymo_config_ui *ui = (cpymo_config_ui *)cpymo_list_ui_data_const(e);
	const cpymo_config_ui_item *item = &ui->items[CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(node_to_draw)];

	const float height = (float)e->gameconfig.imagesize_h / 5.0f;
	y += height / 2 + ui->font_size / 2;
	y -= ui->font_size / 4;

	if (item->show_name) {
		cpymo_backend_text_draw(
			item->show_name, LR_PADDING, y, cpymo_color_white, 1.0f,
			cpymo_backend_image_draw_type_ui_element);
	}

	if (item->show_value) {
		cpymo_backend_text_draw(
			item->show_value, (float)e->gameconfig.imagesize_w - item->show_value_width - LR_PADDING, 
			y, cpymo_color_white, 1.0f, cpymo_backend_image_draw_type_ui_element);
	}
}

static void cpymo_config_ui_deleter(cpymo_engine *e, void *ui_data)
{
	cpymo_config_ui *ui = (cpymo_config_ui *)ui_data;
	for (size_t i = 0; i < sizeof(ui->items) / sizeof(ui->items[0]); ++i) {
		if (ui->items[i].show_name) cpymo_backend_text_free(ui->items[i].show_name);
		if (ui->items[i].show_value) cpymo_backend_text_free(ui->items[i].show_value);
	}

	cpymo_save_config_save(e);
}

static error_t cpymo_config_ui_set_value(cpymo_engine *e, cpymo_config_ui *ui, int item_index, int val)
{
	cpymo_config_ui_item *item = ui->items + item_index;

	item->value = val;
	if (item->show_value) {
		cpymo_backend_text_free(item->show_value);
		item->show_value = NULL;
	}

	char val_str[8];
	sprintf(val_str, "%d", val);
	error_t err = cpymo_backend_text_create(
		&item->show_value, 
		&item->show_value_width, 
		cpymo_parser_stream_span_pure(val_str), 
		ui->font_size);

	if (err != CPYMO_ERR_SUCC)
		item->show_value = NULL;

	switch (item_index) {
	case ITEM_BGM_VOL:
		e->audio.channels[CPYMO_AUDIO_CHANNEL_BGM].volume = (float)val / 10.0f;
		break;
	case ITEM_SE_VOL:
		e->audio.channels[CPYMO_AUDIO_CHANNEL_SE].volume = (float)val / 10.0f;
		break;
	case ITEM_VO_VOL:
		e->audio.channels[CPYMO_AUDIO_CHANNEL_VO].volume = (float)val / 10.0f;
		break;
	case ITEM_FONT_SIZE:
		e->gameconfig.fontsize = (uint16_t)val;
		break;
	case ITEM_TEXT_SPEED:
		e->gameconfig.textspeed = (unsigned)val;
		break;
	default: assert(false);
	}

	cpymo_engine_request_redraw(e);

	return err;
}

static error_t cpymo_config_ui_item_inc(cpymo_engine *e, cpymo_config_ui *ui, int item_index)
{
	cpymo_config_ui_item *item = ui->items + item_index;
	int new_val = item->value + 1;
	if (new_val > item->max_value)
		new_val = item->min_value;

	if (new_val != item->value) {
		error_t err = cpymo_config_ui_set_value(e, ui, item_index, new_val);
		CPYMO_THROW(err);
	}

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_config_ui_item_dec(cpymo_engine *e, cpymo_config_ui *ui, int item_index)
{
	cpymo_config_ui_item *item = ui->items + item_index;
	int new_val = item->value - 1;
	if (new_val < item->min_value)
		new_val = item->max_value;

	if (new_val != item->value) {
		error_t err = cpymo_config_ui_set_value(e, ui, item_index, new_val);
		CPYMO_THROW(err);
	}

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_config_ui_update(cpymo_engine *e, float dt, void *sel)
{
	cpymo_config_ui *ui = (cpymo_config_ui *)cpymo_list_ui_data(e);
	cpymo_key_pluse_update(&ui->left, dt, e->input.left);
	cpymo_key_pluse_update(&ui->right, dt, e->input.right);

	const int i = (int)CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(sel);
	if (cpymo_key_pluse_output(&ui->left))
		cpymo_config_ui_item_dec(e, ui, i);
	else if (cpymo_key_pluse_output(&ui->right))
		cpymo_config_ui_item_inc(e, ui, i);

	return CPYMO_ERR_SUCC;
}


static error_t cpymo_config_ui_ok(cpymo_engine *e, void *selected)
{
	return cpymo_config_ui_item_inc(
		e, 
		(cpymo_config_ui *)cpymo_list_ui_data(e), 
		(int)CPYMO_LIST_UI_ENCODE_UINT_NODE_DEC(selected));
}

error_t cpymo_config_ui_enter(cpymo_engine *e)
{
	cpymo_config_ui *ui = NULL;
	error_t err = cpymo_list_ui_enter(
		e,
		(void **)&ui,
		sizeof(cpymo_config_ui),
		&cpymo_config_ui_draw_node,
		&cpymo_config_ui_ok,
		&cpymo_config_ui_deleter,
		CPYMO_LIST_UI_ENCODE_UINT_NODE_ENC(ITEM_BGM_VOL),
		&cpymo_config_ui_get_next_item,
		&cpymo_config_ui_get_prev_item,
		false,
		5);
	CPYMO_THROW(err);

	cpymo_list_ui_set_scroll_enabled(e, false);
	cpymo_list_ui_set_custom_update(e, &cpymo_config_ui_update);

	for (size_t i = 0; i < sizeof(ui->items) / sizeof(ui->items[0]); ++i) {
		ui->items[i].show_name = NULL;
		ui->items[i].show_value = NULL;
	}

	const cpymo_gameconfig *c = &e->gameconfig;
	ui->font_size = c->fontsize / 240.0f * c->imagesize_h * 0.8f;

	cpymo_key_pluse_init(&ui->left, e->input.left);
	cpymo_key_pluse_init(&ui->right, e->input.right);

	float width;
	#define INIT_ITEM(ITEM_ID, TEXT, MIN_VAL, MAX_VAL, CUR_VAL) \
		ui->items[ITEM_ID].min_value = MIN_VAL; \
		ui->items[ITEM_ID].max_value = MAX_VAL; \
		err = cpymo_backend_text_create( \
			&ui->items[ITEM_ID].show_name, \
			&width, \
			cpymo_parser_stream_span_pure(TEXT), \
			ui->font_size); \
		if (err != CPYMO_ERR_SUCC) { \
			cpymo_ui_exit(e); \
			return err; \
		} \
		err = cpymo_config_ui_set_value(e, ui, ITEM_ID, CUR_VAL); \
		if (err != CPYMO_ERR_SUCC) { \
			cpymo_ui_exit(e); \
			return err; \
		}
		
	INIT_ITEM(ITEM_BGM_VOL, "背景音乐音量", 0, 10, (int)roundf(e->audio.channels[CPYMO_AUDIO_CHANNEL_BGM].volume * 10));
	INIT_ITEM(ITEM_SE_VOL, "音效音量", 0, 10, (int)roundf(e->audio.channels[CPYMO_AUDIO_CHANNEL_SE].volume * 10));
	INIT_ITEM(ITEM_VO_VOL, "语音音量", 0, 10, (int)roundf(e->audio.channels[CPYMO_AUDIO_CHANNEL_VO].volume * 10));
	INIT_ITEM(ITEM_TEXT_SPEED, "文字速度", 0, 5, (int)e->gameconfig.textspeed);
	INIT_ITEM(ITEM_FONT_SIZE, "文字大小", 12, 32, (int)e->gameconfig.fontsize);

	return CPYMO_ERR_SUCC;
}
