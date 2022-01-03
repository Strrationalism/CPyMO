#include "cpymo_engine.h"

#include <cpymo_backend_image.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cpymo_backend_input.h>

error_t cpymo_engine_init(cpymo_engine *out, const char *gamedir)
{
	// load game config
	const size_t gamedir_strlen = strlen(gamedir);
	char *path = (char *)malloc(gamedir_strlen + 16);
	if (path == NULL) return CPYMO_ERR_OUT_OF_MEM;

	strcpy(path, gamedir);
	strcpy(path + gamedir_strlen, "/gameconfig.txt");
	error_t err = cpymo_gameconfig_parse_from_file(&out->gameconfig, path);
	free(path);
	if (err != CPYMO_ERR_SUCC) return err;

	// create asset loader
	err = cpymo_assetloader_init(&out->assetloader, &out->gameconfig, gamedir);
	if (err != CPYMO_ERR_SUCC) return err;

	// create vars
	cpymo_vars_init(&out->vars);

	// create script interpreter
	out->interpreter = (cpymo_interpreter *)malloc(sizeof(cpymo_interpreter));
	if (out->interpreter == NULL) {
		cpymo_assetloader_free(&out->assetloader);
		return CPYMO_ERR_OUT_OF_MEM;
	}

	err = cpymo_interpreter_init_boot(out->interpreter, out->gameconfig.startscript);
	if (err != CPYMO_ERR_SUCC) {
		free(out->interpreter);
		cpymo_vars_free(&out->vars);
		cpymo_assetloader_free(&out->assetloader);
		return err;
	}

	// create title
	out->title = (char *)malloc(1);
	if (out->title == NULL) {
		cpymo_interpreter_free(out->interpreter);
		free(out->interpreter);
		cpymo_vars_free(&out->vars);
		cpymo_assetloader_free(&out->assetloader);
		return CPYMO_ERR_OUT_OF_MEM;
	}
	out->title[0] = '\0';

	return CPYMO_ERR_SUCC;
}

void cpymo_engine_free(cpymo_engine * engine)
{
	cpymo_interpreter_free(engine->interpreter);
	free(engine->interpreter);
	cpymo_vars_free(&engine->vars);
	cpymo_assetloader_free(&engine->assetloader);
	free(engine->title);
}

void cpymo_engine_update(cpymo_engine *engine, float delta_time_sec, bool * redraw)
{
	*redraw = false;
}

void cpymo_engine_draw(cpymo_engine *engine)
{
	float xywh0[] = {
		0,0,800,600
	};

	cpymo_color col;
	col.r = 255;
	col.g = 255;
	col.b = 255;
	cpymo_backend_image_fill_rects(xywh0, 1, col, 1, cpymo_backend_image_draw_type_bg);

	float xywh[] = {
		200,100,400,400
	};

	col.r = 128;
	col.g = 255;
	col.b = 128;

	cpymo_backend_image_fill_rects(xywh, 1, col, 1, cpymo_backend_image_draw_type_bg);
}
