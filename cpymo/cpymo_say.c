#include "cpymo_prelude.h"
#include "cpymo_say.h"
#include "cpymo_rmenu.h"
#include "cpymo_engine.h"
#include "cpymo_save.h"
#include <stdlib.h>
#include <assert.h>

#define ALREADY_READ_TEXT_ALPHA 0.6f

#define DISABLE_TEXTBOX(SAY) \
	if (SAY->textbox_usable) { \
		cpymo_textbox_free(&(SAY)->textbox, &e->backlog); \
		SAY->textbox_usable = false; \
	}

#define ENABLE_TEXTBOX(SAY, E, X, Y, W, H, FONT_SIZE, COL, ALPHA, TEXT, ERR) \
	{ \
		DISABLE_TEXTBOX(SAY); \
		ERR = cpymo_textbox_init( \
			&(SAY)->textbox, X, Y, W, H, FONT_SIZE, \
			COL, ALPHA, TEXT, &(E)->backlog); \
		if (ERR == CPYMO_ERR_SUCC) SAY->textbox_usable = true; \
	}

#define RESET_NAME(SAY) SAY->name = NULL;

static void cpymo_say_lazy_init(cpymo_say *out, cpymo_assetloader *loader)
{
	if (out->lazy_init == false) {
		out->lazy_init = true;
		error_t err = cpymo_assetloader_load_system_image(
			&out->msg_cursor,
			&out->msg_cursor_w,
			&out->msg_cursor_h,
			cpymo_str_pure("message_cursor"),
			loader,
			true);

		if (err != CPYMO_ERR_SUCC) out->msg_cursor = NULL;
	}
}

void cpymo_say_init(cpymo_say *out)
{
	out->msgbox = NULL;
	out->namebox = NULL;
	out->msg_cursor = NULL;
	out->active = false;
	out->lazy_init = false;
	out->textbox_usable = false;
	out->name = NULL;
	out->hide_window = false;
	out->msgbox_name = NULL;
	out->namebox_name = NULL;

	out->current_name = NULL;
	out->current_text = NULL;
	out->current_say_is_already_read = true;
}

void cpymo_say_free(cpymo_say *say)
{
	cpymo_textbox_free(&say->textbox, NULL);
	RESET_NAME(say);
	if (say->msgbox) cpymo_backend_image_free(say->msgbox);
	if (say->namebox) cpymo_backend_image_free(say->namebox);
	if (say->msg_cursor) cpymo_backend_image_free(say->msg_cursor);
	if (say->msgbox_name) free(say->msgbox_name);
	if (say->namebox_name) free(say->namebox_name);

	if (say->current_name) free(say->current_name);
	if (say->current_text) free(say->current_text);
}

void cpymo_say_draw(const struct cpymo_engine *e)
{
	if (e->say.active && !e->input.hide_window && !e->say.hide_window) {
		float ratio = (float)e->gameconfig.imagesize_w / (float)e->say.msgbox_w;
		float msg_h = (float)e->say.msgbox_h * ratio;
		float y = (float)e->gameconfig.imagesize_h - msg_h;
		float offx = (float)e->gameconfig.nameboxorg_x / 540.0f * (float)e->gameconfig.imagesize_w;
		float offy = (float)e->gameconfig.nameboxorg_y / 360.0f * (float)e->gameconfig.imagesize_h;

		float fontsize = cpymo_gameconfig_font_size(&e->gameconfig);
		float namebox_h = fontsize * 1.4f;
		float namebox_w = (float)e->say.namebox_w / ((float)e->say.namebox_h / namebox_h);

		float namebox_x;
		switch (e->gameconfig.namealign) {
		case 0:
			namebox_x = (float)(e->gameconfig.imagesize_w - namebox_w) / 2;
			break;
		case 2:
			namebox_x = (float)(e->gameconfig.imagesize_w - namebox_w);
			break;
		default:
			namebox_x = 0;
			break;
		};

		float namebox_y = y - namebox_h;

		namebox_x += offx;
		namebox_y -= offy;

		const cpymo_color gray = { 127, 127, 127 };

		if (e->say.msgbox) {
#ifdef DISABLE_IMAGE_SCALING
			const float screen_w = e->gameconfig.imagesize_w;
			const float screen_h = e->gameconfig.imagesize_h;
			cpymo_backend_image_draw(
				screen_w / 2 - (float)e->say.msgbox_w / 2,
				screen_h - (float)e->say.msgbox_h,
				e->say.msgbox_w, e->say.msgbox_h,
				e->say.msgbox,
				0, 0, e->say.msgbox_w, e->say.msgbox_h, 1.0f,
				cpymo_backend_image_draw_type_text_say_textbox);
#else
			cpymo_backend_image_draw(
				0, y, (float)e->gameconfig.imagesize_w, msg_h,
				e->say.msgbox, 0, 0, e->say.msgbox_w, e->say.msgbox_h,
				1.0f, cpymo_backend_image_draw_type_text_say_textbox);
#endif
		}
		else {
			float xywh[] = {
				0,
				e->gameconfig.imagesize_h * 0.75f,
				(float)e->gameconfig.imagesize_w,
				e->gameconfig.imagesize_h * 0.25f
			};

			cpymo_backend_image_fill_rects(xywh, 1, gray, 0.5f, cpymo_backend_image_draw_type_text_say_textbox);
		}

		if (e->say.name) {
			float name_x =
				namebox_w / 2 - (float)e->say.name_width / 2 + namebox_x;

			if (e->say.namebox) {
#ifdef DISABLE_IMAGE_SCALING
				cpymo_backend_image_draw(
					namebox_x + (namebox_w - e->say.namebox_w) / 2,
					namebox_y + (namebox_h - e->say.namebox_h) / 2,
					e->say.namebox_w, 
					e->say.namebox_h,
					e->say.namebox, 0, 0, e->say.namebox_w, e->say.namebox_h, 1.0f,
					cpymo_backend_image_draw_type_text_say_textbox);
#else
				cpymo_backend_image_draw(
					namebox_x, namebox_y, namebox_w, namebox_h,
					e->say.namebox, 0, 0, e->say.namebox_w, e->say.namebox_h, 1.0f,
					cpymo_backend_image_draw_type_text_say_textbox);
#endif
			}
			else {
				float xywh[] = { namebox_x, namebox_y, namebox_w, namebox_h };
				cpymo_backend_image_fill_rects(xywh, 1, gray, 0.5f, cpymo_backend_image_draw_type_text_say_textbox);
			}

			cpymo_backend_text_draw(
				e->say.name,
				name_x, namebox_y + cpymo_gameconfig_font_size(&e->gameconfig),
				e->gameconfig.textcolor,
				e->say.current_say_is_already_read ? 
					ALREADY_READ_TEXT_ALPHA : 1, 
				cpymo_backend_image_draw_type_text_say);
		}

		if (e->say.textbox_usable) {
			cpymo_textbox_draw(e, &e->say.textbox, cpymo_backend_image_draw_type_text_say);
		}
	}
}

static inline error_t cpymo_say_load_msgbox_image(cpymo_say *say, cpymo_str name, cpymo_assetloader *l)
{
	if (say->msgbox) cpymo_backend_image_free(say->msgbox);
	say->msgbox = NULL;

	char *msgbox_name = (char *)realloc(say->msgbox_name, name.len + 1);
	if (msgbox_name) {
		cpymo_str_copy(msgbox_name, name.len + 1, name);
		say->msgbox_name = msgbox_name;
	}

	error_t err = cpymo_assetloader_load_system_image(
		&say->msgbox,
		&say->msgbox_w,
		&say->msgbox_h,
		name,
		l,
		true);

	if (err != CPYMO_ERR_SUCC) say->msgbox = NULL;

	return err;
}

static inline error_t cpymo_say_load_namebox_image(cpymo_say *say, cpymo_str name, cpymo_assetloader *l)
{
	if (say->namebox) cpymo_backend_image_free(say->namebox);
	say->namebox = NULL;

	char *namebox_name = (char *)realloc(say->namebox_name, name.len + 1);
	if (namebox_name) {
		cpymo_str_copy(namebox_name, name.len + 1, name);
		say->namebox_name = namebox_name;
	}

	error_t err = cpymo_assetloader_load_system_image(
		&say->namebox,
		&say->namebox_w,
		&say->namebox_h,
		name,
		l,
		true);

	if (err != CPYMO_ERR_SUCC) say->namebox = NULL;

	return err;
}

error_t cpymo_say_load_msgbox_and_namebox_image(cpymo_say *say, cpymo_str msgbox, cpymo_str namebox, cpymo_assetloader *l)
{
	cpymo_say_lazy_init(say, l);
	error_t err = cpymo_say_load_namebox_image(say, namebox, l);
	if (err != CPYMO_ERR_SUCC) {
		say->namebox = NULL;
		say->namebox_w = (int)(l->game_config->imagesize_w * 0.25f);
		say->namebox_h = (int)(cpymo_gameconfig_font_size(l->game_config) * 1.5f);

		say->msgbox = NULL;
		say->msgbox_w = (int)l->game_config->imagesize_w;
		say->msgbox_h = (int)(l->game_config->imagesize_h * 0.25f);

		printf("[Warning] Can not load namebox image.\n");

		return err;
	}

	err = cpymo_say_load_msgbox_image(say, msgbox, l);
	if (err != CPYMO_ERR_SUCC) {
		say->msgbox = NULL;
		say->msgbox_w = l->game_config->imagesize_w;
		say->msgbox_h = (int)(l->game_config->imagesize_h * 0.25f);

		printf("[Warning] Can not load msgbox image.\n");

		return err;
	}

	return CPYMO_ERR_SUCC;
}

/** Mermaid Flow Chart

graph TB
	A(START) --> cpymo_say_wait_text_fadein
	cpymo_say_wait_text_fadein --> cpymo_say_wait_text_fadein_callback
	cpymo_say_wait_text_fadein_callback --> cpymo_say_wait_text_reading
	cpymo_say_wait_text_reading --> cpymo_say_wait_text_read_callback
	cpymo_say_wait_text_read_callback --> cpymo_say_wait_text_fadein
	cpymo_say_wait_text_read_callback --> E(END)

 **/

static error_t cpymo_say_wait_text_fadein_callback(cpymo_engine *e);

static bool cpymo_say_wait_text_reading(cpymo_engine *e, float dt)
{
	assert(e->say.textbox_usable);

	const enum cpymo_key_hold_result mouse_button_state =
		cpymo_key_hold_update(e, &e->say.key_mouse_button, dt, e->input.mouse_button);

	if (e->say.hide_window) {
		if (cpymo_input_foward_key_just_released(e)) {
			e->say.hide_window = false;
			cpymo_engine_request_redraw(e);
		}
		return false;
	}

	bool open_backlog_by_slide = false;
	if (e->prev_input.mouse_button && e->input.mouse_button) {
		if (e->prev_input.mouse_position_useable && 
			e->input.mouse_position_useable) {
			
			if (e->input.mouse_y - e->prev_input.mouse_y > 10) {
				open_backlog_by_slide = true;
			}
		}
	}

	if (CPYMO_INPUT_JUST_PRESSED(e, up) || 
		e->input.mouse_wheel_delta > 0 ||
		open_backlog_by_slide)
		cpymo_backlog_ui_enter(e);
	else if (
		CPYMO_INPUT_JUST_RELEASED(e, cancel) || 
		(mouse_button_state == cpymo_key_hold_result_just_hold && 
		!e->ignore_next_mouse_button_flag)) {
		cpymo_rmenu_enter(e);
		return false;
	}

	return cpymo_textbox_wait_text_reading(e, dt, &e->say.textbox) || e->skipping;
}

static bool cpymo_say_wait_text_fadein(cpymo_engine *e, float dt)
{
	assert(e->say.textbox_usable);

	if (e->say.hide_window) {
		if (cpymo_input_foward_key_just_released(e)) {
			e->say.hide_window = false;
			cpymo_engine_request_redraw(e);
		}
		return false;
	}

	return cpymo_textbox_wait_text_fadein(e, dt, &e->say.textbox);
}

static error_t cpymo_say_wait_text_read_callback(cpymo_engine *e)
{
	cpymo_say *say = &e->say;

	assert(say->textbox_usable);

	if (!cpymo_textbox_all_finished(&say->textbox)) {
		error_t err = cpymo_textbox_clear_page(&say->textbox, &e->backlog);
		CPYMO_THROW(err);

		cpymo_wait_register_with_callback(
			&e->wait,
			&cpymo_say_wait_text_fadein,
			&cpymo_say_wait_text_fadein_callback);
		cpymo_engine_request_redraw(e);
	}
	else {
		DISABLE_TEXTBOX(say);
		say->active = false;
	}

	return CPYMO_ERR_SUCC;
}

static error_t cpymo_say_autosave_and_next(cpymo_engine *e)
{
	cpymo_save_autosave(e);
	cpymo_wait_register_with_callback(
		&e->wait,
		&cpymo_say_wait_text_reading,
		&cpymo_say_wait_text_read_callback);
	return CPYMO_ERR_SUCC;
}

static error_t cpymo_say_wait_text_fadein_callback(cpymo_engine *e)
{
	cpymo_key_hold_init(&e->say.key_mouse_button, e->input.mouse_button);
	if (cpymo_engine_skipping(e)) {
		cpymo_wait_register_with_callback(
		&e->wait,
		&cpymo_say_wait_text_reading,
		&cpymo_say_wait_text_read_callback);
	}
	else {
		cpymo_wait_callback_nextframe(&e->wait, cpymo_say_autosave_and_next);
	}
	return CPYMO_ERR_SUCC;
}

error_t cpymo_say_start(cpymo_engine *e, cpymo_str name, cpymo_str text)
{
	cpymo_say_lazy_init(&e->say, &e->assetloader);

	uint64_t hash;
	cpymo_str_hash_init(&hash);
	cpymo_str_hash_append_cstr(&hash, "say:");
	cpymo_str_hash_append_cstr(&hash, e->interpreter->script->script_name);
	char buf[32];
	sprintf(buf, "%u:%u:", 
		(unsigned)e->interpreter->script_parser.cur_pos,
		(unsigned)e->interpreter->script_parser.cur_line);
	cpymo_str_hash_append_cstr(&hash, buf);
	cpymo_str_hash_append(&hash, name);
	cpymo_str_hash_append(&hash, text);

	e->say.current_say_is_already_read = 
		cpymo_hash_flags_check(&e->flags, hash);
	if (!e->say.current_say_is_already_read)
		cpymo_hash_flags_add(&e->flags, hash);

	if (name.len > 0) {
		char *current_name = (char *)realloc(e->say.current_name, name.len + 1);
		if (current_name) {
			cpymo_str_copy(current_name, name.len + 1, name);
			e->say.current_name = current_name;
		}
	}
	else {
		free(e->say.current_name);
		e->say.current_name = NULL;
	}

	char *current_text = (char *)realloc(e->say.current_text, text.len + 1);
	if (current_text) {
		cpymo_str_copy(current_text, text.len + 1, text);
		e->say.current_text = current_text;
	}

	float fontsize = cpymo_gameconfig_font_size(&e->gameconfig);

	// Create name box
	cpymo_say *say = &e->say;
	RESET_NAME(say);

	cpymo_str_trim(&name);
	if (name.len > 0) {
		error_t err = cpymo_backend_text_create(&say->name, &say->name_width, name, fontsize);
		if (err != CPYMO_ERR_SUCC) say->name = NULL;
	}

	cpymo_backlog_record_write_name(&e->backlog, say->name, name);

	// Create say message text
	float msglr_l = (float)e->gameconfig.msglr_l * e->gameconfig.imagesize_w / 540.0f;
	float msglr_r = (float)e->gameconfig.msglr_r * e->gameconfig.imagesize_w / 540.0f;
	float msgtb_t = (float)e->gameconfig.msgtb_t * e->gameconfig.imagesize_h / 360.0f;
	float msgtb_b = (float)e->gameconfig.msgtb_b * e->gameconfig.imagesize_h / 360.0f;

#ifndef DISABLE_IMAGE_SCALING
	float ratio = (float)e->gameconfig.imagesize_w / (float)e->say.msgbox_w;
	float msg_h = (float)e->say.msgbox_h * ratio;
	float x = msglr_l;
	float w = (float)e->gameconfig.imagesize_w - msglr_l - msglr_r;
#else
	float msg_h = (float)e->say.msgbox_h;
	float x = (e->gameconfig.imagesize_w - e->say.msgbox_w) / 2 + msglr_l;
	float w = e->say.msgbox_w - msglr_l - msglr_r;
#endif

	float y = (float)e->gameconfig.imagesize_h - msg_h + msgtb_t;
	float h = msg_h - msgtb_t - msgtb_b;

	const float text_alpha = 
		e->say.current_say_is_already_read ? 
			ALREADY_READ_TEXT_ALPHA : 1.0f;

	error_t err;
	ENABLE_TEXTBOX(
		say, e, x, y, w, h, fontsize, e->gameconfig.textcolor, text_alpha, text, err);

	cpymo_engine_request_redraw(e);

	e->say.active = true;

	if (cpymo_engine_skipping(e)) {
		cpymo_say_wait_text_read_callback(e);
		cpymo_engine_request_redraw(e);
	}
	else {
		cpymo_wait_register_with_callback(
			&e->wait,
			&cpymo_say_wait_text_fadein,
			&cpymo_say_wait_text_fadein_callback);
	}

	return err;
}

void cpymo_say_hidewindow_until_click(cpymo_engine * e)
{
	e->say.hide_window = true;
	cpymo_engine_request_redraw(e);
}

