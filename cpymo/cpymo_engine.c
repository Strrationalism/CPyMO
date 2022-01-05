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

	// init wait
	cpymo_wait_reset(&out->wait);

	// init flash
	cpymo_flash_reset(&out->flash);

	// init fade 
	cpymo_fade_reset(&out->fade);

	// init bg
	cpymo_bg_init(&out->bg);

	// init anime
	cpymo_anime_init(&out->anime);

	// states
	out->skipping = false;
	out->redraw = true;

	return CPYMO_ERR_SUCC;
}

void cpymo_engine_free(cpymo_engine * engine)
{
	cpymo_anime_free(&engine->anime);
	cpymo_bg_free(&engine->bg);
	cpymo_interpreter_free(engine->interpreter);
	free(engine->interpreter);
	cpymo_vars_free(&engine->vars);
	cpymo_assetloader_free(&engine->assetloader);
	free(engine->title);
}

error_t cpymo_engine_update(cpymo_engine *engine, float delta_time_sec, bool * redraw)
{
	error_t err;
	*redraw = engine->redraw;
	engine->redraw = false;

	engine->prev_input = engine->input;
	engine->input = cpymo_input_snapshot();

	err = cpymo_wait_update(&engine->wait, engine, delta_time_sec);
	if (err != CPYMO_ERR_SUCC) return err;

	if (!cpymo_wait_is_wating(&engine->wait)) {
		err = cpymo_interpreter_execute_step(engine->interpreter, engine);

		if (cpymo_wait_is_wating(&engine->wait)) {
			if (err != CPYMO_ERR_SUCC && err != CPYMO_ERR_NO_MORE_CONTENT) return err;
		}
		else {
			if (err != CPYMO_ERR_SUCC) return err;
		}
	}

	cpymo_bg_update(&engine->bg, redraw);
	cpymo_anime_update(&engine->anime, delta_time_sec, redraw);

	*redraw |= engine->redraw;

	return CPYMO_ERR_SUCC;
}

void cpymo_engine_draw(cpymo_engine *engine)
{
	cpymo_bg_draw(&engine->bg);
	cpymo_anime_draw(&engine->anime);
	cpymo_flash_draw(engine);
	cpymo_fade_draw(engine);
}
