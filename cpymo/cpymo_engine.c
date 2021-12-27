#include "cpymo_engine.h"

#include <cpymo_backend_image.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

error_t cpymo_engine_init(cpymo_engine *out, const char *gamedir)
{
	// load game config
	const size_t gamedir_strlen = strlen(gamedir);
	char *path = (char *)malloc(gamedir_strlen + 16);
	if (path == NULL) return CPYMO_ERR_OUT_OF_MEM;

	strncpy(path, gamedir, gamedir_strlen);
	strcpy(path + gamedir_strlen, "/gameconfig.txt");
	error_t err = cpymo_gameconfig_parse_from_file(&out->gameconfig, path);
	free(path);
	if (err != CPYMO_ERR_SUCC) return err;

	// create asset loader
	err = cpymo_assetloader_init(&out->assetloader, &out->gameconfig, gamedir);
	if (err != CPYMO_ERR_SUCC) return err;

	// states
	out->draw = false;

	return CPYMO_ERR_SUCC;
}

void cpymo_engine_free(cpymo_engine * engine)
{
	cpymo_assetloader_free(&engine->assetloader);
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