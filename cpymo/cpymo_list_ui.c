#include "cpymo_prelude.h"
#include "cpymo_list_ui.h"
#include "cpymo_engine.h"
#include <assert.h>
#include <limits.h>
#include <math.h>

#ifdef ENABLE_TEXT_EXTRACT_ANDROID_ACCESSIBILITY
#include "../cpymo-backends/android/app/src/main/cpp/include/cpymo_android.h"
#endif

#ifdef __3DS__
extern bool fill_screen_enabled;
const extern bool fill_screen;
const extern bool enhanced_3ds_display_mode;
#endif

const static float slide_limit = 10.0f;

typedef struct cpymo_list_ui {
	cpymo_ui_deleter ui_data_deleter;
	cpymo_list_ui_draw_node draw_node;
	cpymo_list_ui_ok ok;
	cpymo_list_ui_custom_update custom_update;
	cpymo_list_ui_selection_changed selection_changed;
	void *current_node;
	int selection_relative_to_cur;
	cpymo_list_ui_get_node get_next;
	cpymo_list_ui_get_node get_prev;
	bool from_bottom_to_top;
	float node_height;

	float mouse_key_press_time;

	cpymo_key_pluse key_up, key_down;
	cpymo_key_hold key_mouse_button;
	float scroll_delta_y_sum;
	float scroll_speed;

	bool allow_scroll;

	bool allow_exit_list_ui;

	cpymo_list_ui_selecting_no_more_content_callback no_more_content_callback;
	cpymo_list_ui_mouse_button_just_hold_callback just_hold_callback;

	float current_y;
	size_t nodes_per_screen;
	float mouse_touch_move_y_sum;
} cpymo_list_ui;

void cpymo_list_ui_ignore_key_hold_exit_onetime(struct cpymo_engine *e)
{
	cpymo_list_ui *ui = (cpymo_list_ui *)cpymo_ui_data_const(e);
	cpymo_key_hold_init(&ui->key_mouse_button, true);
}

static inline float cpymo_list_ui_get_y(const cpymo_engine *e, int relative_to_current)
{
	const cpymo_list_ui *ui = (cpymo_list_ui *)cpymo_ui_data_const(e);

	return ui->from_bottom_to_top ?
		(float)e->gameconfig.imagesize_h - (1 + relative_to_current) * ui->node_height - ui->current_y :
		ui->current_y + relative_to_current * ui->node_height;
}

static int cpymo_list_ui_get_selection_relative_to_cur_by_mouse(const cpymo_engine *e)
{
	const cpymo_input *input = &e->input;
	if (!input->mouse_position_useable) {
		input = &e->prev_input;
		if (!input->mouse_position_useable)
			return INT_MAX;
	}

	const float mouse_y = input->mouse_y;

	const cpymo_list_ui *ui = (cpymo_list_ui *)cpymo_ui_data_const(e);

	float y = ui->from_bottom_to_top ?
		(float)e->gameconfig.imagesize_h - ui->node_height - ui->current_y :
		ui->current_y;

	void *prev = ui->get_prev(e, cpymo_list_ui_data_const(e), ui->current_node);
	if (prev) {
		float top = y - (ui->from_bottom_to_top ? -ui->node_height : ui->node_height);
		float bottom = top + ui->node_height;
		
		if (mouse_y >= top && mouse_y < bottom) return -1;
	}

	int cur = 0;

	void *node = ui->current_node;
	while (ui->from_bottom_to_top ?
		y > -ui->node_height :
		y < (float)e->gameconfig.imagesize_h) {

		float bottom = y + ui->node_height;
		if (mouse_y >= y && mouse_y < bottom) return cur;

		y += ui->from_bottom_to_top ? -ui->node_height : ui->node_height;
		node = ui->get_next(e, cpymo_list_ui_data_const(e), node);
		if (node == NULL) break;

		cur++;
	}

	return INT_MAX;
}

static void *cpymo_list_ui_get_relative_id_to_cur(const cpymo_engine *e, int id)
{
	const cpymo_list_ui *ui = (cpymo_list_ui *)cpymo_ui_data_const(e);
	void *node = ui->current_node;

	while (id && node) {
		if (id > 0) {
			id--;
			node = ui->get_next(e, ui + 1, node);
		}
		else if (id < 0) {
			id++;
			node = ui->get_prev(e, ui + 1, node);
		}
	}

	return node;
}

static void cpymo_list_ui_fix_key_scroll(cpymo_engine *e)
{
	cpymo_list_ui *ui = (cpymo_list_ui *)cpymo_ui_data(e);

	float y = cpymo_list_ui_get_y(e, ui->selection_relative_to_cur);

	bool a = y < 0;
	bool b = y > e->gameconfig.imagesize_h - ui->node_height;

	if (ui->from_bottom_to_top) {
		bool t = a;
		a = b;
		b = t;
	}

	if (a) {
		ui->current_y = 0;

		ui->current_node = cpymo_list_ui_get_relative_id_to_cur(
			e, ui->selection_relative_to_cur);
		ui->selection_relative_to_cur = 0;
	}
	else if (b) {
		ui->current_y = 0;
		ui->current_node = cpymo_list_ui_get_relative_id_to_cur(
			e, ui->selection_relative_to_cur);
		ui->selection_relative_to_cur = 0;

		while (ui->selection_relative_to_cur < (int)ui->nodes_per_screen - 1) {
			void *next_node = ui->get_prev(e, ui + 1, ui->current_node);
			if (next_node == NULL) break;
			ui->current_node = next_node;
			ui->selection_relative_to_cur++;
		}
	}
}

static bool cpymo_list_ui_is_last_node(cpymo_engine *e, void *ui_data)
{
	cpymo_list_ui *ui = (cpymo_list_ui *)ui_data;
	bool is_last_page = false;
	void *node = ui->current_node;
	for (size_t i = 0; i < ui->nodes_per_screen; ++i) {
		if (node == NULL) {
			is_last_page = true;
			break;
		}

		node = ui->get_next(e, ui + 1, node);

		if (node == NULL) {
			is_last_page = true;
			break;
		}
	}

	return is_last_page;
}

static error_t cpymo_list_ui_update(cpymo_engine *e, void *ui_data, float d)
{
	cpymo_list_ui *ui = (cpymo_list_ui *)ui_data;

	enum cpymo_key_hold_result mouse_button_state =
		cpymo_key_hold_update(e, &ui->key_mouse_button, d, e->input.mouse_button);

	if (CPYMO_INPUT_JUST_RELEASED(e, cancel)) {
		cpymo_list_ui_exit(e); 
		return CPYMO_ERR_SUCC;
	}

	float mouse_y_delta = 0;
	if (e->prev_input.mouse_position_useable && 
		e->input.mouse_position_useable)
		mouse_y_delta = e->input.mouse_y - e->prev_input.mouse_y;

	if (CPYMO_INPUT_JUST_RELEASED(e, mouse_button))
		cpymo_engine_request_redraw(e);

	if (CPYMO_INPUT_JUST_PRESSED(e, mouse_button))
		ui->mouse_touch_move_y_sum = 0;

	if (e->input.mouse_button && e->prev_input.mouse_button)
		ui->mouse_touch_move_y_sum += fabsf(mouse_y_delta);

	bool scrolled = false;
#ifndef LOW_FRAME_RATE
	if (ui->allow_scroll) {
		float prev_speed = fabsf(ui->scroll_speed);
		if (e->input.mouse_button) {
			ui->scroll_speed = 0;
			if (e->prev_input.mouse_position_useable && 
				e->input.mouse_position_useable)
				ui->scroll_speed = mouse_y_delta;
		}
		else {
			if (fabsf(ui->scroll_speed) >= 0.001f) {
				const float spd_dec = d * 100;
				if (fabsf(ui->scroll_speed) - spd_dec < spd_dec)
					ui->scroll_speed = 0;
				else {
					if (ui->scroll_speed > 0) ui->scroll_speed -= spd_dec;
					else ui->scroll_speed += spd_dec;

					ui->current_y += 
						ui->from_bottom_to_top ?
							-ui->scroll_speed : ui->scroll_speed;
					scrolled = true;
					cpymo_engine_request_redraw(e);
				}
			}
			else ui->scroll_speed = 0;

			if (ui->get_prev(e, ui + 1, ui->current_node) == NULL) {
				if (ui->current_y > 0) {
					ui->current_y = 0;
					ui->scroll_speed = 0;
				}
			}

			if (cpymo_list_ui_is_last_node(e, ui_data)) {
				if (ui->current_y < 0) {
					ui->current_y = 0;
					ui->scroll_speed = 0;
				}
			}
		}

		float cur_speed = fabsf(ui->scroll_speed);
		if (prev_speed > 0.0001f && cur_speed <= 0.0001f)
			cpymo_engine_request_redraw(e);
	}
#endif	

	if ((e->input.mouse_button || e->input.mouse_wheel_delta || scrolled) && ui->allow_scroll) {
		ui->mouse_key_press_time += d;
		float delta_y = mouse_y_delta;
		if (!e->input.mouse_button) delta_y = 0;

		if (mouse_button_state == cpymo_key_hold_result_just_press) {
			ui->mouse_key_press_time = 0;
			delta_y = 0;
		}

		ui->scroll_delta_y_sum = ui->scroll_delta_y_sum > fabsf(delta_y) ? ui->scroll_delta_y_sum : fabsf(delta_y);

		bool control_by_wheel = fabs(e->input.mouse_wheel_delta) > fabs(delta_y);
		if (control_by_wheel) {
			delta_y = e->input.mouse_wheel_delta * ui->node_height * 0.5f;
		}

		ui->current_y += ui->from_bottom_to_top ? -delta_y : delta_y;

		bool a = delta_y > 0;
		bool b = delta_y < 0;

		if (ui->from_bottom_to_top) {
			bool tmp = b;
			b = a;
			a = tmp;
		}
		
		a = a && ui->current_y > 0;
		b = b && ui->current_y < 0;

		if (a) {
			void *p = ui->get_prev(e, ui + 1, ui->current_node);
			if (p == NULL) ui->current_y = 0;
		}
		else if (b) {
			void *p = ui->get_next(e, ui + 1, ui->current_node);
			if (p == NULL) ui->current_y = 0;

			if (cpymo_list_ui_is_last_node(e, ui_data)) ui->current_y = 0;
		}

		while (ui->current_y >= ui->node_height) {
			ui->current_y -= ui->node_height;
			void *p = ui->get_prev(e, ui + 1, ui->current_node);
			if (p) {
				ui->current_node = p;
				ui->selection_relative_to_cur++;
			}
		}

		while (ui->current_y <= -ui->node_height) {
			ui->current_y += ui->node_height;
			void *p = ui->get_next(e, ui + 1, ui->current_node);
			if (p) {
				ui->current_node = p;
				ui->selection_relative_to_cur--;
			}
		}

		if (ui->selection_relative_to_cur < 0)
			ui->selection_relative_to_cur = 0;

		if (ui->selection_relative_to_cur > (int)ui->nodes_per_screen - 1)
			ui->selection_relative_to_cur = (int)ui->nodes_per_screen - 1;

		if (!control_by_wheel) {
			int s = cpymo_list_ui_get_selection_relative_to_cur_by_mouse(e);
			if (s != INT_MAX) 
				ui->selection_relative_to_cur = s;
		}

		cpymo_engine_request_redraw(e);
	}

	cpymo_key_pluse_update(&ui->key_up, d, e->input.up);
	cpymo_key_pluse_update(&ui->key_down, d, e->input.down);

	bool just_press_up = cpymo_key_pluse_output(&ui->key_up);
	bool just_press_down = cpymo_key_pluse_output(&ui->key_down);

	if (ui->from_bottom_to_top) {
		bool tmp = just_press_up;
		just_press_up = just_press_down;
		just_press_down = tmp;
	}

	if (just_press_up) {
		int r = ui->selection_relative_to_cur - 1;
		if (cpymo_list_ui_get_relative_id_to_cur(e, r)) {
			ui->selection_relative_to_cur = r;
			cpymo_list_ui_fix_key_scroll(e);
			cpymo_engine_request_redraw(e);
		}
		else {
			if (ui->no_more_content_callback) {
				error_t err = ui->no_more_content_callback(e, false);
				CPYMO_THROW(err);
				if (!cpymo_ui_enabled(e))
					return CPYMO_ERR_SUCC;
			}

			cpymo_list_ui_fix_key_scroll(e);
		}

		if (ui->selection_changed) {
#ifdef ENABLE_TEXT_EXTRACT_ANDROID_ACCESSIBILITY
			cpymo_android_play_sound(SOUND_SELECT);
#endif
			error_t err = ui->selection_changed(e,
				cpymo_list_ui_get_relative_id_to_cur(e, ui->selection_relative_to_cur));
			CPYMO_THROW(err);
		}
	}

	else if (just_press_down) {
		int r = ui->selection_relative_to_cur + 1;
		if (cpymo_list_ui_get_relative_id_to_cur(e, r)) {
			ui->selection_relative_to_cur = r;
			cpymo_list_ui_fix_key_scroll(e);
			cpymo_engine_request_redraw(e);
		}
		else {
			if (ui->no_more_content_callback) {
				error_t err = ui->no_more_content_callback(e, true);
				CPYMO_THROW(err);
				if (!cpymo_ui_enabled(e))
					return CPYMO_ERR_SUCC;
			}

			cpymo_list_ui_fix_key_scroll(e);
		}

		if (ui->selection_changed) {
#ifdef ENABLE_TEXT_EXTRACT_ANDROID_ACCESSIBILITY
			cpymo_android_play_sound(SOUND_SELECT);
#endif
			error_t err = ui->selection_changed(e,
				cpymo_list_ui_get_relative_id_to_cur(e, ui->selection_relative_to_cur));
			CPYMO_THROW(err);
		}
	}

	if (cpymo_input_mouse_moved(e)) {
		int s = cpymo_list_ui_get_selection_relative_to_cur_by_mouse(e);
		if (s != ui->selection_relative_to_cur && s != INT_MAX) {
			ui->selection_relative_to_cur = s;
			cpymo_engine_request_redraw(e);

			if (ui->selection_changed) {
#ifdef ENABLE_TEXT_EXTRACT_ANDROID_ACCESSIBILITY
				cpymo_android_play_sound(SOUND_SELECT);
#endif
				error_t err = ui->selection_changed(e, 
					cpymo_list_ui_get_relative_id_to_cur(e, ui->selection_relative_to_cur));
				CPYMO_THROW(err);
			}
		}
	}

	if (ui->custom_update) {
		void *obj = cpymo_list_ui_get_relative_id_to_cur(e, ui->selection_relative_to_cur);
		error_t err = ui->custom_update(e, d, obj);
		CPYMO_THROW(err);

		if (!cpymo_ui_enabled(e))
			return CPYMO_ERR_SUCC;
	}

	if (mouse_button_state == cpymo_key_hold_result_just_hold && ui->scroll_delta_y_sum < 5.0f && ui->mouse_touch_move_y_sum < slide_limit) {
		if (ui->just_hold_callback) 
			return ui->just_hold_callback(e);
		else {
			cpymo_list_ui_exit(e);
			return CPYMO_ERR_SUCC;
		}
	}
	else if (CPYMO_INPUT_JUST_PRESSED(e, ok)) {
		void *obj = cpymo_list_ui_get_relative_id_to_cur(e, ui->selection_relative_to_cur);
		error_t err = ui->ok(e, obj);
		CPYMO_THROW(err);
	}
	else if (mouse_button_state == cpymo_key_hold_result_just_released && ui->mouse_key_press_time < 0.15f && ui->mouse_touch_move_y_sum < slide_limit) {
		int selected = cpymo_list_ui_get_selection_relative_to_cur_by_mouse(e);
		if (selected != INT_MAX) {
			if (ui->selection_relative_to_cur != selected) {
				ui->selection_relative_to_cur = selected;
				cpymo_engine_request_redraw(e);
			}
			void *obj = cpymo_list_ui_get_relative_id_to_cur(e, ui->selection_relative_to_cur);
			error_t err = ui->ok(e, obj);
			CPYMO_THROW(err);
		}
	}
	else if (mouse_button_state == cpymo_key_hold_result_released)
		ui->scroll_delta_y_sum = 0;

	return CPYMO_ERR_SUCC;
}

const void *cpymo_list_ui_get_current_selected_const(
	const struct cpymo_engine * e)
{
	const cpymo_list_ui *ui = (cpymo_list_ui *)cpymo_ui_data_const(e);
	return cpymo_list_ui_get_relative_id_to_cur(
		e, ui->selection_relative_to_cur);
}

static void cpymo_list_ui_draw(const cpymo_engine *e, const void *ui_data)
{
	cpymo_bg_draw(e);
	cpymo_scroll_draw(&e->scroll);

	#ifdef __3DS__
	if (fill_screen && !enhanced_3ds_display_mode)
		fill_screen_enabled = false;
	#endif

	float xywh[] = {-100, -100, 200 + (float)e->gameconfig.imagesize_w, 200 + (float)e->gameconfig.imagesize_h };
	cpymo_backend_image_fill_rects(xywh, 1, cpymo_color_black, 0.5f, cpymo_backend_image_draw_type_ui_bg);

	const cpymo_list_ui *ui = (cpymo_list_ui *)ui_data;

	bool is_sliding = 
		e->input.mouse_button && ui->scroll_delta_y_sum >= slide_limit;
	bool is_sliding_inertia = 
		fabs(ui->scroll_speed) > 0.001f && !e->input.mouse_button;
	if (!is_sliding && !is_sliding_inertia) {
		xywh[1] = cpymo_list_ui_get_y(e, ui->selection_relative_to_cur);
		xywh[3] = ui->node_height;
		cpymo_backend_image_fill_rects(
			xywh, 1, cpymo_color_white, 0.5f, 
			cpymo_backend_image_draw_type_ui_element_bg);
	}

	// For Debugging!
	/*cpymo_color red;
	red.r = 255;
	red.g = 0;
	red.b = 0;
	xywh[1] = cpymo_list_ui_get_y(e, 0);
	xywh[3] = ui->node_height;
	cpymo_backend_image_fill_rects(xywh, 1, red, 0.5f, cpymo_backend_image_draw_type_bg);*/

	float y = ui->from_bottom_to_top ? 
		(float)e->gameconfig.imagesize_h - ui->node_height - ui->current_y : 
		ui->current_y;

	void *prev = ui->get_prev(e, cpymo_list_ui_data_const(e), ui->current_node);
	if (prev) {
		ui->draw_node(e, prev, y - (ui->from_bottom_to_top ? -ui->node_height : ui->node_height));	
	}

	void *node = ui->current_node;
	while (ui->from_bottom_to_top ? 
			y > -ui->node_height : 
			y < (float)e->gameconfig.imagesize_h) {
		ui->draw_node(e, node, y);

		y += ui->from_bottom_to_top ? -ui->node_height : ui->node_height;
		node = ui->get_next(e, cpymo_list_ui_data_const(e), node);
		if (node == NULL) break;
	}

	#ifdef __3DS__
	fill_screen_enabled = true;
	#endif
}

static void cpymo_list_ui_delete(cpymo_engine *e, void *ui)
{
	cpymo_list_ui *list_ui = (cpymo_list_ui *)ui;
	list_ui->ui_data_deleter(e, cpymo_list_ui_data(e));
}

error_t cpymo_list_ui_enter(
	cpymo_engine *e,
	void **out_ui_data,
	size_t ui_data_size,
	cpymo_list_ui_draw_node draw_node,
	cpymo_list_ui_ok ok,
	cpymo_ui_deleter ui_data_deleter,
	void *current,
	cpymo_list_ui_get_node get_next,
	cpymo_list_ui_get_node get_prev,
	bool from_bottom_to_top,
	size_t nodes_per_screen)
{
	cpymo_list_ui *data = NULL;
	error_t err = cpymo_ui_enter(
		(void **)&data, e, sizeof(cpymo_list_ui) + ui_data_size,
		&cpymo_list_ui_update,
		&cpymo_list_ui_draw,
		&cpymo_list_ui_delete);
	CPYMO_THROW(err);

	data->ui_data_deleter = ui_data_deleter;
	data->draw_node = draw_node;
	data->current_node = current;
	data->get_next = get_next;
	data->get_prev = get_prev;
	data->from_bottom_to_top = from_bottom_to_top;
	data->node_height = (float)e->gameconfig.imagesize_h / nodes_per_screen;
	data->current_y = 0;
	data->selection_relative_to_cur = 0;
	data->ok = ok;
	data->mouse_key_press_time = 0;
	data->allow_scroll = true;
	data->custom_update = NULL;
	data->scroll_delta_y_sum = 0;
	data->scroll_speed = 0;
	data->no_more_content_callback = NULL;
	data->selection_changed = NULL;
	data->allow_exit_list_ui = true;
	data->nodes_per_screen = nodes_per_screen;
	data->mouse_touch_move_y_sum = 0;
	data->just_hold_callback = NULL;

	cpymo_key_pluse_init(&data->key_up, e->input.up);
	cpymo_key_pluse_init(&data->key_down, e->input.down);
	cpymo_key_hold_init(&data->key_mouse_button, e->input.mouse_button);

	assert(*out_ui_data == NULL);
	*out_ui_data = cpymo_list_ui_data(e);

	return err;
}

static error_t cpymo_list_ui_selecting_no_more_content_callback_looping(cpymo_engine *e, bool is_down)
{
	cpymo_list_ui *ui = (cpymo_list_ui *)cpymo_ui_data(e);

	if (is_down) {
		ui->selection_relative_to_cur = 0;
		while (true) {
			void *next = ui->get_prev(e, ui + 1, ui->current_node);
			if (next == NULL) break;
			ui->current_node = next;
		}
		
	}
	else {
		ui->selection_relative_to_cur = 0;
		while (true) {
			void *next = ui->get_next(e, ui + 1, ui->current_node);
			if (next == NULL) break;
			ui->current_node = next;
		}

		int nodes_per_screen = (size_t)((float)e->gameconfig.imagesize_h / ui->node_height);
		while (ui->selection_relative_to_cur < nodes_per_screen - 1) {
			void *prev = ui->get_prev(e, ui + 1, ui->current_node);
			if (prev == NULL) break;
			ui->current_node = prev;
			ui->selection_relative_to_cur++;
		}
	}

	ui->current_y = 0;

	cpymo_engine_request_redraw(e);

	return CPYMO_ERR_SUCC;
}

void cpymo_list_ui_exit(cpymo_engine * e)
{
	cpymo_list_ui *ui = (cpymo_list_ui *)cpymo_ui_data(e);
	if (ui->allow_exit_list_ui)
		cpymo_ui_exit(e);
}

void cpymo_list_ui_enable_loop(cpymo_engine *e)
{
	cpymo_list_ui_set_selecting_no_more_content_callback(e, 
		&cpymo_list_ui_selecting_no_more_content_callback_looping);
}

void cpymo_list_ui_set_selection_changed_callback(struct cpymo_engine *e, cpymo_list_ui_selection_changed c)
{ ((cpymo_list_ui *)cpymo_ui_data(e))->selection_changed = c; }

void cpymo_list_ui_set_scroll_enabled(struct cpymo_engine *e, bool allow_scroll)
{ ((cpymo_list_ui *)cpymo_ui_data(e))->allow_scroll = allow_scroll; }

void cpymo_list_ui_set_custom_update(struct cpymo_engine *e, cpymo_list_ui_custom_update update)
{ ((cpymo_list_ui *)cpymo_ui_data(e))->custom_update = update; }

void *cpymo_list_ui_data(struct cpymo_engine *e)
{ return ((uint8_t *)cpymo_ui_data(e)) + sizeof(cpymo_list_ui); }

void cpymo_list_ui_set_selecting_no_more_content_callback(struct cpymo_engine *e, cpymo_list_ui_selecting_no_more_content_callback c)
{ ((cpymo_list_ui *)cpymo_ui_data(e))->no_more_content_callback = c; }

void cpymo_list_ui_set_mouse_button_just_hold_callback(struct cpymo_engine *e, cpymo_list_ui_mouse_button_just_hold_callback c)
{ ((cpymo_list_ui *)cpymo_ui_data(e))->just_hold_callback = c; }

const void *cpymo_list_ui_data_const(const struct cpymo_engine *e)
{ return ((uint8_t *)cpymo_ui_data_const(e)) + sizeof(cpymo_list_ui); }

void cpymo_list_ui_set_current_node(struct cpymo_engine *e, void *cur)
{
	cpymo_list_ui *ui = (cpymo_list_ui *)cpymo_ui_data(e);
	ui->current_node = cur;
}

void cpymo_list_ui_set_allow_exit(struct cpymo_engine *e, bool b)
{
	cpymo_list_ui *ui = (cpymo_list_ui *)cpymo_ui_data(e);
	ui->allow_exit_list_ui = b;
}

void cpymo_list_ui_set_selection_relative_to_cur(struct cpymo_engine *e, int c)
{
	cpymo_list_ui *ui = (cpymo_list_ui *)cpymo_ui_data(e);
	ui->selection_relative_to_cur = c;
}