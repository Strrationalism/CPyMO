#include "cpymo_floating_hint.h"
#include <assert.h>
#include "cpymo_engine.h"

void cpymo_floating_hint_free(cpymo_floating_hint *h)
{
	if (h->text) cpymo_backend_text_free(h->text);
	if (h->background) cpymo_backend_image_free(h->background);
}

void cpymo_floating_hint_draw(const cpymo_floating_hint *h)
{
	float alpha = 1.0f;

	if (h->time < 1.0f) alpha = h->time;
	if (h->time > 4.0f) alpha = 5.0f - h->time;

	if (h->background) {
		cpymo_backend_image_draw(
			h->x,
			h->y,
			(float)h->background_w,
			(float)h->background_h,
			h->background,
			0,
			0,
			h->background_w,
			h->background_h,
			alpha,
			cpymo_backend_image_draw_type_titledate_bg);
	}

	if (h->text) {
		float xywh[] = { h->x, h->y, cpymo_backend_text_width(h->text),  h->fontsize };
		cpymo_backend_image_fill_rects(xywh, 1, cpymo_color_black, 1.0f, cpymo_backend_image_draw_type_titledate_text);

		cpymo_backend_text_draw(
			h->text,
			h->x,
			h->y + h->fontsize,
			h->color,
			alpha,
			cpymo_backend_image_draw_type_titledate_text);
	}
}

static bool cpymo_floating_hint_wait(cpymo_engine *e, float dt)
{
	cpymo_floating_hint *h = &e->floating_hint;
	h->time += dt;

	if (h->time <= 1.0f || h->time >= 4.0f)
		cpymo_engine_request_redraw(e);

	if (cpymo_input_foward_key_just_pressed(e)) {
		if (h->time <= 1.2f) h->time = 1.2f;
		else if(h->time > 2.0f) return true;
	}

	return h->time > 5.0f;
}

static error_t cpymo_floating_hint_finish(cpymo_engine *e)
{
	cpymo_floating_hint_free(&e->floating_hint);
	cpymo_floating_hint_init(&e->floating_hint);
	cpymo_engine_request_redraw(e);
	return CPYMO_ERR_SUCC;
}

error_t cpymo_floating_hint_start(
	cpymo_engine *engine,
	cpymo_parser_stream_span text,
	cpymo_parser_stream_span background,
	float x, float y, 
	cpymo_color col,
	float fontscale)
{
	cpymo_floating_hint *hint = &engine->floating_hint;

	assert(hint->background == NULL);
	assert(hint->text == NULL);

	hint->x = x;
	hint->y = y;
	hint->color = col;
	hint->time = 0;
	hint->fontsize = engine->gameconfig.fontsize * fontscale;

	if (background.len > 0) {
		error_t err = cpymo_assetloader_load_system_image(
			&hint->background,
			&hint->background_w,
			&hint->background_h,
			background,
			"png",
			&engine->assetloader,
			cpymo_gameconfig_is_symbian(&engine->gameconfig));

		if (err != CPYMO_ERR_SUCC) {
			hint->background = NULL;
			hint->background_w = 0;
			hint->background_h = 0;
		}
	}

	if (text.len > 0) {
		error_t err = cpymo_backend_text_create(
			&hint->text,
			text,
			hint->fontsize);

		if (err != CPYMO_ERR_SUCC) {
			hint->text = NULL;
			cpymo_floating_hint_free(hint);
			cpymo_floating_hint_init(hint);
			return err;
		}
	}

	cpymo_engine_request_redraw(engine);

	cpymo_wait_register_with_callback(
		&engine->wait,
		&cpymo_floating_hint_wait,
		&cpymo_floating_hint_finish);

	return CPYMO_ERR_SUCC;
}
