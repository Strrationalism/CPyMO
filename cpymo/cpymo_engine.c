#include "cpymo_engine.h"

#include <cpymo_backend_image.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <cpymo_backend_input.h>

static void cpymo_logo() {
	puts("   __________        __  _______");
	puts("  / ____/ __ \\__  __/  |/  / __ \\");
	puts(" / /   / /_/ / / / / /|_/ / / / /");
	puts("/ /___/ ____/ /_/ / /  / / /_/ /");
	puts("\\____/_/    \\__, /_/  /_/\\____/");
	puts("           /____/");
	puts("");
	puts("This software is licensed under GPLv3.");
	puts("You can only run copies of game that you LEGALLY own.");
	puts("");
	puts("https://github.com/Seng-Jik/cpymo");
	puts("");
}

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

	// init select_img/select_imgs
	cpymo_select_img_init(&out->select_img);

	// init charas
	cpymo_charas_init(&out->charas);

	// init scroll
	cpymo_scroll_init(&out->scroll);

	// init floating hint
	cpymo_floating_hint_init(&out->floating_hint);

	// init say
	cpymo_say_init(&out->say);

	// states
	out->skipping = false;
	out->redraw = true;

	// checks
	if (out->gameconfig.scripttype[0] != 'p'
		|| out->gameconfig.scripttype[1] != 'y'
		|| out->gameconfig.scripttype[2] != 'm'
		|| out->gameconfig.scripttype[3] != '\0')
	{
		printf("[Warning] Script type is %s, some commands maybe unsupported.\n", out->gameconfig.scripttype);
	}

	cpymo_logo();

	return CPYMO_ERR_SUCC;
}

void cpymo_engine_free(cpymo_engine *engine)
{
	cpymo_say_free(&engine->say);
	cpymo_floating_hint_free(&engine->floating_hint);
	cpymo_scroll_free(&engine->scroll);
	cpymo_charas_free(&engine->charas);
	cpymo_select_img_free(&engine->select_img);
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
	delta_time_sec = cpymo_utils_clampf(delta_time_sec, 0.001f, 0.15f);
	error_t err;
	*redraw = engine->redraw;
	engine->redraw = false;

	engine->prev_input = engine->input;
	engine->input = cpymo_input_snapshot();

	err = cpymo_bg_update(&engine->bg, redraw);
	CPYMO_THROW(err);

	cpymo_anime_update(&engine->anime, delta_time_sec, redraw);

	err = cpymo_select_img_update(engine);
	CPYMO_THROW(err);

	err = cpymo_wait_update(&engine->wait, engine, delta_time_sec);
	CPYMO_THROW(err);

	if (!cpymo_wait_is_wating(&engine->wait)) {
		err = cpymo_interpreter_execute_step(engine->interpreter, engine);

		if (cpymo_wait_is_wating(&engine->wait)) {
			if (err != CPYMO_ERR_SUCC && err != CPYMO_ERR_NO_MORE_CONTENT) return err;
		}
		else {
			if (err != CPYMO_ERR_SUCC) return err;
		}
	}

	*redraw |= engine->redraw;

	return CPYMO_ERR_SUCC;
}

void cpymo_engine_draw(const cpymo_engine *engine)
{
	cpymo_bg_draw(engine);
	cpymo_scroll_draw(&engine->scroll);
	cpymo_charas_draw(engine);
	cpymo_anime_draw(&engine->anime);
	cpymo_select_img_draw(
		&engine->select_img, 
		engine->gameconfig.imagesize_w, 
		engine->gameconfig.imagesize_h);

	cpymo_floating_hint_draw(&engine->floating_hint);
	cpymo_flash_draw(engine);
	cpymo_fade_draw(engine);
	cpymo_bg_draw_transform_effect(engine);

	cpymo_say_draw(engine);
}
