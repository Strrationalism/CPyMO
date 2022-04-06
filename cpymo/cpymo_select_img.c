#include "cpymo_select_img.h"
#include "cpymo_localization.h"
#include "cpymo_engine.h"
#include "cpymo_rmenu.h"
#include <memory.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

static error_t cpymo_select_img_ok_callback_default(cpymo_engine *e, int sel, uint64_t hash, bool save_enabled)
{
	if (save_enabled)
		cpymo_hash_flags_add(&e->flags, hash);

	error_t err = cpymo_vars_set(
		&e->vars,
		cpymo_parser_stream_span_pure("FSEL"),
		sel);

	if (err != CPYMO_ERR_SUCC) return err;
	return CPYMO_ERR_SUCC;
}

void cpymo_select_img_reset(cpymo_select_img *img)
{
	if (img->select_img_image) {
		// select_img
		cpymo_backend_image_free(img->select_img_image);
	}
	else if(img->selections) {
		// select_imgs
		for (size_t i = 0; i < img->all_selections; ++i) {
			if (img->selections[i].image)
				cpymo_backend_image_free(img->selections[i].image); 
			if (img->selections[i].or_text)
				cpymo_backend_text_free(img->selections[i].or_text);
#ifndef NON_VISUALLY_IMPAIRED_HELP
			if (img->selections[i].original_text)
				free(img->selections[i].original_text);
#endif
		}
	}

	if (img->selections)
		free(img->selections);

	for (size_t i = 0; i < 4; ++i) {
		if (img->hint[i]) cpymo_backend_image_free(img->hint[i]);
		img->hint[i] = NULL;
	}

	img->selections = NULL;
	img->select_img_image = NULL;
	img->show_option_background = false;
}

error_t cpymo_select_img_configuare_begin(
	cpymo_select_img *sel, size_t selections,
	cpymo_parser_stream_span image_name_or_empty_when_select_imgs,
	cpymo_assetloader *loader, cpymo_gameconfig *gameconfig)
{
	sel->selections =
		(cpymo_select_img_selection *)malloc(sizeof(cpymo_select_img_selection) * selections);
	if (sel->selections == NULL) return CPYMO_ERR_SUCC;

	memset(sel->selections, 0, sizeof(cpymo_select_img_selection) * selections);

	if (image_name_or_empty_when_select_imgs.len > 0) {
		error_t err = cpymo_assetloader_load_system_image(
			&sel->select_img_image,
			&sel->select_img_image_w, &sel->select_img_image_h,
			image_name_or_empty_when_select_imgs,
			loader,
			true);

		if (err != CPYMO_ERR_SUCC) {
			free(sel->selections);
			sel->selections = NULL;
			return err;
		}
	}

	sel->current_selection = 0;
	sel->all_selections = selections;
	sel->ok_callback = &cpymo_select_img_ok_callback_default;

	return CPYMO_ERR_SUCC;
}

void cpymo_select_img_configuare_select_img_selection(cpymo_engine *e, float x, float y, bool enabled, uint64_t hash)
{
	assert(e->select_img.selections);
	assert(e->select_img.all_selections);
	assert(e->select_img.select_img_image);

	cpymo_select_img_selection *sel = &e->select_img.selections[e->select_img.current_selection++];
	sel->image = e->select_img.select_img_image;
	sel->or_text = NULL;
#ifndef NON_VISUALLY_IMPAIRED_HELP
	sel->original_text = NULL;
#endif

	sel->x = x;
	sel->y = y;
	sel->w = e->select_img.select_img_image_w / 2;
	sel->h = e->select_img.select_img_image_h / (int)e->select_img.all_selections;

	sel->enabled = enabled;

	sel->hash = hash;
	sel->has_selected = cpymo_hash_flags_check(&e->flags, hash);
}

error_t cpymo_select_img_configuare_select_imgs_selection(cpymo_engine *e, cpymo_parser_stream_span image_name, float x, float y, bool enabled, uint64_t hash)
{
	assert(e->select_img.selections);
	assert(e->select_img.all_selections);
	assert(e->select_img.select_img_image == NULL);

	cpymo_select_img_selection *sel = &e->select_img.selections[e->select_img.current_selection++];

	error_t err = cpymo_assetloader_load_system_image(
		&sel->image,
		&sel->w,
		&sel->h,
		image_name,
		&e->assetloader,
		true);

	if (err != CPYMO_ERR_SUCC) {
		return err;
	}

	sel->or_text = NULL;
	sel->x = x;
	sel->y = y;
	sel->w /= 2;
	sel->enabled = enabled;

	sel->hash = hash;
	sel->has_selected = cpymo_hash_flags_check(&e->flags, hash);

#ifndef NON_VISUALLY_IMPAIRED_HELP
	sel->original_text = NULL;
#endif

	return CPYMO_ERR_SUCC;
}

static bool cpymo_select_img_wait(struct cpymo_engine *e, float dt)
{
	if (e->select_img.hint[0] != NULL) {
		e->select_img.hint_timer += dt;
		if (e->select_img.hint_timer >= 1.0f) {
			e->select_img.hint_timer -= 1.0f;
			e->select_img.hint_tiktok = !e->select_img.hint_tiktok;
			cpymo_engine_request_redraw(e);
		}
	}

	enum cpymo_key_hold_result mouse_button_state =
		cpymo_key_hold_update(&e->select_img.key_mouse_button, dt, e->input.mouse_button);

	if (e->select_img.save_enabled) {
		if (e->input.mouse_wheel_delta > 0)
			cpymo_backlog_ui_enter(e);
		else if (CPYMO_INPUT_JUST_RELEASED(e, cancel) || mouse_button_state == cpymo_key_hold_result_hold_released)
			cpymo_rmenu_enter(e);
	}

	return e->select_img.selections == NULL;
}

static bool cpymo_select_img_mouse_in_selection(cpymo_select_img *o, int sel, const cpymo_engine *e) {
	assert(sel >= 0 && sel < (int)o->all_selections);

	const cpymo_input *input = &e->input;
	if (!input->mouse_position_useable) {
		input = &e->prev_input;
		if(!input->mouse_position_useable)
			return false;
	}
	cpymo_select_img_selection *s = &o->selections[sel];
	
	float x = input->mouse_x;
	float y = input->mouse_y;

	if (s->image) {
		float left = s->x - (float)s->w / 2.0f;
		float top = s->y - (float)s->h / 2.0f;
		float right = s->x + (float)s->w / 2.0f;
		float bottom = s->y + (float)s->h / 2.0f;

		return x >= left && x <= right && y >= top && y <= bottom;
	}

	if (s->or_text) {
		float w = o->sel_highlight ? (float)o->sel_highlight_w : o->selections[sel].w;
		float left = (float)s->w / 2.0f - w / 2.0f + s->x;
		float right = left + w;
		float top = s->y - (float)s->h;
		float bottom = s->y;

		return x >= left && x <= right && y >= top && y <= bottom;
	}

	return false;
}


void cpymo_select_img_configuare_end(cpymo_select_img *sel, cpymo_wait *wait, struct cpymo_engine *e, int init_position)
{
	assert(sel->selections);
	assert(sel->all_selections);
	assert(sel->current_selection == sel->all_selections);

	sel->current_selection = init_position >= 0 ? init_position : 0;
	sel->save_enabled = init_position == -1;

	// In pymo, if all options are disabled, it will enable every option.
	bool all_is_disabled = true;
	for (size_t i = 0; i < sel->all_selections; ++i) {
		if (sel->selections[i].enabled) {
			all_is_disabled = false;
			break;
		}
	}

	if (all_is_disabled)
		for (size_t i = 0; i < sel->all_selections; ++i)
			sel->selections[i].enabled = true;

	if (sel->selections[sel->current_selection].enabled == false) {
		sel->current_selection = 0;
		while (sel->selections[sel->current_selection].enabled == false)
			sel->current_selection++;
	}

	// Trim disabled images.
	for (size_t i = 0; i < sel->all_selections; ++i) {
		if (!sel->selections[i].enabled) {
			if (sel->selections[i].image) {
				if (sel->select_img_image == NULL)
					cpymo_backend_image_free(sel->selections[i].image);
				sel->selections[i].image = NULL;
			}

			if (sel->selections[i].or_text) {
				cpymo_backend_text_free(sel->selections[i].or_text);
				sel->selections[i].or_text = NULL;
			}

#ifndef NON_VISUALLY_IMPAIRED_HELP
			if (sel->selections[i].original_text) {
				free(sel->selections[i].original_text);
				sel->selections[i].original_text = NULL;
			}
#endif
		}
	}

	for (int i = 0; i < (int)sel->all_selections; ++i) {
		if (cpymo_select_img_mouse_in_selection(sel, i, e)) {
			if (i != sel->current_selection) {
				sel->current_selection = i;
				break;
			}
		}
	}

	cpymo_key_hold_init(&sel->key_mouse_button, e->input.mouse_button);
	cpymo_wait_register(wait, &cpymo_select_img_wait);
	cpymo_engine_request_redraw(e);
}

static void cpymo_select_img_move(cpymo_select_img *o, int move) {
	assert(move == 1 || move == -1);
	
	o->current_selection += move;
	while (o->current_selection < 0) o->current_selection += (int)o->all_selections;
	while (o->current_selection >= (int)o->all_selections) o->current_selection -= (int)o->all_selections;

	if (o->selections[o->current_selection].image == NULL && o->selections[o->current_selection].or_text == NULL)
		cpymo_select_img_move(o, move);
}

static error_t cpymo_select_img_ok(cpymo_engine *e, int sel, uint64_t hash, cpymo_select_img *o)
{
	bool save_enabled = o->save_enabled;
	cpymo_select_img_reset(o);

	error_t err = o->ok_callback(e, sel, hash, save_enabled);
	cpymo_engine_request_redraw(e);
	return err;
}

#ifdef NON_VISUALLY_IMPAIRED_HELP
#define CALL_VISUALLY_IMPAIRED(X)
#else
#define CALL_VISUALLY_IMPAIRED(X) cpymo_backend_text_visually_impaired_help(X)
#endif

error_t cpymo_select_img_update(cpymo_engine *e, cpymo_select_img *o, float dt)
{
	if (o->selections) {
		cpymo_key_pluse_update(&o->key_up, dt, e->input.up);
		cpymo_key_pluse_update(&o->key_down, dt, e->input.down);

		if (cpymo_key_pluse_output(&o->key_down)) {
			cpymo_select_img_move(o, 1);
			cpymo_engine_request_redraw(e);

			CALL_VISUALLY_IMPAIRED(o->selections[o->current_selection].original_text);
		}

		if (cpymo_key_pluse_output(&o->key_up)) {
			cpymo_select_img_move(o, -1);
			cpymo_engine_request_redraw(e);

			CALL_VISUALLY_IMPAIRED(o->selections[o->current_selection].original_text);
		}

		if (cpymo_input_mouse_moved(e) && e->input.mouse_position_useable) {
			for (int i = 0; i < (int)o->all_selections; ++i) {
				if (cpymo_select_img_mouse_in_selection(o, i, e)) {
					if (i != o->current_selection) {
						o->current_selection = i;
						cpymo_engine_request_redraw(e);

						CALL_VISUALLY_IMPAIRED(o->selections[o->current_selection].original_text);
					}
				}
			}
		}

		if (CPYMO_INPUT_JUST_RELEASED(e, ok)) {
			return cpymo_select_img_ok(e, o->current_selection, o->selections[o->current_selection].hash, o);
		}

		if (CPYMO_INPUT_JUST_RELEASED(e, cancel) && !o->save_enabled) {
			o->current_selection = (int)o->all_selections - 1;
			while (o->selections[o->current_selection].enabled == false 
					|| (o->selections[o->current_selection].image == NULL
						&& o->selections[o->current_selection].or_text == NULL))
				o->current_selection--;
			return cpymo_select_img_ok(e, o->current_selection, o->selections[o->current_selection].hash, o);
		}

		if (CPYMO_INPUT_JUST_RELEASED(e, mouse_button)) {
			for (int i = 0; i < (int)o->all_selections; ++i) {
				if (cpymo_select_img_mouse_in_selection(o, i, e)) {
					o->current_selection = i;
					return cpymo_select_img_ok(e, i, o->selections[o->current_selection].hash, o);
				}
			}
		}
	}

	return CPYMO_ERR_SUCC;
}

void cpymo_select_img_draw(const cpymo_select_img *o, int logical_screen_w, int logical_screen_h, bool gray_seleted)
{
	if (o->selections) {

		if (o->show_option_background && o->option_background) {
			cpymo_backend_image_draw(
				(float)logical_screen_w / 2.0f - (float)o->option_background_w / 2.0f,
				(float)logical_screen_h / 4.0f - (float)o->option_background_h / 2.0f,
				(float)o->option_background_w, (float)o->option_background_h,
				o->option_background,
				0, 0, o->option_background_w, o->option_background_h, 1.0f,
				o->draw_type);
		}

		for (int i = 0; i < (int)o->all_selections; ++i) {
			const cpymo_select_img_selection *sel = &o->selections[i];
			bool gray = sel->has_selected;
			if (!o->save_enabled || !gray_seleted) gray = false;
			
			if (sel->image) {
				const bool selected = o->current_selection == i;
				cpymo_backend_image_draw(
					sel->x - (float)sel->w / 2.0f,
					sel->y - (float)sel->h / 2.0f,
					(float)sel->w,
					(float)sel->h,
					sel->image,
					selected ? sel->w : 0,
					o->select_img_image ? i * sel->h : 0,
					sel->w,
					sel->h,
					gray ? 0.75f : 1.0f,
					o->draw_type
				);
			}

			if (sel->or_text) {
				const bool selected = o->current_selection == i;

				if (selected && o->sel_highlight) {
					cpymo_backend_image_draw(
						(float)sel->w / 2.0f - (float)o->sel_highlight_w / 2.0f + sel->x,
						(float)sel->h / 2.0f - (float)o->sel_highlight_h / 2.0f + sel->y - sel->h,
						(float)o->sel_highlight_w,
						(float)o->sel_highlight_h,
						o->sel_highlight,
						0, 0, o->sel_highlight_w, o->sel_highlight_h,
						1.0f,
						o->draw_type);
				}

				cpymo_backend_text_draw(
					sel->or_text,
					sel->x,
					sel->y,
					(selected && o->sel_highlight == NULL) ? 
						cpymo_color_inv(sel->text_color) : sel->text_color,
					gray ? 0.75f : 1.0f,
					o->draw_type
				);

			}
		}

		if (o->hint[0] != NULL) {
			int hint_state = o->selections[o->current_selection].hint_state;

			if (hint_state != cpymo_select_img_selection_nohint) {
				int cur_img = 0;
				if (hint_state == cpymo_select_img_selection_hint23) cur_img = 2;
				if (o->hint_tiktok) cur_img++;

				float hint_y = 0.0f;
				if(o->show_option_background && o->option_background)
					hint_y = (float)logical_screen_h / 4.0f - (float)o->option_background_h / 2.0f;

				cpymo_backend_image_draw(
					0,
					hint_y,
					(float)o->hint_w[cur_img],
					(float)o->hint_h[cur_img],
					o->hint[cur_img],
					0,
					0,
					o->hint_w[cur_img],
					o->hint_h[cur_img],
					1.0f,
					o->draw_type);
			}
		}
	}
}

error_t cpymo_select_img_configuare_select_text(
	cpymo_select_img *sel, cpymo_assetloader *loader, cpymo_gameconfig *gc, cpymo_hash_flags *flags,
	cpymo_parser_stream_span text, bool enabled, enum cpymo_select_img_selection_hint_state hint_mode, 
	uint64_t hash, float fontsize)
{
	if (sel->sel_highlight == NULL) {
		error_t err = cpymo_assetloader_load_system_image(
			&sel->sel_highlight,
			&sel->sel_highlight_w,
			&sel->sel_highlight_h,
			cpymo_parser_stream_span_pure("sel_highlight"),
			loader,
			true);

		if (err != CPYMO_ERR_SUCC) {
			sel->sel_highlight = NULL;
			printf("[Warning] Can not load sel_highlight image.\n");
		}
	}

	assert(sel->selections);
	assert(sel->all_selections);
	assert(sel->select_img_image == NULL);

	cpymo_select_img_selection *s = &sel->selections[sel->current_selection++];
	float text_width;
	error_t err =
		cpymo_backend_text_create(
			&s->or_text,
			&text_width,
			text,
			fontsize);
	
	CPYMO_THROW(err);


	s->image = NULL;
	s->enabled = enabled;
	s->h = (int)fontsize + 1;
	s->w = (int)text_width + 1;
	s->hint_state = hint_mode;
	s->hash = hash;
	s->has_selected = cpymo_hash_flags_check(flags, hash);

#ifndef NON_VISUALLY_IMPAIRED_HELP
	s->original_text = (char *)malloc(text.len + 1);
	if (s->original_text)
		cpymo_parser_stream_span_copy(s->original_text, text.len + 1, text);
#endif

	return CPYMO_ERR_SUCC;
}

void cpymo_select_img_configuare_select_text_hint_pic(cpymo_engine * engine, cpymo_parser_stream_span hint)
{
	if (engine->gameconfig.hint == 0) return;

	assert(engine->select_img.hint[0] == NULL);
	assert(engine->select_img.hint[1] == NULL);
	assert(engine->select_img.hint[2] == NULL);
	assert(engine->select_img.hint[3] == NULL);

	engine->select_img.hint_timer = 0;
	engine->select_img.hint_tiktok = false;

	char *hint_pic_name = (char *)malloc(hint.len + 2);
	if (hint_pic_name == NULL) return;

	bool is_all_succ = true;
	for (size_t i = 0; i < 4; ++i) {
		cpymo_parser_stream_span_copy(hint_pic_name, hint.len + 2, hint);
		hint_pic_name[hint.len] = (char)('0' + i);
		hint_pic_name[hint.len + 1] = '\0';

		error_t err = cpymo_assetloader_load_system_image(
			&engine->select_img.hint[i],
			&engine->select_img.hint_w[i],
			&engine->select_img.hint_h[i],
			cpymo_parser_stream_span_pure(hint_pic_name),
			&engine->assetloader,
			true);
		
		if (err != CPYMO_ERR_SUCC) {
			is_all_succ = false;
			engine->select_img.hint[i] = NULL;
			break;
		}
	}

	free(hint_pic_name);

	if (is_all_succ) return;
	else {
		for (size_t i = 0; i < 4; ++i) {
			if (engine->select_img.hint[i] != NULL) {
				cpymo_backend_image_free(engine->select_img.hint[i]);
				engine->select_img.hint[i] = NULL;
			}
		}
	}
}

void cpymo_select_img_configuare_end_select_text(
	cpymo_select_img *sel, cpymo_wait *waiter, cpymo_engine *e, float x1, float y1, float x2, float y2, cpymo_color col, int init_pos, bool show_option_background)
{
	float box_w = x2 - x1;
	float y = y1;
	for (size_t i = 0; i < sel->all_selections; ++i) {
		cpymo_select_img_selection *s = &sel->selections[i];
		s->text_color = col;
		s->x = (box_w - s->w) / 2 + x1;
		s->y = y + s->h;	// y is baseline.
		if(s->enabled) y += s->h;
	}

	float y_offset = (y2 - y) / 2;

	for (size_t i = 0; i < sel->all_selections; ++i)
		sel->selections[i].y += y_offset;

	sel->show_option_background = show_option_background;

	cpymo_select_img_configuare_end(sel, waiter, e, init_pos);

	// load option background if not load.
	if (sel->option_background == NULL && show_option_background) {
		error_t err = cpymo_assetloader_load_system_image(
			&sel->option_background,
			&sel->option_background_w,
			&sel->option_background_h,
			cpymo_parser_stream_span_pure("option"),
			&e->assetloader,
			true);

		if (err != CPYMO_ERR_SUCC) {
			sel->option_background = NULL;
			sel->show_option_background = false;
		}
	}

#ifndef NON_VISUALLY_IMPAIRED_HELP
	const char *hint_header = cpymo_localization_get(e)->visual_help_selection;
	size_t hint_header_len = strlen(hint_header);
	const char *hint_content = sel->selections[sel->current_selection].original_text;
	size_t hint_content_len = strlen(hint_content);
	char *full_first_hint = (char *)malloc(1 + hint_header_len + hint_content_len);
	if (full_first_hint) {
		strcpy(full_first_hint, hint_header);
		strcpy(full_first_hint + hint_header_len, hint_content);
		cpymo_backend_text_visually_impaired_help(full_first_hint);
		free(full_first_hint);
	}
#endif
}
