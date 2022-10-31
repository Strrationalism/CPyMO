#include "cpymo_prelude.h"
#include "cpymo_engine.h"
#include "cpymo_interpreter.h"
#include <cpymo_backend_image.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cpymo_backend_input.h>
#include "cpymo_msgbox_ui.h"
#include "cpymo_save_global.h"
#include <cpymo_backend_input.h>
#include "cpymo_localization.h"

static void cpymo_logo() {
	static bool logo_printed = false;
	if (logo_printed) return;
	logo_printed = true;

	puts("   __________        __  _______");
	puts("  / ____/ __ \\__  __/  |/  / __ \\");
	puts(" / /   / /_/ / / / / /|_/ / / / /");
	puts("/ /___/ ____/ /_/ / /  / / /_/ /");
	puts("\\____/_/    \\__, /_/  /_/\\____/");
	puts("           /____/");
	puts("");
	puts("This software is licensed under AGPLv3.");
	puts("You can only run copies of game that you LEGALLY own.");
	puts("");
	puts("https://github.com/Strrationalism/CPyMO");
	puts("");
}

static error_t cpymo_engine_non_pymo_warning(cpymo_engine *e) 
{
	const cpymo_localization *l = cpymo_localization_get(e);
	return cpymo_msgbox_ui_enter(
		e, cpymo_str_pure(l->mo2pymo_required), NULL, NULL);
}

static error_t cpymo_engine_version_warning(cpymo_engine *e) 
{
	const cpymo_localization *l = cpymo_localization_get(e);
	char *out = NULL;
	error_t err = l->pymo_version_not_compatible_message(
			&out, e->gameconfig.engineversion);
	CPYMO_THROW(err);

	err = cpymo_msgbox_ui_enter(
		e,
		cpymo_str_pure(out),
		NULL,
		NULL);
	free(out);

	return err;
}

static error_t cpymo_engine_feature_level_warning(cpymo_engine *e) 
{
	return cpymo_msgbox_ui_enter(
		e,
		cpymo_str_pure("CPyMO HD Feature Level is not compatible."),
		NULL,
		NULL);
}

error_t cpymo_engine_init(cpymo_engine *out, const char *gamedir)
{
	// init audio system
	cpymo_audio_init(&out->audio);

	// load game config
	const size_t gamedir_strlen = strlen(gamedir);
	char *path = (char *)malloc(gamedir_strlen + 16);
	if (path == NULL) return CPYMO_ERR_OUT_OF_MEM;

	strcpy(path, gamedir);
	strcpy(path + gamedir_strlen, "/gameconfig.txt");
	error_t err = cpymo_gameconfig_parse_from_file(&out->gameconfig, path);
	free(path);
	if (err != CPYMO_ERR_SUCC) return err;

	// get feature level
	if (!strcmp(out->gameconfig.platform, "cpymohd1")) 
		out->feature_level = 1;
	else if (!strcmp(out->gameconfig.platform, "cpymohd2")) 
		out->feature_level = 2;
	else if (!strcmp(out->gameconfig.platform, "cpymohd3")) 
		out->feature_level = 3;
	else
		out->feature_level = 0;

	// set default volume for audio system
	{
		float v = cpymo_utils_clamp(out->gameconfig.bgmvolume, 0, 5) * 0.2f;
		cpymo_audio_set_channel_volume(CPYMO_AUDIO_CHANNEL_BGM, &out->audio, v);
		cpymo_audio_set_channel_volume(CPYMO_AUDIO_CHANNEL_SE, &out->audio, v);
		v = cpymo_utils_clamp(out->gameconfig.vovolume, 0, 5) * 0.2f;
		cpymo_audio_set_channel_volume(CPYMO_AUDIO_CHANNEL_VO, &out->audio, v);
	}

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

	cpymo_script *boot_script;
	err = cpymo_script_create_bootloader(
		&boot_script, out->gameconfig.startscript, out->feature_level);
	if (err != CPYMO_ERR_SUCC) {
		free(out->interpreter);
		cpymo_vars_free(&out->vars);
		cpymo_assetloader_free(&out->assetloader);
		return err;
	}

	cpymo_interpreter_init(out->interpreter, boot_script, true, NULL);

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

	// init text
	cpymo_text_init(&out->text);

	// init hash flags
	cpymo_hash_flags_init(&out->flags);

	// init ui
	out->ui = NULL;

	// init backlog
	err = cpymo_backlog_init(&out->backlog);
	if (err != CPYMO_ERR_SUCC) {
		free(out->title);
		cpymo_interpreter_free(out->interpreter);
		free(out->interpreter);
		cpymo_vars_free(&out->vars);
		cpymo_assetloader_free(&out->assetloader);
		return err;
	}

	// init lua
	#if CPYMO_FEATURE_LEVEL >= 1
	err = cpymo_lua_context_init(&out->lua, out);
	if (err != CPYMO_ERR_SUCC) {
		cpymo_backlog_free(&out->backlog);
		free(out->title);
		cpymo_interpreter_free(out->interpreter);
		free(out->interpreter);
		cpymo_vars_free(&out->vars);
		cpymo_assetloader_free(&out->assetloader);
		return err;
	}
	#endif

	// states
	out->skipping = false;
	out->redraw = true;
	out->ignore_next_mouse_button_flag = false;

	// default config
	out->config_skip_already_read_only = true;

	// load config
	err = cpymo_save_config_load(out);
	if (err != CPYMO_ERR_SUCC && err != CPYMO_ERR_CAN_NOT_OPEN_FILE) {
		printf("[Error] Config data broken! %s\n", cpymo_error_message(err));
	}

	// load global save data
	err = cpymo_save_global_load(out);
	if (err != CPYMO_ERR_SUCC && err != CPYMO_ERR_CAN_NOT_OPEN_FILE) {
		printf("[Error] Global save data broken! %s\n", cpymo_error_message(err));
	}

	out->input = out->prev_input = cpymo_input_snapshot();
	out->ignore_next_mouse_button_flag = out->input.mouse_button;

	cpymo_logo();

	// checks
	if (out->gameconfig.scripttype[0] != 'p'
		|| out->gameconfig.scripttype[1] != 'y'
		|| out->gameconfig.scripttype[2] != 'm'
		|| out->gameconfig.scripttype[3] != '\0') {
		cpymo_wait_callback_after_seconds(&out->wait, 0, &cpymo_engine_non_pymo_warning);
	}

	if (!cpymo_pymo_version_compatible(out->gameconfig.engineversion)) {
		cpymo_wait_callback_after_seconds(
			&out->wait, 0, 
			&cpymo_engine_version_warning);
	}

	if (out->feature_level > CPYMO_FEATURE_LEVEL) {
		cpymo_wait_callback_after_seconds(
			&out->wait, 0,
			&cpymo_engine_feature_level_warning);
	}

	return CPYMO_ERR_SUCC;
}

void cpymo_engine_free(cpymo_engine *engine)
{
	while (engine->ui) cpymo_ui_exit(engine);

	if (engine->assetloader.gamedir) {
		error_t err = cpymo_save_global_save(engine);
		if (err != CPYMO_ERR_SUCC)
			printf("[Error] Can not save global savedata. %s\n", cpymo_error_message(err));

		err = cpymo_save_config_save(engine);
		if (err != CPYMO_ERR_SUCC)
			printf("[Error] Can not save config. %s\n", cpymo_error_message(err));
	}

	#if CPYMO_FEATURE_LEVEL >= 1
	cpymo_lua_context_free(&engine->lua);
	#endif
	
	cpymo_hash_flags_free(&engine->flags);
	cpymo_text_free(&engine->text);
	cpymo_say_free(&engine->say);
	cpymo_backlog_free(&engine->backlog);
	cpymo_floating_hint_free(&engine->floating_hint);
	cpymo_scroll_free(&engine->scroll);
	cpymo_charas_free(&engine->charas);
	cpymo_select_img_free(&engine->select_img);
	cpymo_anime_free(&engine->anime);
	cpymo_bg_free(&engine->bg);
	if (engine->interpreter) {
		cpymo_interpreter_free(engine->interpreter);
		free(engine->interpreter);
	}
	cpymo_vars_free(&engine->vars);
	cpymo_assetloader_free(&engine->assetloader);
	if (engine->title) free(engine->title);
	cpymo_audio_free(&engine->audio);
}

bool cpymo_engine_skipping(cpymo_engine *e)
{
	bool skipping = e->input.skip || e->skipping;
	if (e->config_skip_already_read_only) {
		if (!e->say.current_say_is_already_read) {
			e->skipping = false;
			return false;
		}
	}
	return skipping;
}

error_t cpymo_engine_update(cpymo_engine *engine, float delta_time_sec, bool * redraw)
{
	error_t err;
	*redraw |= engine->redraw;
	engine->redraw = false;

	engine->prev_input = engine->input;
	engine->input = cpymo_input_snapshot();

	if (!engine->prev_input.mouse_button && !engine->input.mouse_button)
		engine->ignore_next_mouse_button_flag = false;

	if (engine->skipping) {
		if (engine->input.ok ||
			engine->input.cancel ||
			engine->input.down ||
			engine->input.hide_window ||
			engine->input.left ||
			engine->input.mouse_button ||
			fabs(engine->input.mouse_wheel_delta) > 0 ||
			engine->input.ok ||
			engine->input.right ||
			engine->input.skip ||
			engine->input.up)
			engine->skipping = false;
	}

	if (engine->input.hide_window != engine->prev_input.hide_window)
		*redraw = true;

	if (cpymo_ui_enabled(engine)) {
		error_t err = cpymo_ui_update(engine, delta_time_sec);
		*redraw |= engine->redraw;
		return err;
	}

#if CPYMO_FEATURE_LEVEL >= 1
	if (engine->feature_level >= 1) {
		err = cpymo_lua_actor_update_main(&engine->lua, delta_time_sec);
		CPYMO_THROW(err);
	}
#endif

	err = cpymo_bg_update(&engine->bg, redraw);
	CPYMO_THROW(err);

	cpymo_anime_update(&engine->anime, delta_time_sec, redraw);

	err = cpymo_select_img_update(engine, &engine->select_img, delta_time_sec);
	CPYMO_THROW(err);

	err = cpymo_wait_update(&engine->wait, engine, delta_time_sec);
	CPYMO_THROW(err);

	if (!cpymo_wait_is_wating(&engine->wait)) {
		if (engine->interpreter)
			err = cpymo_interpreter_execute_step(engine->interpreter, engine);
		else return CPYMO_ERR_NO_MORE_CONTENT;

		if (cpymo_wait_is_wating(&engine->wait)) {
			if (err != CPYMO_ERR_SUCC && err != CPYMO_ERR_NO_MORE_CONTENT) return err;
		}
		else {
			if (err != CPYMO_ERR_SUCC) return err;
		}
	}

	*redraw |= engine->redraw;
	engine->redraw = false;

	#if CPYMO_FEATURE_LEVEL >= 1 && defined LEAKCHECK
	cpymo_lua_context_leakcheck(&engine->lua);
	#endif

	return CPYMO_ERR_SUCC;
}

void cpymo_engine_draw(const cpymo_engine *engine)
{
	if (cpymo_ui_enabled(engine)) {
		cpymo_ui_draw(engine);
		return;
	}

	cpymo_bg_draw(engine);
	cpymo_scroll_draw(&engine->scroll);
	cpymo_charas_draw(engine);

	#if CPYMO_FEATURE_LEVEL >= 1
	if (engine->feature_level >= 1)
		cpymo_lua_actor_draw_main(&engine->lua);
	#endif

	cpymo_anime_draw(&engine->anime);
	cpymo_select_img_draw(
		&engine->select_img, 
		engine->gameconfig.imagesize_w, 
		engine->gameconfig.imagesize_h,
		engine->gameconfig.grayselected);

	cpymo_floating_hint_draw(&engine->floating_hint);
	cpymo_flash_draw(engine);
	cpymo_fade_draw(engine);
	cpymo_bg_draw_transform_effect(engine);

	cpymo_text_draw(engine);
	cpymo_say_draw(engine);

	#if CPYMO_FEATURE_LEVEL >= 1 && defined LEAKCHECK
	cpymo_lua_context_leakcheck((cpymo_lua_context *)&engine->lua);
	#endif
}

