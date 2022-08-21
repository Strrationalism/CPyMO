#include "cpymo_prelude.h"
#include "cpymo_bg.h"
#include "cpymo_engine.h"
#include <stb_image.h>
#include <math.h>
#include <assert.h>

void cpymo_bg_free(cpymo_bg *bg)
{
	if (bg->current_bg)
		cpymo_backend_image_free(bg->current_bg);

	if(bg->transform_next_bg)
		cpymo_backend_image_free(bg->transform_next_bg);

	if (bg->trans)
		cpymo_backend_masktrans_free(bg->trans);

	bg->current_bg = NULL;
	bg->trans = NULL;
	bg->transform_next_bg = NULL;

	if (bg->current_bg_name)
		free(bg->current_bg_name);
}

error_t cpymo_bg_update(cpymo_bg *bg, bool *redraw)
{
	if (bg->redraw) *redraw = true;

	bg->redraw = false;

	return CPYMO_ERR_SUCC;
}

void cpymo_bg_draw(const cpymo_engine *e)
{
	const cpymo_bg *bg = &e->bg;

	if (bg->current_bg) {
		float follow_chara_anime_x = 0;
		float follow_chara_anime_y = 0;

		if (bg->follow_chara_quake) {
			const cpymo_charas *c = &e->charas;
			assert(c->anime_pos_current * 2 + 1 < c->anime_pos_count * 2);
			follow_chara_anime_x = c->anime_pos[c->anime_pos_current * 2] * (float)e->gameconfig.imagesize_w / 540.0f;
			follow_chara_anime_y = c->anime_pos[c->anime_pos_current * 2 + 1] * (float)e->gameconfig.imagesize_h / 360.0f;
		}

		cpymo_backend_image_draw(
			bg->current_bg_x + follow_chara_anime_x,
			bg->current_bg_y + follow_chara_anime_y,
			(float)bg->current_bg_w,
			(float)bg->current_bg_h,
			bg->current_bg,
			0,
			0,
			bg->current_bg_w,
			bg->current_bg_h,
			1.0f,
			cpymo_backend_image_draw_type_bg
		);
	}
}

void cpymo_bg_draw_transform_effect(const cpymo_engine * e)
{
	if(e->bg.transform_draw)
		e->bg.transform_draw(e);
}

static void cpymo_bg_draw_transform_effect_alpha(const cpymo_engine *e)
{
	const cpymo_bg *bg = &e->bg;

	assert(bg->transform_next_bg);
	cpymo_backend_image_draw(
		bg->transform_next_bg_x,
		bg->transform_next_bg_y,
		(float)bg->transform_next_bg_w,
		(float)bg->transform_next_bg_h,
		bg->transform_next_bg,
		0,
		0,
		bg->transform_next_bg_w,
		bg->transform_next_bg_h,
		cpymo_tween_value(&bg->transform_progression),
		cpymo_backend_image_draw_type_bg
	);
}

static void cpymo_bg_draw_transform_effect_fade(const cpymo_engine *e)
{
	float alpha;
	const float progression = cpymo_tween_value(&e->bg.transform_progression);

	if (progression < 0.5f) alpha = progression * 2.0f;
	else alpha = 1.0f - (progression - 0.5f) * 2.0f;

	if (e->bg.trans == NULL) {
		float xywh[] = { 
			0, 0, 
			(float)e->gameconfig.imagesize_w, 
			(float)e->gameconfig.imagesize_h };

		cpymo_backend_image_fill_rects(
			xywh,
			1,
			cpymo_color_black,
			alpha,
			cpymo_backend_image_draw_type_bg);
	}
	else {
		cpymo_backend_masktrans_draw(
			e->bg.trans, 
			progression > 0.5f ? 1 - alpha : alpha,
			progression > 0.5f);
	}
}

static void cpymo_bg_transfer_operate(cpymo_bg *bg)
{
	assert(bg->transform_next_bg);

	if (bg->current_bg)
		cpymo_backend_image_free(bg->current_bg);
	bg->current_bg = bg->transform_next_bg;
	bg->transform_next_bg = NULL;

	bg->current_bg_w = bg->transform_next_bg_w;
	bg->current_bg_h = bg->transform_next_bg_h;
	bg->current_bg_x = bg->transform_next_bg_x;
	bg->current_bg_y = bg->transform_next_bg_y;
	bg->transform_draw = NULL;

	bg->redraw = true;
}

static void cpymo_bg_transfer(cpymo_engine *e)
{
	cpymo_engine_request_redraw(e);

	cpymo_bg_transfer_operate(&e->bg);

	// After transfer
	cpymo_charas_fast_kill_all(&e->charas);
	cpymo_scroll_reset(&e->scroll);
}

static bool cpymo_bg_wait_for_progression(cpymo_engine *engine, float delta_time)
{
	cpymo_engine_request_redraw(engine);

	if (cpymo_input_foward_key_just_pressed(engine))
		cpymo_tween_finish(&engine->bg.transform_progression);

	cpymo_tween_update(&engine->bg.transform_progression, delta_time);
	return cpymo_tween_finished(&engine->bg.transform_progression);
}

static error_t cpymo_bg_progression_over_callback(cpymo_engine *e)
{
	cpymo_engine_request_redraw(e);
	if (e->bg.transform_next_bg)
		cpymo_bg_transfer(e);
	
	if (e->bg.trans)
		cpymo_backend_masktrans_free(e->bg.trans);
	e->bg.trans = NULL;
	e->bg.transform_draw = NULL;

	return CPYMO_ERR_SUCC;
}

static bool cpymo_bg_wait_for_progression_fade(cpymo_engine *engine, float delta_time)
{
	cpymo_engine_request_redraw(engine);
	cpymo_tween *tween = &engine->bg.transform_progression;
	if (cpymo_tween_value(tween) < 0.5f && cpymo_tween_value_after(tween, delta_time) >= 0.5f) {
		if (engine->bg.transform_next_bg) {
			cpymo_bg_transfer(engine);
			engine->bg.transform_draw = &cpymo_bg_draw_transform_effect_fade;
		}
	}

	return cpymo_bg_wait_for_progression(engine, delta_time);
}

error_t cpymo_bg_command(
	cpymo_engine *engine,
	cpymo_bg *bg,
	cpymo_str bgname,
	cpymo_str transition,
	float x,
	float y,
	float time)
{
	int w, h;
	cpymo_backend_image img;
	error_t err = cpymo_assetloader_load_bg_image(&img, &w, &h, bgname, &engine->assetloader);
	CPYMO_THROW(err);

	char *next_bg_name = (char *)realloc(bg->current_bg_name, bgname.len + 1);
	if (next_bg_name) {
		cpymo_str_copy(next_bg_name, bgname.len + 1, bgname);
		bg->current_bg_name = next_bg_name;
	}

	if (bg->transform_next_bg)
		cpymo_backend_image_free(bg->transform_next_bg);

	bg->transform_next_bg = img;
	bg->transform_next_bg_w = w;
	bg->transform_next_bg_h = h;

	// In pymo, when x = y = 0 and bg smaller than screen, bg will be centered.
	if (fabs(x) < 1 && fabs(y) < 1 && (w <= engine->gameconfig.imagesize_w || h <= engine->gameconfig.imagesize_h)) {
		bg->transform_next_bg_x = (float)(engine->gameconfig.imagesize_w - w) / 2.0f;
		bg->transform_next_bg_y = (float)(engine->gameconfig.imagesize_h - h) / 2.0f;
	} 
	else {
		bg->transform_next_bg_x = -(x / 100.0f) * (float)w;
		bg->transform_next_bg_y = -(y / 100.0f) * (float)h;
	}

#ifdef LOW_FRAME_RATE
	transition = cpymo_str_pure("BG_NOFADE");
#endif

	if (cpymo_str_equals_str(transition, "BG_NOFADE")) {
		cpymo_bg_transfer(engine);
	}
	else if (cpymo_str_equals_str(transition, "BG_ALPHA")) {
		bg->transform_progression = cpymo_tween_create(time);
		bg->transform_draw = &cpymo_bg_draw_transform_effect_alpha;
		cpymo_wait_register_with_callback(
			&engine->wait,
			&cpymo_bg_wait_for_progression,
			&cpymo_bg_progression_over_callback);
	}
	else {
		if (!cpymo_str_equals_str(transition, "BG_FADE")) {
			cpymo_backend_masktrans trans;
			if (cpymo_assetloader_load_system_masktrans(
				&trans, 
				transition, 
				&engine->assetloader) == CPYMO_ERR_SUCC) 
				bg->trans = trans;
			else {
				printf("[Warning] Failed to load mask transition.\n");
			}
		}

		bg->transform_progression = cpymo_tween_create(time * 2);
		bg->transform_draw = &cpymo_bg_draw_transform_effect_fade;
		cpymo_wait_register_with_callback(
			&engine->wait,
			&cpymo_bg_wait_for_progression_fade,
			&cpymo_bg_progression_over_callback);
	}

	cpymo_engine_request_redraw(engine);

	return CPYMO_ERR_SUCC;
}


