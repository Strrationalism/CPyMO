#include "cpymo_engine.h"

error_t cpymo_engine_init(cpymo_engine * engine, cpymo_gameconfig * gameconfig)
{
	engine->gameconfig = gameconfig;
	engine->image1 = NULL;
	engine->draw = true;
}

void cpymo_engine_update(cpymo_engine *engine, float delta_time_sec, bool * redraw)
{
	*redraw = engine->draw;
	engine->draw = false;
}

void cpymo_engine_draw(cpymo_engine *engine)
{
	float xywh[] = {
		200,100,400,400
	};

	cpymo_color col;
	col.r = 128;
	col.g = 255;
	col.b = 128;

	cpymo_backend_image_fill_rects(xywh, 1, col, 1, cpymo_backend_image_draw_type_bg);
}
