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
		cpymo_backend_image_free(bg->current_bg);
}

error_t cpymo_bg_update(cpymo_bg *bg, bool *redraw)
{
	if (bg->redraw) *redraw = true;

	bg->redraw = false;

	return CPYMO_ERR_SUCC;
}

void cpymo_bg_draw(const cpymo_bg *bg)
{
	if (bg->current_bg) {
		cpymo_backend_image_draw(
			bg->current_bg_x,
			bg->current_bg_y,
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

static void cpymo_bg_transfer(cpymo_bg *bg)
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
		cpymo_bg_transfer(&e->bg);
	return CPYMO_ERR_SUCC;
}

error_t cpymo_bg_command(
	cpymo_engine *engine,
	cpymo_bg *bg,
	cpymo_parser_stream_span bgname,
	cpymo_parser_stream_span transition,
	float x,
	float y,
	float time)
{
	char bg_name[40];
	cpymo_parser_stream_span_copy(bg_name, sizeof(bg_name), bgname);

	char *buf = NULL;
	size_t buf_size = 0;
	error_t err = cpymo_assetloader_load_bg(&buf, &buf_size, bg_name, &engine->assetloader);
	if (err != CPYMO_ERR_SUCC) return err;

	int w, h, channels;
	stbi_uc *pixels = stbi_load_from_memory((stbi_uc *)buf, (int)buf_size, &w, &h, &channels, 3);
	free(buf);

	if (pixels == NULL)
		return CPYMO_ERR_BAD_FILE_FORMAT;

	cpymo_backend_image img;
	err = cpymo_backend_image_load_immutable(
		&img,
		pixels,
		w,
		h,
		cpymo_backend_image_format_rgb);

	if (err != CPYMO_ERR_SUCC) {
		free(pixels);
		return err;
	}

	if (bg->transform_next_bg)
		cpymo_backend_image_free(bg->transform_next_bg);

	bg->transform_next_bg = img;
	bg->transform_next_bg_w = w;
	bg->transform_next_bg_h = h;

	// In pymo, when x = y = 0 and bg smaller than screen, bg will be centered.
	if (fabs(x) < 1 && fabs(y) < 1 && w <= engine->gameconfig.imagesize_w && h <= engine->gameconfig.imagesize_h) {
		bg->transform_next_bg_x = (float)(engine->gameconfig.imagesize_w - w) / 2.0f;
		bg->transform_next_bg_y = (float)(engine->gameconfig.imagesize_h - h) / 2.0f;
	} 
	else {
		bg->transform_next_bg_x = -(x / 100.0f) * (float)w;
		bg->transform_next_bg_y = -(y / 100.0f) * (float)h;
	}

	if (cpymo_parser_stream_span_equals_str(transition, "BG_NOFADE")) {
		cpymo_bg_transfer(bg);
	}
	else if (cpymo_parser_stream_span_equals_str(transition, "BG_ALPHA")) {
		bg->transform_progression = cpymo_tween_create(time);
		bg->transform_draw = &cpymo_bg_draw_transform_effect_alpha;
		cpymo_wait_register_with_callback(
			&engine->wait,
			&cpymo_bg_wait_for_progression,
			&cpymo_bg_progression_over_callback);
	}
	else {
		printf("[Warning] Unsupported bg transition.\n");
		cpymo_bg_transfer(bg);
		cpymo_wait_for_seconds(engine, time);
	}

	return CPYMO_ERR_SUCC;
}

