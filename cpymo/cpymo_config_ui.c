#include "cpymo_prelude.h"
#include "cpymo_config_ui.h"
#include "cpymo_engine.h"
#include "cpymo_list_ui.h"
#include "cpymo_save_global.h"
#include "cpymo_localization.h"
#include <assert.h>
#include <math.h>

#define CONFIG_ITEMS 6

typedef struct {
	cpymo_backend_text show_name;
	cpymo_backend_text show_value;
	float show_value_width;
	int value, max_value, min_value;
	bool show_inc_dec_button;
} cpymo_config_ui_item;

typedef struct {
	cpymo_backend_text inc_btn, dec_btn;
	float inc_btn_w, dec_btn_w;
	cpymo_config_ui_item items[CONFIG_ITEMS];
	float font_size;

	cpymo_key_pluse left, right, ok, mouse_button;
} cpymo_config_ui;

#define ITEM_BGM_VOL 0
#define ITEM_SE_VOL 1
#define ITEM_VO_VOL 2
#define ITEM_TEXT_SPEED 3
#define ITEM_FONT_SIZE 4
#define ITEM_SKIP_ALREADY_READ_ONLY 5

const static float side_padding = 16;

static void *cpymo_config_ui_get_next_item(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	uintptr_t x = cpymo_list_ui_encode_uint_node_dec(cur);

	if (x >= CONFIG_ITEMS - 1) return NULL;
	else return cpymo_list_ui_encode_uint_node_enc((x + 1));
}

static void *cpymo_config_ui_get_prev_item(const cpymo_engine *e, const void *ui_data, const void *cur)
{
	uintptr_t x = cpymo_list_ui_encode_uint_node_dec(cur);
	if (x == 0) return NULL;
	else return cpymo_list_ui_encode_uint_node_enc((x - 1));
}

static inline void cpymo_config_ui_calc_layout(
	const cpymo_engine *e,
	const cpymo_config_ui *ui,
	size_t item_id,
	float y,
	const cpymo_config_ui_item **out_item,
	float *out_text_y,
	float *out_value_display_x,
	float *out_inc_btn_xywh,
	float *out_dec_btn_x,
	float *out_inc_dec_btn_symbol
)
{
	const cpymo_config_ui_item *item = 
		&ui->items[item_id];
	*out_item = item;

	float height = (float)e->gameconfig.imagesize_h / (float)CONFIG_ITEMS;

	float text_y = y + height / 2 + ui->font_size / 4;
	*out_text_y = text_y;

	const float max_font_size = 32 / 240.0f * e->gameconfig.imagesize_h * 0.8f;
	float inc_dec_btn_width = 0;
	float inc_dec_btn_space = 0;
	if (item->show_inc_dec_button) {
		inc_dec_btn_width = ui->dec_btn_w;
		if (ui->inc_btn_w > inc_dec_btn_width) inc_dec_btn_width = 
			ui->inc_btn_w;
		if (max_font_size > inc_dec_btn_width) inc_dec_btn_width = 
			max_font_size;
		inc_dec_btn_space = max_font_size / 2;
	}

	float right_edge = 
		(float)e->gameconfig.imagesize_w - side_padding;

	*out_value_display_x = 
		right_edge 
		- inc_dec_btn_space 
		- inc_dec_btn_width 
		- item->show_value_width;



	out_inc_btn_xywh[0] = right_edge - inc_dec_btn_width;
	out_inc_btn_xywh[1] = (height - max_font_size) / 2 + y;
	out_inc_btn_xywh[2] = inc_dec_btn_width;
	out_inc_btn_xywh[3] = max_font_size;

	float max_value_width = 0;
		for (size_t i = 0; i < CONFIG_ITEMS; ++i) 
			if (max_value_width < ui->items[i].show_value_width && 
				ui->items[i].show_inc_dec_button)
				max_value_width = ui->items[i].show_value_width;
				
	*out_dec_btn_x = 
		out_inc_btn_xywh[0] 
		- (max_value_width + 1.95f * inc_dec_btn_space + inc_dec_btn_width);

	*out_inc_dec_btn_symbol = y + height / 2 + max_font_size / 4;

}

static void cpymo_config_ui_draw_node(const cpymo_engine *e, const void *node_to_draw, float y)
{
	const cpymo_config_ui *ui = (cpymo_config_ui *)cpymo_list_ui_data_const(e);
	const cpymo_config_ui_item *item;
	float 
		text_y, 
		value_display_x,
		inc_dec_btn_xywh[4],
		dec_btn_x,
		inc_dec_btn_symbol_y;

	size_t item_index = cpymo_list_ui_encode_uint_node_dec(node_to_draw);

	cpymo_config_ui_calc_layout(
		e, ui, 
		item_index, y,
		&item,
		&text_y,
		&value_display_x,
		inc_dec_btn_xywh,
		&dec_btn_x,
		&inc_dec_btn_symbol_y);

	if (item->show_name) {
		cpymo_backend_text_draw(
			item->show_name, side_padding, text_y, cpymo_color_white, 1.0f,
			cpymo_backend_image_draw_type_ui_element);
	}

	if (item->show_value) {
		cpymo_backend_text_draw(
			item->show_value, 
			value_display_x, 
			text_y, cpymo_color_white, 1.0f, 
			cpymo_backend_image_draw_type_ui_element);
	}

	if (item->show_inc_dec_button) {
		bool inc_pressed = false, dec_pressed = false;

		if (cpymo_list_ui_get_current_selected_const(e) == node_to_draw)
		{
			dec_pressed = e->input.left;
			inc_pressed = e->input.right || e->input.ok;
		}

		bool is_pointed = false;
		if (e->input.mouse_position_useable) {
			is_pointed = cpymo_utils_point_in_rect_xywh(
				e->input.mouse_x,
				e->input.mouse_y,
				inc_dec_btn_xywh);

			if (e->input.mouse_button && is_pointed)
				inc_pressed = true;
		}

		cpymo_color pressed_color;
		pressed_color.r = 127;
		pressed_color.g = 127;
		pressed_color.b = 127;

		cpymo_backend_image_fill_rects(
			inc_dec_btn_xywh, 
			1, 
			inc_pressed ? pressed_color : 
				(is_pointed ? cpymo_color_white : cpymo_color_black),
			0.5f, 
			cpymo_backend_image_draw_type_ui_element);

		cpymo_backend_text_draw(
			ui->inc_btn,
			inc_dec_btn_xywh[0] + (inc_dec_btn_xywh[2] - ui->inc_btn_w) / 2, 
			inc_dec_btn_symbol_y, 
			is_pointed || inc_pressed ? cpymo_color_black : cpymo_color_white, 1.0f,
			cpymo_backend_image_draw_type_ui_element);
		
		inc_dec_btn_xywh[0] = dec_btn_x;

		if (e->input.mouse_position_useable) {
			is_pointed = cpymo_utils_point_in_rect_xywh(
				e->input.mouse_x,
				e->input.mouse_y,
				inc_dec_btn_xywh);

			if (e->input.mouse_button && is_pointed)
				dec_pressed = true;
		}

		cpymo_backend_image_fill_rects(
			inc_dec_btn_xywh, 
			1, 
			dec_pressed ? pressed_color : 
				(is_pointed ? cpymo_color_white : cpymo_color_black),
			0.5f, 
			cpymo_backend_image_draw_type_ui_element);

		cpymo_backend_text_draw(
			ui->dec_btn,
			inc_dec_btn_xywh[0] + (inc_dec_btn_xywh[2] - ui->inc_btn_w) / 2, 
			inc_dec_btn_symbol_y, 
			is_pointed || dec_pressed ? cpymo_color_black : cpymo_color_white, 1.0f,
			cpymo_backend_image_draw_type_ui_element);
	}
}

static void cpymo_config_ui_deleter(cpymo_engine *e, void *ui_data)
{
	cpymo_config_ui *ui = (cpymo_config_ui *)ui_data;
	for (size_t i = 0; i < sizeof(ui->items) / sizeof(ui->items[0]); ++i) {
		if (ui->items[i].show_name) cpymo_backend_text_free(ui->items[i].show_name);
		if (ui->items[i].show_value) cpymo_backend_text_free(ui->items[i].show_value);
	}

	if (ui->dec_btn) cpymo_backend_text_free(ui->dec_btn);
	if (ui->inc_btn) cpymo_backend_text_free(ui->inc_btn);

	cpymo_save_config_save(e);
}

static error_t cpymo_config_ui_set_value(
	cpymo_engine *e, 
	cpymo_config_ui *ui, 
	int item_index, 
	int val,
	bool refreshing);

static void cpymo_config_ui_refresh_items(cpymo_engine *e)
{
	cpymo_config_ui *ui = (cpymo_config_ui *)cpymo_list_ui_data(e);
	
	const cpymo_gameconfig *c = &e->gameconfig;
	ui->font_size = c->fontsize / 240.0f * c->imagesize_h * 0.8f;
	
	float width;
	cpymo_backend_text show_name;

	error_t err = CPYMO_ERR_SUCC;


	#define INIT_ITEM(ITEM_ID, TEXT, MIN_VAL, MAX_VAL, CUR_VAL, SHOW_INC_DEC_BTN) \
		ui->items[ITEM_ID].min_value = MIN_VAL; \
		ui->items[ITEM_ID].max_value = MAX_VAL; \
		ui->items[ITEM_ID].show_inc_dec_button = SHOW_INC_DEC_BTN; \
		err = cpymo_backend_text_create( \
			&show_name, \
			&width, \
			cpymo_str_pure(TEXT), \
			ui->font_size); \
		if (err == CPYMO_ERR_SUCC) { \
			if (ui->items[ITEM_ID].show_name) \
				cpymo_backend_text_free(ui->items[ITEM_ID].show_name); \
			ui->items[ITEM_ID].show_name = show_name; \
		} \
		err = cpymo_config_ui_set_value(e, ui, ITEM_ID, CUR_VAL, true); \
		if (err != CPYMO_ERR_SUCC) { \
			cpymo_ui_exit(e); \
			return; \
		}

	const cpymo_localization *l = cpymo_localization_get(e);

	#ifdef LOW_FRAME_RATE
	const static int min_say_speed = t;
	e->gameconfig.textspeed = 5;
	const static bool text_speed_inc_dec_btn = false;
	#else
	const static int min_say_speed = 0;
	const static bool text_speed_inc_dec_btn = true;
	#endif

		
	INIT_ITEM(
		ITEM_BGM_VOL, 
		l->config_bgmvol, 
		0, 
		10, 
		(int)roundf(
			cpymo_audio_get_channel_volume(
				CPYMO_AUDIO_CHANNEL_BGM, &e->audio) * 10),
		true);

	INIT_ITEM(
		ITEM_SE_VOL, 
		l->config_sevol, 
		0, 
		10, 
		(int)roundf(
			cpymo_audio_get_channel_volume(
				CPYMO_AUDIO_CHANNEL_SE, &e->audio) * 10),
		true);

	INIT_ITEM(
		ITEM_VO_VOL, 
		l->config_vovol, 
		0, 
		10, 
		(int)roundf(
			cpymo_audio_get_channel_volume(
				CPYMO_AUDIO_CHANNEL_VO, &e->audio) * 10),
		true);

	INIT_ITEM(
		ITEM_TEXT_SPEED, 
		l->config_sayspeed, 
		min_say_speed, 
		5, 
		(int)e->gameconfig.textspeed,
		text_speed_inc_dec_btn);

	INIT_ITEM(
		ITEM_FONT_SIZE, 
		l->config_fontsize, 
		12, 
		32, 
		(int)e->gameconfig.fontsize,
		true);

	INIT_ITEM(
		ITEM_SKIP_ALREADY_READ_ONLY, 
		l->config_skip_mode, 
		0, 
		1, 
		(int)e->config_skip_already_read_only,
		false);
	
	#undef INIT_ITEM
}

#ifdef ENABLE_TEXT_EXTRACT
static void cpymo_config_ui_extract_setting_title(cpymo_engine *e, int item_id)
{
	const cpymo_localization *l = cpymo_localization_get(e);
	const char *p = NULL;
	switch (item_id) {
	case ITEM_BGM_VOL: p = l->config_bgmvol; break;
	case ITEM_SE_VOL: p = l->config_sevol; break;
	case ITEM_VO_VOL: p = l->config_vovol; break;
	case ITEM_FONT_SIZE: p = l->config_fontsize; break;
	case ITEM_TEXT_SPEED: p = l->config_sayspeed; break;
	case ITEM_SKIP_ALREADY_READ_ONLY: p = l->config_skip_mode; break;
	default: assert(false);
	}

	cpymo_backend_text_extract(p);
}
#endif

static error_t cpymo_config_ui_set_value(
	cpymo_engine *e, 
	cpymo_config_ui *ui, 
	int item_index, 
	int val,
	bool refreshing)
{
	cpymo_config_ui_item *item = ui->items + item_index;

	item->value = val;
	if (item->show_value) {
		cpymo_backend_text_free(item->show_value);
		item->show_value = NULL;
	}

	const cpymo_localization *l = cpymo_localization_get(e);

	char val_str_buf[8];
	const char *val_str = val_str_buf;
	switch (item_index) {
	case ITEM_BGM_VOL:	
	case ITEM_SE_VOL:
	case ITEM_VO_VOL:
		if (val) sprintf(val_str_buf, "%d0%%", val);
		else val_str = "0%";
		break;
	case ITEM_FONT_SIZE:
		sprintf(val_str_buf, "%d", val);
		break;
	case ITEM_TEXT_SPEED:
		assert(val >= 0 && val <= 5);
		val_str = l->config_sayspeeds[val];
		break;
	case ITEM_SKIP_ALREADY_READ_ONLY:
		assert(val == 0 || val == 1);
		val_str = l->config_skip_modes[val];
		break;
	}

#ifdef ENABLE_TEXT_EXTRACT
	if (!refreshing) {
		cpymo_config_ui_extract_setting_title(e, item_index);
		cpymo_backend_text_extract(val_str);
	}
#endif

	error_t err = CPYMO_ERR_SUCC;
	if (!refreshing && item_index == ITEM_FONT_SIZE) goto JUST_REFRESH;
	
	err = cpymo_backend_text_create(
		&item->show_value, 
		&item->show_value_width, 
		cpymo_str_pure(val_str), 
		ui->font_size);

	if (err != CPYMO_ERR_SUCC)
		item->show_value = NULL;

	if (refreshing) return CPYMO_ERR_SUCC;

JUST_REFRESH:
	switch (item_index) {
	case ITEM_BGM_VOL:
		cpymo_audio_set_channel_volume(CPYMO_AUDIO_CHANNEL_BGM, &e->audio, (float)val / 10.0f);
		break;
	case ITEM_SE_VOL:
		cpymo_audio_set_channel_volume(CPYMO_AUDIO_CHANNEL_SE, &e->audio, (float)val / 10.0f);
		break;
	case ITEM_VO_VOL:
		cpymo_audio_set_channel_volume(CPYMO_AUDIO_CHANNEL_VO, &e->audio, (float)val / 10.0f);
		break;
	case ITEM_FONT_SIZE:
		e->gameconfig.fontsize = (uint16_t)val;
		cpymo_config_ui_refresh_items(e);
		break;
	case ITEM_TEXT_SPEED:
		e->gameconfig.textspeed = (unsigned)val;
		break;
	case ITEM_SKIP_ALREADY_READ_ONLY:
		e->config_skip_already_read_only = val > 0;
		break;
	default: assert(false);
	}

	cpymo_engine_request_redraw(e);

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_config_ui_item_inc(cpymo_engine *e, cpymo_config_ui *ui, int item_index)
{
	cpymo_config_ui_item *item = ui->items + item_index;
	int new_val = item->value + 1;
	if (new_val > item->max_value)
		new_val = item->min_value;

	if (new_val != item->value) {
		error_t err = cpymo_config_ui_set_value(e, ui, item_index, new_val, false);
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
		error_t err = cpymo_config_ui_set_value(e, ui, item_index, new_val, false);
		CPYMO_THROW(err);
	}

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_config_ui_update(cpymo_engine *e, float dt, void *sel)
{
	cpymo_config_ui *ui = (cpymo_config_ui *)cpymo_list_ui_data(e);
	cpymo_key_pluse_update(&ui->left, dt, e->input.left);
	cpymo_key_pluse_update(&ui->right, dt, e->input.right);
	cpymo_key_pluse_update(&ui->ok, dt, e->input.ok);
	cpymo_key_pluse_update(&ui->mouse_button, dt, e->input.mouse_button);

	const int i = (int)cpymo_list_ui_encode_uint_node_dec(sel);

	if (ui->items[i].show_inc_dec_button)
	{
		const cpymo_config_ui_item *item;
		float y;
		float x;

		float inc_dec_rect_xywh[4];
		float dec_rect_x;
		float symbol_y;
		cpymo_config_ui_calc_layout(
			e, ui, i, 
			((float)e->gameconfig.imagesize_h / CONFIG_ITEMS) * i,
			&item,
			&y,
			&x,
			inc_dec_rect_xywh,
			&dec_rect_x,
			&symbol_y);

		bool cur_pointed_plus = false, prev_pointed = false;
		if (e->input.mouse_position_useable)
			cur_pointed_plus = cpymo_utils_point_in_rect_xywh(
				e->input.mouse_x, e->input.mouse_y, inc_dec_rect_xywh);
		if (e->prev_input.mouse_position_useable)
			prev_pointed = cpymo_utils_point_in_rect_xywh(
				e->prev_input.mouse_x, e->prev_input.mouse_y, inc_dec_rect_xywh);
		if (cur_pointed_plus != prev_pointed)
			cpymo_engine_request_redraw(e);

		inc_dec_rect_xywh[0] = dec_rect_x;
		bool cur_pointed_minus = false;
		prev_pointed = false;
		if (e->input.mouse_position_useable)
			cur_pointed_minus = cpymo_utils_point_in_rect_xywh(
				e->input.mouse_x, e->input.mouse_y, inc_dec_rect_xywh);
		if (e->prev_input.mouse_position_useable)
			prev_pointed = cpymo_utils_point_in_rect_xywh(
				e->prev_input.mouse_x, e->prev_input.mouse_y, inc_dec_rect_xywh);
		if (cur_pointed_minus != prev_pointed)
			cpymo_engine_request_redraw(e);

		if (cur_pointed_minus && cpymo_key_pluse_output(&ui->mouse_button))
			cpymo_config_ui_item_dec(e, ui, i);		
		
		if (cur_pointed_plus && cpymo_key_pluse_output(&ui->mouse_button))
			cpymo_config_ui_item_inc(e, ui, i);

		if (e->prev_input.mouse_position_useable != e->input.mouse_position_useable ||
			(e->prev_input.mouse_button != e->input.mouse_button && (cur_pointed_minus || cur_pointed_plus)) ||
			e->prev_input.ok != e->input.ok ||
			e->prev_input.left != e->input.left ||
			e->prev_input.right != e->input.right)
			cpymo_engine_request_redraw(e);

		if (cpymo_key_pluse_output(&ui->left))
			cpymo_config_ui_item_dec(e, ui, i);
		else if (cpymo_key_pluse_output(&ui->right) || 
				(cpymo_key_pluse_output(&ui->ok)))
			cpymo_config_ui_item_inc(e, ui, i);
	}
	else {
		if (CPYMO_INPUT_JUST_PRESSED(e, left))
			cpymo_config_ui_item_dec(e, ui, i);
		else if (CPYMO_INPUT_JUST_PRESSED(e, right))
			cpymo_config_ui_item_inc(e, ui, i);
	}

	return CPYMO_ERR_SUCC;
}


static error_t cpymo_config_ui_ok(cpymo_engine *e, void *sel)
{
	cpymo_config_ui *ui = (cpymo_config_ui *)cpymo_list_ui_data(e);
	const int i = (int)cpymo_list_ui_encode_uint_node_dec(sel);

	return ui->items[i].show_inc_dec_button ?
		CPYMO_ERR_SUCC :
		cpymo_config_ui_item_inc(
			e, (cpymo_config_ui *)cpymo_list_ui_data(e), i);
}

#ifdef ENABLE_TEXT_EXTRACT
static error_t cpymo_config_ui_visual_im_help_selection_changed_callback(cpymo_engine *e, void *sel)
{
	const int i = (int)cpymo_list_ui_encode_uint_node_dec(sel);
	cpymo_config_ui_extract_setting_title(e, i);

	return CPYMO_ERR_SUCC;
}
#endif

error_t cpymo_config_ui_mouse_button_just_hold(cpymo_engine *e)
{ 
	cpymo_config_ui *ui = (cpymo_config_ui *)cpymo_list_ui_data(e);
	for (size_t i = 0; i < CONFIG_ITEMS; ++i) {
		if (!ui->items[i].show_inc_dec_button) continue;
		const cpymo_config_ui_item *item;
		float y;
		float x;

		float inc_dec_rect_xywh[4];
		float dec_rect_x;
		float symbol_y;
		cpymo_config_ui_calc_layout(
			e, ui, i, 
			((float)e->gameconfig.imagesize_h / CONFIG_ITEMS) * i,
			&item,
			&y,
			&x,
			inc_dec_rect_xywh,
			&dec_rect_x,
			&symbol_y);

		if (e->input.mouse_position_useable)
			if (cpymo_utils_point_in_rect_xywh(
				e->input.mouse_x, e->input.mouse_y, inc_dec_rect_xywh))
				return CPYMO_ERR_SUCC;

		inc_dec_rect_xywh[0] = dec_rect_x;
		if (e->input.mouse_position_useable)
			if (cpymo_utils_point_in_rect_xywh(
				e->input.mouse_x, e->input.mouse_y, inc_dec_rect_xywh))
				return CPYMO_ERR_SUCC;
	}

	cpymo_ui_exit(e);

	return CPYMO_ERR_SUCC; 
}

error_t cpymo_config_ui_enter(cpymo_engine *e)
{
	const cpymo_gameconfig *c = &e->gameconfig;
	const float font_size = c->fontsize / 240.0f * c->imagesize_h * 0.8f;

	cpymo_config_ui *ui = NULL;
	error_t err = cpymo_list_ui_enter(
		e,
		(void **)&ui,
		sizeof(cpymo_config_ui),
		&cpymo_config_ui_draw_node,
		&cpymo_config_ui_ok,
		&cpymo_config_ui_deleter,

		cpymo_list_ui_encode_uint_node_enc(ITEM_BGM_VOL),
		&cpymo_config_ui_get_next_item,
		&cpymo_config_ui_get_prev_item,
		false,
		CONFIG_ITEMS);
	CPYMO_THROW(err);

	ui->dec_btn = NULL;
	ui->dec_btn_w = 0;
	ui->inc_btn = NULL;
	ui->inc_btn_w = 0;

#ifdef ENABLE_TEXT_EXTRACT
	cpymo_list_ui_set_selection_changed_callback(e, &cpymo_config_ui_visual_im_help_selection_changed_callback);
#endif

	cpymo_list_ui_set_scroll_enabled(e, false);
	cpymo_list_ui_set_custom_update(e, &cpymo_config_ui_update);
	cpymo_list_ui_enable_loop(e);
	cpymo_list_ui_set_mouse_button_just_hold_callback(
		e, &cpymo_config_ui_mouse_button_just_hold);

	for (size_t i = 0; i < sizeof(ui->items) / sizeof(ui->items[0]); ++i) {
		ui->items[i].show_name = NULL;
		ui->items[i].show_value = NULL;
	}

	ui->font_size = font_size;

	cpymo_key_pluse_init(&ui->left, e->input.left);
	cpymo_key_pluse_init(&ui->right, e->input.right);
	cpymo_key_pluse_init(&ui->ok, e->input.ok);
	cpymo_key_pluse_init(&ui->mouse_button, e->input.mouse_button);

	cpymo_backend_text_extract(cpymo_localization_get(e)->config_bgmvol);

	err = cpymo_backend_text_create(
		&ui->inc_btn, 
		&ui->inc_btn_w,
		cpymo_str_pure("+"),
		32 / 240.0f * c->imagesize_h * 0.8f);
	if (err != CPYMO_ERR_SUCC) {
		ui->inc_btn = NULL;
		ui->dec_btn = NULL;
		cpymo_ui_exit(e);
		return err;
	}

	err = cpymo_backend_text_create(
		&ui->dec_btn,
		&ui->dec_btn_w,
		cpymo_str_pure("-"),
		32 / 240.0f * c->imagesize_h * 0.8f);
	if (err != CPYMO_ERR_SUCC) {
		ui->dec_btn = NULL;
		cpymo_ui_exit(e);
		return err;
	}

	cpymo_config_ui_refresh_items(e);

	return CPYMO_ERR_SUCC;
}

