#include "cpymo_prelude.h"
#include "cpymo_flash.h"
#include "cpymo_engine.h"
#include <cpymo_backend_image.h>

static error_t cpymo_flash_callback(cpymo_engine *engine)
{
	engine->flash.enable = false;
	cpymo_engine_request_redraw(engine);
	return CPYMO_ERR_SUCC;
}

void cpymo_flash_start(cpymo_engine * engine, cpymo_color col, float time)
{
	cpymo_engine_request_redraw(engine);
	engine->flash.color = col;
	engine->flash.enable = true;

	cpymo_wait_callback_after_seconds(&engine->wait, time, &cpymo_flash_callback);
}

void cpymo_flash_draw(const cpymo_engine *engine)
{
	if (engine->flash.enable) {
		float xywh[] = {
			0, 0,
			(float)engine->gameconfig.imagesize_w,
			(float)engine->gameconfig.imagesize_h
		};

		cpymo_backend_image_fill_rects(xywh, 1, engine->flash.color, 1, cpymo_backend_image_draw_type_bg);
	}
}

