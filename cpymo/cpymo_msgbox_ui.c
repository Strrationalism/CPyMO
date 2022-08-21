#include "cpymo_prelude.h"
#include "cpymo_msgbox_ui.h"
#include "cpymo_engine.h"
#include "cpymo_localization.h"
#include "cpymo_key_hold.h"
#include <stdlib.h>
#include <string.h>

#ifdef __3DS__
extern bool fill_screen_enabled;
const extern bool fill_screen;
const extern bool enhanced_3ds_display_mode;
#endif

typedef struct {
	cpymo_backend_text message;
	float message_width;

	cpymo_backend_text confirm_btn;
	float confirm_btn_width;
	cpymo_backend_text cancel_btn;
	float cancel_btn_width;

	int selection;

	error_t(*confirm)(cpymo_engine *e, void *data);
	void *confirm_data;
	cpymo_key_hold mouse_button;
} cpymo_msgbox_ui;

static error_t cpymo_msgbox_ui_confirm(cpymo_engine *e)
{
	cpymo_msgbox_ui *ui = (cpymo_msgbox_ui *)cpymo_ui_data(e);
	error_t(*func)(cpymo_engine *, void *) = ui->confirm;
	void *data = ui->confirm_data;
	cpymo_ui_exit(e);
	return func(e, data);
}

static void cpymo_msgbox_ui_get_btn_rect(
	float *out_xywh,
	float txt_width,
	float font_size,
	float screen_w,
	float y)
{
	float btn_width = txt_width * 3.0f;
	float btn_height = font_size * 1.5f;

	out_xywh[0] = screen_w / 2 - btn_width / 2;
	out_xywh[1] = y;
	out_xywh[2] = btn_width;
	out_xywh[3] = btn_height;
}

static int cpymo_msgbox_ui_get_mouse_selection(cpymo_engine *e)
{
	cpymo_msgbox_ui *ui = (cpymo_msgbox_ui *)cpymo_ui_data(e);
	float fontsize = cpymo_gameconfig_font_size(&e->gameconfig);

	float screen_w = (float)e->gameconfig.imagesize_w, screen_h = (float)e->gameconfig.imagesize_h;

	const cpymo_input *input = &e->input;
	if (!input->mouse_position_useable)
		input = &e->prev_input;

	float btn_width =
		ui->confirm_btn_width > ui->cancel_btn_width ?
		ui->confirm_btn_width : ui->cancel_btn_width;

	if (input->mouse_position_useable) {
		float xywh[4];
		cpymo_msgbox_ui_get_btn_rect(
			xywh,
			btn_width,
			fontsize,
			screen_w,
			ui->cancel_btn ? screen_h * 0.5f : screen_h * 0.618f);

		float x = input->mouse_x;
		float y = input->mouse_y;
		if (x >= xywh[0] && x <= xywh[0] + xywh[2] && y >= xywh[1] && y <= xywh[1] + xywh[3])
			return 0;

		if (ui->cancel_btn) {
			cpymo_msgbox_ui_get_btn_rect(
				xywh,
				btn_width,
				fontsize,
				screen_w,
				screen_h * 0.5f + fontsize * 1.75f + (8.0f / (fontsize / 32)));

			if (x >= xywh[0] && x <= xywh[0] + xywh[2] && y >= xywh[1] && y <= xywh[1] + xywh[3])
				return 1;
		}
	}

	return -1;
}

static error_t cpymo_msgbox_ui_update(cpymo_engine *e, void *ui_data, float dt)
{
	cpymo_msgbox_ui *ui = (cpymo_msgbox_ui *)ui_data;

	enum cpymo_key_hold_result mbs = cpymo_key_hold_update(
		&ui->mouse_button, dt, e->input.mouse_button);
	
	if (CPYMO_INPUT_JUST_RELEASED(e, cancel) || mbs == cpymo_key_hold_result_holding) {
		cpymo_ui_exit(e);
		return CPYMO_ERR_SUCC;
	}

#ifdef ENABLE_TEXT_EXTRACT
	const cpymo_localization *l = cpymo_localization_get(e);
#endif

	if ((CPYMO_INPUT_JUST_PRESSED(e, up) || CPYMO_INPUT_JUST_PRESSED(e, down))) {
		if (ui->selection == -1) {
			if (ui->cancel_btn) ui->selection = 1;
			else ui->selection = 0;
		}
		else if (ui->selection == 0 && ui->cancel_btn) ui->selection = 1;
		else if (ui->selection == 1 && ui->cancel_btn) ui->selection = 0;

		cpymo_engine_request_redraw(e);

#ifdef ENABLE_TEXT_EXTRACT
		cpymo_backend_text_extract(ui->selection == 0 ? l->msgbox_ok : l->msgbox_cancel);
#endif
	}
	else if (CPYMO_INPUT_JUST_PRESSED(e, ok)) {
		if (ui->cancel_btn == NULL) {
			ui->selection = 0;
		}

		cpymo_engine_request_redraw(e);
	}
	else if (CPYMO_INPUT_JUST_PRESSED(e, mouse_button)) {
		ui->selection = cpymo_msgbox_ui_get_mouse_selection(e);
		cpymo_engine_request_redraw(e);
	}
	else if (CPYMO_INPUT_JUST_RELEASED(e, ok)) {
		cpymo_engine_request_redraw(e);
		if (ui->selection == 0) {
			return cpymo_msgbox_ui_confirm(e);
		}
		else if (ui->selection == 1) {
			cpymo_ui_exit(e);
			return CPYMO_ERR_SUCC;
		}
	}
	else if (CPYMO_INPUT_JUST_RELEASED(e, mouse_button)) {
		cpymo_engine_request_redraw(e);
		int mouse_sel = cpymo_msgbox_ui_get_mouse_selection(e);
		if (mouse_sel == 0) return cpymo_msgbox_ui_confirm(e);
		else if (mouse_sel == 1) {
			cpymo_ui_exit(e);
			return CPYMO_ERR_SUCC;
		}
	}

	if (cpymo_input_mouse_moved(e)) {
		int mouse_sel = cpymo_msgbox_ui_get_mouse_selection(e);
		if (mouse_sel != ui->selection) {
			ui->selection = mouse_sel;
			cpymo_engine_request_redraw(e);

#ifdef ENABLE_TEXT_EXTRACT
			if (ui->selection >= 0)
				cpymo_backend_text_extract(ui->selection == 0 ? l->msgbox_ok : l->msgbox_cancel);
#endif
		}
	}

	return CPYMO_ERR_SUCC;
}

static void cpymo_msgbox_ui_draw_btn(
	cpymo_backend_text btn_txt,
	float btn_width,
	float txt_width, 
	float font_size, 
	float screen_w,
	float screen_h,
	float y, 
	bool sel,
	bool key_down)
{
	float xywh[4];
	cpymo_msgbox_ui_get_btn_rect(xywh, btn_width, font_size, screen_w, y);

	cpymo_color gray;
	gray.r = 128;
	gray.g = 128;
	gray.b = 128;

	cpymo_backend_image_fill_rects(
		xywh,
		1,
		sel ? (key_down ? gray : cpymo_color_white) : cpymo_color_black,
		sel ? 1.0f : 0.5f,
		cpymo_backend_image_draw_type_ui_element);

	cpymo_backend_text_draw(
		btn_txt,
		screen_w / 2 - txt_width / 2,
		y + xywh[3] / 2 + font_size / 2 - font_size * 0.15f,
		sel ? cpymo_color_black : cpymo_color_white,
		1.0f,
		cpymo_backend_image_draw_type_ui_element);
}

static void cpymo_msgbox_ui_draw(const cpymo_engine *e, const void *ui_data)
{
	cpymo_bg_draw(e);
	cpymo_scroll_draw(&e->scroll);

	#ifdef __3DS__
	if (fill_screen && !enhanced_3ds_display_mode)
		fill_screen_enabled = false;
	#endif

	float screen_w = (float)e->gameconfig.imagesize_w, screen_h = (float)e->gameconfig.imagesize_h;
	float xywh[] = { -100, -100, screen_w + 200, screen_h + 200 };
	cpymo_backend_image_fill_rects(xywh, 1, cpymo_color_black, 0.5f, cpymo_backend_image_draw_type_bg);

	const cpymo_msgbox_ui *ui = (const cpymo_msgbox_ui *)ui_data;
	float fontsize = cpymo_gameconfig_font_size(&e->gameconfig);

	float x = (screen_w - ui->message_width) / 2;
	float y = screen_h * 0.3f + fontsize / 2;
	cpymo_backend_text_draw(ui->message, x, y, cpymo_color_white, 1.0f, cpymo_backend_image_draw_type_ui_element);

	bool key_down = e->input.mouse_button || e->input.ok;

	float btn_width = ui->confirm_btn_width > ui->cancel_btn_width ? ui->confirm_btn_width : ui->cancel_btn_width;

	cpymo_msgbox_ui_draw_btn(
		ui->confirm_btn, 
		btn_width,
		ui->confirm_btn_width, 
		fontsize, 
		screen_w, 
		screen_h,
		ui->cancel_btn ? screen_h * 0.5f : screen_h * 0.618f,
		ui->selection == 0,
		key_down);

	if (ui->cancel_btn) {
		cpymo_msgbox_ui_draw_btn(
			ui->cancel_btn,
			btn_width,
			ui->cancel_btn_width,
			fontsize,
			screen_w,
			screen_h,
			screen_h * 0.5f + fontsize * 1.75f + (8.0f / (fontsize / 32)),
			ui->selection == 1,
			key_down);
	}

	#ifdef __3DS__
	fill_screen_enabled = true;
	#endif
}

static void cpymo_msgbox_ui_delete(struct cpymo_engine *e, void *ui_data)
{
	cpymo_msgbox_ui *ui = (cpymo_msgbox_ui *)ui_data;
	
	if (ui->message) cpymo_backend_text_free(ui->message);
	if (ui->confirm_btn) cpymo_backend_text_free(ui->confirm_btn);
	if (ui->cancel_btn) cpymo_backend_text_free(ui->cancel_btn);
}

static error_t cpymo_msgbox_ui_default_confirm(struct cpymo_engine *e, void *data)
{
	return CPYMO_ERR_SUCC;
}

error_t cpymo_msgbox_ui_enter(
	cpymo_engine *e, 
	cpymo_parser_stream_span message, 
	error_t(*confirm)(cpymo_engine *e, void *data), 
	void * confirm_data)
{
	cpymo_msgbox_ui *ui = NULL;
	error_t err = cpymo_ui_enter(
		(void **)&ui,
		e,
		sizeof(cpymo_msgbox_ui),
		&cpymo_msgbox_ui_update,
		&cpymo_msgbox_ui_draw,
		&cpymo_msgbox_ui_delete);
	CPYMO_THROW(err);

	ui->message = NULL;
	ui->confirm_btn = NULL;
	ui->cancel_btn = NULL;
	ui->cancel_btn_width = 0;
	ui->confirm_btn_width = 0;

	ui->confirm = confirm;
	ui->confirm_data = confirm_data;
	ui->selection = confirm ? -1 : 0;
	cpymo_key_hold_init(&ui->mouse_button, e->input.mouse_button);

	float fontsize = cpymo_gameconfig_font_size(&e->gameconfig);

	err = cpymo_backend_text_create(&ui->message, &ui->message_width, message, fontsize);
	if (err != CPYMO_ERR_SUCC) {
		cpymo_ui_exit(e);
		return err;
	}

	const cpymo_localization *l = cpymo_localization_get(e);

	err = cpymo_backend_text_create(
		&ui->confirm_btn, 
		&ui->confirm_btn_width, 
		cpymo_parser_stream_span_pure(l->msgbox_ok), 
		fontsize);
	if (err != CPYMO_ERR_SUCC) {
		cpymo_ui_exit(e);
		return err;
	}

	if (confirm) {
		err = cpymo_backend_text_create(
			&ui->cancel_btn,
			&ui->cancel_btn_width,
			cpymo_parser_stream_span_pure(l->msgbox_cancel),
			fontsize);
		if (err != CPYMO_ERR_SUCC) {
			cpymo_ui_exit(e);
			return err;
		}
	}
	else {
		ui->confirm = &cpymo_msgbox_ui_default_confirm;
	}

#ifdef ENABLE_TEXT_EXTRACT
	char *msg = (char *)malloc(message.len + strlen(l->msgbox_cancel) + 1);
	if (msg) {
		cpymo_parser_stream_span_copy(msg, message.len + strlen(l->msgbox_cancel) + 1, message);
		if (confirm) {
			strcat(msg, l->msgbox_cancel);
		}

		cpymo_backend_text_extract(msg);
		free(msg);
	}
#endif

	return err;
}

