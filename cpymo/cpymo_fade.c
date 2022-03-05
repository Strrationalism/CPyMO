#include "cpymo_fade.h"
#include "cpymo_engine.h"
#include <cpymo_backend_image.h>

void cpymo_fade_draw(const cpymo_engine *engine)
{
	if (engine->fade.state != cpymo_fade_disabled) {
		float xywh[] = {
			0, 0,
			(float)engine->gameconfig.imagesize_w,
			(float)engine->gameconfig.imagesize_h
		};

		float alpha = cpymo_tween_progress(&engine->fade.alpha);
		if (engine->fade.state == cpymo_fade_in) alpha = 1.0f - alpha;
		else if (engine->fade.state == cpymo_fade_keep) alpha = 1.0f;

		cpymo_backend_image_fill_rects(xywh, 1, engine->fade.col, alpha, cpymo_backend_image_draw_type_bg);
	}
}

void cpymo_fade_start_fadeout(cpymo_engine *engine, float time, cpymo_color col)
{
	engine->fade.col = col;
	cpymo_fade_start_fadein(engine, time);
	engine->fade.state = cpymo_fade_out;
}

static bool cpymo_fade_waiter(cpymo_engine *engine, float delta_time)
{
	if (cpymo_input_foward_key_just_pressed(engine))
		cpymo_tween_finish(&engine->fade.alpha);

	cpymo_engine_request_redraw(engine);
	cpymo_tween_update(&engine->fade.alpha, delta_time);
	return cpymo_tween_finished(&engine->fade.alpha);
}

static error_t cpymo_fade_finish_callback(cpymo_engine *engine)
{
	if (engine->fade.state == cpymo_fade_in) {
		engine->fade.state = cpymo_fade_disabled;
	}
	else if (engine->fade.state == cpymo_fade_out) {
		engine->fade.state = cpymo_fade_keep;
		cpymo_tween_finish(&engine->fade.alpha);
	}
	return CPYMO_ERR_SUCC;
}

void cpymo_fade_start_fadein(cpymo_engine *engine, float time)
{
	engine->fade.state = cpymo_fade_in;
	engine->fade.alpha = cpymo_tween_create(time);

	cpymo_wait_register_with_callback(
		&engine->wait, 
		&cpymo_fade_waiter, 
		&cpymo_fade_finish_callback);
}
