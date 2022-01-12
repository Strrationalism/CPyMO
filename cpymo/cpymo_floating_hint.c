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
	if (h->background) {
		cpymo_backend_image_draw(
			h->x,
			h->y,
			h->background_w,
			h->background_h,
			h->background,
			0,
			0,
			h->background_w,
			h->background_h,
			1.0f,
			cpymo_backend_image_draw_type_titledate_bg);
	}

	if (h->text) {
		cpymo_backend_text_draw(
			h->text,
			h->x,
			h->y,
			h->color,
			1.0f,
			cpymo_backend_image_draw_type_titledate_text);
	}
}

static bool cpymo_floating_hint_wait(cpymo_engine *e, float dt)
{
	cpymo_floating_hint *h = &e->floating_hint;
	h->time += dt;

	if (cpymo_input_foward_key_just_pressed(e))
		return true;

	return h->time > 10.0f;
}

static error_t cpymo_floating_hint_finish(cpymo_engine *e)
{
	cpymo_floating_hint_free(&e->floating_hint);
	cpymo_floating_hint_init(&e->floating_hint);
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
			engine->gameconfig.fontsize * fontscale);

		if (err != CPYMO_ERR_SUCC) {
			hint->text = NULL;
			cpymo_floating_hint_free(hint);
			cpymo_floating_hint_init(hint);
			return err;
		}
	}

	cpymo_wait_register_with_callback(
		&engine->wait,
		&cpymo_floating_hint_wait,
		&cpymo_floating_hint_finish);

	return CPYMO_ERR_SUCC;
}
