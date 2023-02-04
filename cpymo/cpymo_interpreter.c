#include "cpymo_prelude.h"
#include "cpymo_interpreter.h"
#include "cpymo_engine.h"
#include "cpymo_album.h"
#include "cpymo_music_box.h"
#include "cpymo_config_ui.h"
#include "cpymo_save_ui.h"
#include "cpymo_save.h"
#include "cpymo_movie.h"
#include "cpymo_localization.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <assert.h>
#include <ctype.h>

void cpymo_interpreter_init(
	cpymo_interpreter *out, 
	cpymo_script *script, 
	bool own_script,
	cpymo_interpreter *caller)
{
	out->script = script;
	out->own_script = own_script;
	cpymo_parser_init(
		&out->script_parser, 
		out->script->script_content, 
		out->script->script_content_len);
	out->no_more_content = false;
	out->caller = caller;
	out->checkpoint.cur_line = 0;
}

error_t cpymo_interpreter_init_script(
	cpymo_interpreter *out, 
	cpymo_str script_name, 
	const cpymo_assetloader *loader,
	cpymo_interpreter *caller)
{	
	out->script = NULL;
	out->own_script = true;

	cpymo_interpreter *find_script = caller;
	while (find_script) {
		if (cpymo_str_equals_str(script_name, find_script->script->script_name)) {
			out->script = find_script->script;
			out->own_script = false;
			break;
		}
		find_script = find_script->caller;
	}

	if (out->script == NULL) {
		error_t err = cpymo_script_load(&out->script, script_name, loader);
		if (err != CPYMO_ERR_SUCC) {
			out->own_script = false;
			return err;
		}
	}

	cpymo_interpreter_init(out, out->script, out->own_script, caller);

	return CPYMO_ERR_SUCC;
}

error_t cpymo_interpreter_goto_line(cpymo_interpreter * interpreter, uint64_t line)
{
	cpymo_parser_reset(&interpreter->script_parser);
	while (line != interpreter->script_parser.cur_line)
		if (!cpymo_parser_next_line(&interpreter->script_parser))
			return CPYMO_ERR_NO_MORE_CONTENT;
	return CPYMO_ERR_SUCC;
}

void cpymo_interpreter_free(cpymo_interpreter * interpreter)
{
	cpymo_interpreter *caller = interpreter->caller;
	while (caller) {
		cpymo_interpreter *to_free = caller;
		caller = caller->caller;

		if (to_free->own_script)
			cpymo_script_free(to_free->script);
		free(to_free);
	}

	if (interpreter->own_script)
		cpymo_script_free(interpreter->script);
}

error_t cpymo_interpreter_goto_label(cpymo_interpreter * interpreter, cpymo_str label)
{
	bool retring = false;
	uint64_t cur_line_num = interpreter->script_parser.cur_line;

RETRY:
	while (1) {
		cpymo_str command = 
			cpymo_parser_curline_pop_command(&interpreter->script_parser);

		if (cpymo_str_equals_str(command, "label")) {
			cpymo_str cur_label = 
				cpymo_parser_curline_pop_commacell(&interpreter->script_parser);

			if (cpymo_str_equals(cur_label, label)) {
				cpymo_parser_next_line(&interpreter->script_parser);
				return CPYMO_ERR_SUCC;
			}
		}
		else {
			if (!cpymo_parser_next_line(&interpreter->script_parser)) {
				if (retring) {
					char label_name[32];
					cpymo_str_copy(label_name, sizeof(label_name), label);
					printf("[Error] Can not find label %s in script %s.\n", 
						label_name, interpreter->script->script_name);
					return cpymo_interpreter_goto_line(interpreter, cur_line_num);
				}
				else {
					cpymo_parser_reset(&interpreter->script_parser);
					retring = true;
					goto RETRY;
				}
			}
		}
	}
}

static error_t cpymo_interpreter_dispatch(cpymo_str command, cpymo_interpreter *interpreter, cpymo_engine *engine, jmp_buf cont);

#define CPYMO_EXEC_CONTVAL_OK 1
#define CPYMO_EXEC_CONTVAL_INTERPRETER_UPDATED 2

error_t cpymo_interpreter_execute_step(cpymo_interpreter * interpreter, cpymo_engine *engine)
{
	jmp_buf cont;

	switch (setjmp(cont)) {
	case 0: break;
	case CPYMO_EXEC_CONTVAL_OK: break;
	case CPYMO_EXEC_CONTVAL_INTERPRETER_UPDATED: interpreter = engine->interpreter; break;
	default: return CPYMO_ERR_INVALID_ARG;
	}

	cpymo_str command =
		cpymo_parser_curline_pop_command(&interpreter->script_parser);

	error_t err = cpymo_interpreter_dispatch(command, interpreter, engine, cont);
	switch (err) {
	case CPYMO_ERR_NOT_FOUND:
	case CPYMO_ERR_CAN_NOT_OPEN_FILE:
	case CPYMO_ERR_BAD_FILE_FORMAT:
	case CPYMO_ERR_UNSUPPORTED:
	case CPYMO_ERR_INVALID_ARG:
	case CPYMO_ERR_UNKNOWN:
		printf("[Error] In script \'%s\'(%d): %s\n",
			interpreter->script->script_name,
			(int)interpreter->script_parser.cur_line,
			cpymo_error_message(err));
		break;
	default: return err;
	};

	if (!cpymo_parser_next_line(&interpreter->script_parser)) {
		if (interpreter->no_more_content)
			return CPYMO_ERR_NO_MORE_CONTENT;
		else
			interpreter->no_more_content = true;
	}

	return CPYMO_ERR_SUCC;
}

void cpymo_interpreter_checkpoint(cpymo_interpreter * interpreter)
{
	interpreter->checkpoint.cur_line = interpreter->script_parser.cur_line;
}

#define D(CMD) \
	else if (cpymo_str_equals_str(command, CMD))

#define D_FEATURE_LEVEL(LEVEL, CMD) \
	else if (engine->feature_level >= LEVEL && cpymo_str_equals_str(command, CMD))

#define POP_ARG(X) \
	cpymo_str X = cpymo_parser_curline_pop_commacell(&interpreter->script_parser); \
	cpymo_str_trim(&X)

#define IS_EMPTY(X) \
	cpymo_str_equals_str(X, "")
	
#define ENSURE(X) \
	{ if (IS_EMPTY(X)) return CPYMO_ERR_INVALID_ARG; }

#define POS(OUT_X, OUT_Y, IN_X, IN_Y) \
	float OUT_X = cpymo_str_atof(IN_X) / 100.0f * engine->gameconfig.imagesize_w; \
	float OUT_Y = cpymo_str_atof(IN_Y) / 100.0f * engine->gameconfig.imagesize_h;

#define CONT_WITH_CURRENT_CONTEXT { longjmp(cont, CPYMO_EXEC_CONTVAL_OK); return CPYMO_ERR_UNKNOWN; }

#define CONT_NEXTLINE { \
	if (cpymo_parser_next_line(&interpreter->script_parser))	\
		{ longjmp(cont, CPYMO_EXEC_CONTVAL_OK); return CPYMO_ERR_UNKNOWN; }	\
	else return CPYMO_ERR_NO_MORE_CONTENT; }

static error_t cpymo_interpreter_dispatch(cpymo_str command, cpymo_interpreter *interpreter, cpymo_engine *engine, jmp_buf cont)
{
	error_t err;

	if (IS_EMPTY(command)) {
		CONT_NEXTLINE;
	}

	/*** I. Text ***/
	D("say") {
		cpymo_fade_reset(&engine->fade);
		cpymo_interpreter_checkpoint(interpreter);

		POP_ARG(name_or_text);
		POP_ARG(text);

		if (IS_EMPTY(text)) {
			text = name_or_text;
			name_or_text.len = 0;
		}

		cpymo_engine_extract_text(engine, name_or_text);
		if (name_or_text.len)
			cpymo_engine_extract_text_cstr(engine, "\n");
		cpymo_engine_extract_text(engine, text);
		cpymo_engine_extract_text_submit(engine);

		if (IS_EMPTY(text)) {
			text = cpymo_str_pure(" ");
		}

		cpymo_text_clear(&engine->text);

		return cpymo_say_start(engine, name_or_text, text);
	}

	D("text") {
		POP_ARG(content); ENSURE(content);
		POP_ARG(x1_str); ENSURE(x1_str);
		POP_ARG(y1_str); ENSURE(y1_str);
		POS(x1, y1, x1_str, y1_str);
		POP_ARG(x2_str); ENSURE(x2_str);
		POP_ARG(y2_str); ENSURE(y2_str);
		POS(x2, y2, x2_str, y2_str);
		POP_ARG(col_str); ENSURE(col_str);
		cpymo_color col = cpymo_str_as_color(col_str);
		POP_ARG(fontsize_str); ENSURE(fontsize_str);
		float fontsize = 
			cpymo_str_atof(fontsize_str) * 
			engine->gameconfig.imagesize_h / 240.0f * 1.2f;
		POP_ARG(show_immediately_str);
		bool show_immediately = cpymo_str_atoi(show_immediately_str) != 0;

		cpymo_engine_extract_text(engine, content);
		cpymo_engine_extract_text_submit(engine);

		return cpymo_text_new(engine, x1, y1, x2, y2, col, fontsize, content, show_immediately);
	}

	D("text_off") {
		cpymo_engine_request_redraw(engine);
		cpymo_text_clear(&engine->text);
		return CPYMO_ERR_SUCC;
	}

	D("waitkey") {
		cpymo_engine_request_redraw(engine);
		cpymo_wait_for_seconds(&engine->wait, 5.0f);
		return CPYMO_ERR_SUCC;
	}

	D("title") {
		POP_ARG(title);

		char *buf = cpymo_str_copy_malloc_trim_memory(engine, title);
		if (buf == NULL) return CPYMO_ERR_OUT_OF_MEM;

		free(engine->title);
		engine->title = buf;
		
		CONT_NEXTLINE;
	}

	D("title_dsp") {
		if (strlen(engine->title) <= 0)
			CONT_NEXTLINE;

		cpymo_backend_text_extract(engine->title);

		return cpymo_floating_hint_start(
			engine,
			cpymo_str_pure(engine->title),
			cpymo_str_pure(""),
			2 * cpymo_gameconfig_font_size(&engine->gameconfig),
			cpymo_gameconfig_font_size(&engine->gameconfig),
			cpymo_color_white,
			1.0f);
	}

#define CHARA_BUF_SIZE 64

	/*** II. Video ***/
	D("chara") {
		int chara_ids[CHARA_BUF_SIZE];
		int layers[CHARA_BUF_SIZE];
		float pos_x_s[CHARA_BUF_SIZE];
		cpymo_str filenames[CHARA_BUF_SIZE];
		size_t command_buffer_size = 0;

		float time = 0.3f;
		while (true) {
			POP_ARG(chara_id_or_time_str);
			POP_ARG(filename);
			POP_ARG(pos_x_str);
			POP_ARG(layer_str);

			int chara_id_or_time = cpymo_str_atoi(chara_id_or_time_str);
			
			if (IS_EMPTY(filename) && IS_EMPTY(pos_x_str) && IS_EMPTY(layer_str)) {
				time = (float)chara_id_or_time / 1000.0f;
				break;
			}

			int chara_id = chara_id_or_time;

			ENSURE(filename);
			ENSURE(pos_x_str);
			ENSURE(layer_str);

			if (command_buffer_size >= CHARA_BUF_SIZE) {
				printf("[Warning] chara command buffer was overflow.\n");
			} 
			else {
				chara_ids[command_buffer_size] = chara_id;
				layers[command_buffer_size] = cpymo_str_atoi(layer_str);
				pos_x_s[command_buffer_size] =
					(float)cpymo_str_atoi(pos_x_str) / 100.0f * (float)engine->gameconfig.imagesize_w;
				filenames[command_buffer_size] = filename;
				command_buffer_size++;
			}
		}

		for (size_t i = 0; i < command_buffer_size; ++i) {
			if (cpymo_str_equals_str(filenames[i], "NULL")) {
				cpymo_charas_kill(engine, chara_ids[i], time);
			}
			else {
				struct cpymo_chara *ch;
				err = cpymo_charas_new_chara(
					engine, 
					&ch, 
					filenames[i], 
					chara_ids[i], 
					layers[i], 
					5, 
					pos_x_s[i], 
					0, 
					0,
					time);
				CPYMO_THROW(err);
			}
		}

		cpymo_charas_wait(engine);

		return CPYMO_ERR_SUCC;
	}

	D("chara_cls") {
		POP_ARG(id_str); ENSURE(id_str);
		POP_ARG(time_str);

		float time = IS_EMPTY(time_str) ? 0.3f : (float)cpymo_str_atoi(time_str) / 1000.0f;
		if (cpymo_str_equals_str(id_str, "a"))
			cpymo_charas_kill_all(engine, time);
		else cpymo_charas_kill(engine, cpymo_str_atoi(id_str), time);

		cpymo_charas_wait(engine);
		return CPYMO_ERR_SUCC;
	}

	D("chara_pos") {
		POP_ARG(id_str); ENSURE(id_str);
		POP_ARG(x_str); ENSURE(x_str);
		POP_ARG(y_str); ENSURE(y_str);
		POP_ARG(coord_mode_str);

		int id = cpymo_str_atoi(id_str);
		POS(x, y, x_str, y_str);

		int coord_mode = IS_EMPTY(coord_mode_str) ? 5 : cpymo_str_atoi(coord_mode_str);

		cpymo_charas_pos(engine, id, coord_mode, x, y);
		return CPYMO_ERR_SUCC;
	}

	D("bg") {
		POP_ARG(bg_name); ENSURE(bg_name);
		POP_ARG(transition);
		POP_ARG(time_str);
		POP_ARG(x_str);
		POP_ARG(y_str);

		float x, y, time;

		cpymo_album_cg_unlock(engine, bg_name);

		if (cpymo_str_equals_str(time_str, "BG_VERYFAST")) time = 0.1f;
		if (cpymo_str_equals_str(time_str, "BG_FAST")) time = 0.175f;
		else if (cpymo_str_equals_str(time_str, "BG_NORMAL")) time = 0.25f;
		else if (cpymo_str_equals_str(time_str, "BG_SLOW")) time = 0.5f;
		else if (cpymo_str_equals_str(time_str, "BG_VERYSLOW")) time = 1.0f;
		else if (!IS_EMPTY(time_str))
			time = (float)cpymo_str_atoi(time_str) / 1000.0f;
		else time = 0.3f;
		

		if (IS_EMPTY(x_str)) x = 0.0f; else x = (float)cpymo_str_atoi(x_str);
		if (IS_EMPTY(y_str)) y = 0.0f; else y = (float)cpymo_str_atoi(y_str);

		if (IS_EMPTY(transition)) {
			transition.begin = "BG_ALPHA";
			transition.len = strlen(transition.begin);
		}

		err = cpymo_bg_command(
			engine,
			&engine->bg,
			bg_name,
			transition,
			x,
			y,
			time
		);

		return err;
	}

	D("flash") {
		POP_ARG(col_str); ENSURE(col_str);
		POP_ARG(time_str); ENSURE(time_str);

		cpymo_color col = cpymo_str_as_color(col_str);
		float time = cpymo_str_atoi(time_str) / 1000.0f;

		cpymo_flash_start(engine, col, time);

		return CPYMO_ERR_SUCC;
	}

	D("quake") {
		static float offsets[] = { -1, -2, 4, 3, 6, -4, 5, 3, 2, -1, 0, 0 };
		cpymo_charas_play_anime(
			engine, 0.06f, 1, offsets,
			sizeof(offsets) / sizeof(float) / 2, false);
		cpymo_charas_set_all_chara_play_anime(&engine->charas);
		cpymo_bg_follow_chara_quake(&engine->bg, true);
		return CPYMO_ERR_SUCC;
	}

	D("fade_out") {
		POP_ARG(col_str); ENSURE(col_str);
		POP_ARG(time_str); ENSURE(time_str);

		cpymo_color col = cpymo_str_as_color(col_str);
		float time = cpymo_str_atoi(time_str) / 1000.0f;

		cpymo_fade_start_fadeout(engine, time, col);
		return CPYMO_ERR_SUCC;
	}

	D("fade_in") {
		POP_ARG(time_str); ENSURE(time_str);
		float time = cpymo_str_atoi(time_str) / 1000.0f;
		cpymo_fade_start_fadein(engine, time);
		return CPYMO_ERR_SUCC;
	}

	D("movie") {
		POP_ARG(movie_name);

		if (engine->gameconfig.playvideo) {
			return cpymo_movie_play(engine, movie_name);
		}
		else {
			cpymo_audio_bgm_stop(engine);
			cpymo_audio_se_stop(engine);
			cpymo_audio_vo_stop(engine);
			printf("[Info] Playvideo is disabled in gameconfig.\n");
			CONT_NEXTLINE;
		}
	}

	D("textbox") {
		POP_ARG(msg); ENSURE(msg);
		POP_ARG(name); ENSURE(name);

		cpymo_engine_request_redraw(engine);
		error_t err = cpymo_say_load_msgbox_and_namebox_image(
			&engine->say, msg, name, &engine->assetloader);
		CPYMO_THROW(err);

		CONT_NEXTLINE;
	}

	#define CHARA_QUAKE(NAME, ...) \
		D(NAME) { \
			static float offsets[] = { __VA_ARGS__}; \
			cpymo_charas_play_anime( \
				engine, 0.1f, 1, \
				offsets, \
				sizeof(offsets) / sizeof(float) / 2, \
				false); \
			\
			while (true) { \
				POP_ARG(id); \
				if (IS_EMPTY(id)) break; \
				\
				cpymo_charas_set_play_anime(&engine->charas, cpymo_str_atoi(id)); \
			} \
			\
			return CPYMO_ERR_SUCC; \
		}

	CHARA_QUAKE("chara_quake", -10, 3, 10, 3, -6, 2, 5, 2, -4, 1, 3, 0, -1, 0, 0, 0)
	CHARA_QUAKE("chara_down", 0, 7, 0, 16, 0, 12, 0, 16, 0, 7, 0, 0)
	CHARA_QUAKE("chara_up", 0, -16, 0, 0, 0, -6, 0, 0)
	#undef CHARA_QUAKE

	D("chara_anime") {
		POP_ARG(id_str); ENSURE(id_str);
		POP_ARG(peroid_str); ENSURE(peroid_str);
		POP_ARG(loop_str); ENSURE(loop_str);

		int loops = cpymo_str_atoi(loop_str);

		float *buffer = (float *)malloc(64 * sizeof(float));
		if (buffer == NULL) return CPYMO_ERR_OUT_OF_MEM;
		size_t offsets = 0;

		while (offsets < 32) {
			POP_ARG(x_str);
			if (IS_EMPTY(x_str)) break;

			POP_ARG(y_str); 
			if (IS_EMPTY(y_str)) break;

			float x = (float)cpymo_str_atoi(x_str);
			float y = (float)cpymo_str_atoi(y_str);
			buffer[offsets * 2] = x;
			buffer[offsets * 2 + 1] = y;
			offsets++;
		}

		if (offsets > 0 || loops <= 0) {
			cpymo_charas_play_anime(
				engine,
				(float)cpymo_str_atoi(peroid_str) / 1000.0f,
				loops,
				buffer,
				offsets,
				true);

			cpymo_charas_set_play_anime(&engine->charas, cpymo_str_atoi(id_str));

			return CPYMO_ERR_SUCC;
		}
		else {
			free(buffer);
			printf(
				"[Error] chara_anime has invalid argument in script %s(%u)\n",
				interpreter->script->script_name,
				(unsigned)interpreter->script_parser.cur_line);
			CONT_NEXTLINE;
		}
	}

	D("scroll") {
		POP_ARG(filename); ENSURE(filename);
		POP_ARG(sx_str); ENSURE(sx_str);
		POP_ARG(sy_str); ENSURE(sy_str);
		POP_ARG(ex_str); ENSURE(ex_str);
		POP_ARG(ey_str); ENSURE(ey_str);
		POP_ARG(time_str); ENSURE(time_str);

		cpymo_album_cg_unlock(engine, filename);

		float sx = (float)cpymo_str_atoi(sx_str);
		float sy = (float)cpymo_str_atoi(sy_str);
		float ex = (float)cpymo_str_atoi(ex_str);
		float ey = (float)cpymo_str_atoi(ey_str);
		float time = (float)cpymo_str_atoi(time_str) / 1000.0f;

		return cpymo_scroll_start(engine, filename, sx, sy, ex, ey, time);
	}

	D("chara_y") {
		int chara_ids[CHARA_BUF_SIZE];
		int layers[CHARA_BUF_SIZE];
		float pos_x_s[CHARA_BUF_SIZE];
		float pos_y_s[CHARA_BUF_SIZE];
		cpymo_str filenames[CHARA_BUF_SIZE];
		size_t command_buffer_size = 0;

		POP_ARG(coord_mode_str); ENSURE(coord_mode_str);
		int coord_mode = cpymo_str_atoi(coord_mode_str);

		float time = 0.3f;
		while (true) {
			POP_ARG(chara_id_or_time_str);
			POP_ARG(filename);
			POP_ARG(pos_x_str);
			POP_ARG(pos_y_str);
			POP_ARG(layer_str);

			int chara_id_or_time = cpymo_str_atoi(chara_id_or_time_str);

			if (IS_EMPTY(filename) && IS_EMPTY(pos_x_str) && IS_EMPTY(pos_y_str) && IS_EMPTY(layer_str)) {
				time = (float)chara_id_or_time / 1000.0f;
				break;
			}

			int chara_id = chara_id_or_time;

			ENSURE(filename);
			ENSURE(pos_x_str);
			ENSURE(pos_y_str);
			ENSURE(layer_str);

			if (command_buffer_size >= CHARA_BUF_SIZE) {
				printf("[Warning] chara command buffer was overflow.\n");
			}
			else {
				chara_ids[command_buffer_size] = chara_id;
				layers[command_buffer_size] = cpymo_str_atoi(layer_str);
				POS(pos_x, pos_y, pos_x_str, pos_y_str);
				pos_x_s[command_buffer_size] = pos_x;
				pos_y_s[command_buffer_size] = pos_y;
				filenames[command_buffer_size] = filename;
				command_buffer_size++;
			}
		}

		for (size_t i = 0; i < command_buffer_size; ++i) {
			if (cpymo_str_equals_str(filenames[i], "NULL")) {
				cpymo_charas_kill(engine, chara_ids[i], time);
			}
			else {
				struct cpymo_chara *ch;
				err = cpymo_charas_new_chara(
					engine,
					&ch,
					filenames[i],
					chara_ids[i],
					layers[i],
					coord_mode,
					pos_x_s[i],
					pos_y_s[i],
					0,
					time);
				CPYMO_THROW(err);
			}
		}

		cpymo_charas_wait(engine);

		return CPYMO_ERR_SUCC;
	}

	D("chara_scroll") {
		POP_ARG(coord_mode_str); ENSURE(coord_mode_str);
		POP_ARG(chara_id_str); ENSURE(chara_id_str);
		POP_ARG(filename_or_endx); ENSURE(filename_or_endx);
		POP_ARG(startx_str_or_endy); ENSURE(startx_str_or_endy);
		POP_ARG(starty_str_or_time); ENSURE(starty_str_or_time);
		POP_ARG(endx_str);

		int coord_mode = cpymo_str_atoi(coord_mode_str);
		int chara_id = cpymo_str_atoi(chara_id_str);

		if (!IS_EMPTY(endx_str)) {

			POP_ARG(endy_str); ENSURE(endy_str);
			POP_ARG(begin_alpha_str); ENSURE(begin_alpha_str);
			POP_ARG(layer_str); ENSURE(layer_str);
			POP_ARG(time_str); ENSURE(time_str);

			POS(startx, starty, startx_str_or_endy, starty_str_or_time);
			POS(endx, endy, endx_str, endy_str);
			int layer = cpymo_str_atoi(layer_str);
			float begin_alpha = 1.0f - (float)cpymo_str_atoi(begin_alpha_str) / 255.0f;
			float time = (float)cpymo_str_atoi(time_str) / 1000.0f;

			struct cpymo_chara *c = NULL;
			err = cpymo_charas_new_chara(
				engine,
				&c,
				filename_or_endx,
				chara_id,
				layer,
				coord_mode,
				startx, starty,
				begin_alpha,
				time);
			CPYMO_THROW(err);

			err = cpymo_chara_convert_to_mode0_pos(engine, c, coord_mode, &endx, &endy);
			CPYMO_THROW(err);

			cpymo_tween_to(&c->pos_x, endx, time);
			cpymo_tween_to(&c->pos_y, endy, time);
		}
		else {
			POS(endx, endy, filename_or_endx, startx_str_or_endy);
			float time = (float)cpymo_str_atoi(starty_str_or_time) / 1000.0f;

			struct cpymo_chara *c = NULL;
			err = cpymo_charas_find(
				&engine->charas,
				&c,
				chara_id);
			if (err == CPYMO_ERR_NOT_FOUND) {
				printf("[Error] Character %d not found.\n", chara_id);
				return CPYMO_ERR_SUCC;
			}
			else { CPYMO_THROW(err); }

			err = cpymo_chara_convert_to_mode0_pos(engine, c, coord_mode, &endx, &endy);
			CPYMO_THROW(err);

			cpymo_tween_to(&c->pos_x, endx, time);
			cpymo_tween_to(&c->pos_y, endy, time);
		}

		cpymo_charas_wait(engine);

		return CPYMO_ERR_SUCC;
	}

	D("anime_on") {
#ifdef LOW_FRAME_RATE
		CONT_NEXTLINE;
#endif
		POP_ARG(frames_str); ENSURE(frames_str);
		POP_ARG(filename); ENSURE(filename);
		POP_ARG(x_str); ENSURE(x_str);
		POP_ARG(y_str); ENSURE(y_str);
		POP_ARG(interval_str); ENSURE(interval_str);
		POP_ARG(is_loop_s); ENSURE(is_loop_s);

		int frames = cpymo_str_atoi(frames_str);
		
		float interval = cpymo_str_atoi(interval_str) / 1000.0f;
		bool is_loop = cpymo_str_atoi(is_loop_s) != 0;

		POS(x, y, x_str, y_str);

		err = cpymo_anime_on(engine, frames, filename, x, y, interval, is_loop);
		if (err != CPYMO_ERR_SUCC) {
			char anime_name[16];
			cpymo_str_copy(anime_name, sizeof(anime_name), filename);
			printf("[Warning] Can not load anime %s in script %s(%u).\n", 
				anime_name, interpreter->script->script_name, (unsigned)interpreter->script_parser.cur_line);
		}

		CONT_NEXTLINE;
	}
	
	D("anime_off") {
#ifdef LOW_FRAME_RATE
		CONT_NEXTLINE;
#endif
		cpymo_anime_off(&engine->anime);
		CONT_NEXTLINE;
	}

	/*** III. Variables, Selection, Jump ***/
	D("set") {
		POP_ARG(name); ENSURE(name);
		POP_ARG(value_str); ENSURE(value_str);

		err = cpymo_vars_set(
			&engine->vars, name, 
			cpymo_vars_eval(&engine->vars, value_str));
		CPYMO_THROW(err);
			
		CONT_NEXTLINE;
	}

	D("add") {
		POP_ARG(name); ENSURE(name);
		POP_ARG(value); ENSURE(value);

		err = cpymo_vars_add(&engine->vars, name, 
			cpymo_vars_eval(&engine->vars, value));
		CPYMO_THROW(err);

		CONT_NEXTLINE;
	}

	D("sub") {
		POP_ARG(name); ENSURE(name);
		POP_ARG(value); ENSURE(value);

		err = cpymo_vars_add(&engine->vars, name, 
			-cpymo_vars_eval(&engine->vars, value));
		CPYMO_THROW(err);

		CONT_NEXTLINE;
	}

	D("label") {
		CONT_NEXTLINE;
	}

	D("goto") {
		POP_ARG(label);
		ENSURE(label);
		err = cpymo_interpreter_goto_label(interpreter, label);
		CPYMO_THROW(err);
		
		CONT_WITH_CURRENT_CONTEXT;
	}

	D("change") {
		POP_ARG(script_name);
		ENSURE(script_name);

		bool own_script = interpreter->own_script;
		interpreter->own_script = false;
		cpymo_script *script = interpreter->script;
		cpymo_interpreter *caller = interpreter->caller;
		interpreter->caller = NULL;
		cpymo_interpreter_free(interpreter);
		err = cpymo_interpreter_init_script(
			interpreter, script_name, &engine->assetloader, caller);
		if (own_script) cpymo_script_free(script);
		CPYMO_THROW(err);

		CONT_WITH_CURRENT_CONTEXT;
	}

	D("if") {
		POP_ARG(condition); ENSURE(condition);

		cpymo_parser parser;
		cpymo_parser_init(&parser, condition.begin, condition.len);

		cpymo_str left;
		left.begin = condition.begin;
		left.len = 0;

		while (left.len < condition.len) {
			char ch = left.begin[left.len];
			if (ch == '>' || ch == '<' || ch == '=' || ch == '!') 
				break;
			left.len++;
		}

		if (left.len >= condition.len) goto BAD_EXPRESSION;

		cpymo_str op;
		op.begin = left.begin + left.len;
		op.len = 1;

		if (op.begin[0] == '!') {
			op.len++;
			if (op.len + left.len >= condition.len) goto BAD_EXPRESSION;
			if (op.begin[1] != '=') goto BAD_EXPRESSION;
		}

		if ((op.begin[0] == '>' || op.begin[0] == '<')) {
			if (op.len + 1 + left.len < condition.len) {
				if (op.begin[1] == '=')
					op.len++;
			}
		}

		if ((op.begin[0] == '<')) {
			if (op.len + 1 + left.len < condition.len) {
				if (op.begin[1] == '>')
					op.len++;
			}
		}

		cpymo_str right;
		right.begin = op.begin + op.len;
		right.len = condition.len - op.len - left.len;

		cpymo_str_trim(&left);
		cpymo_str_trim(&op);
		cpymo_str_trim(&right);

		if (IS_EMPTY(left) || IS_EMPTY(right) || IS_EMPTY(op)) 
			goto BAD_EXPRESSION;

		const int lv = cpymo_vars_eval(&engine->vars, left);

		int rv;
		if (cpymo_vars_is_constant(right)) {
			rv = cpymo_str_atoi(right);
		}
		else {
			const cpymo_val *var = cpymo_vars_access(&engine->vars, right);
			if (var == NULL) { CONT_NEXTLINE; }
			else {
				rv = *var;
			}
		}

		bool run_sub_command;
		if (cpymo_str_equals_str(op, "="))
			run_sub_command = lv == rv;
		else if (cpymo_str_equals_str(op, "!=") || cpymo_str_equals_str(op, "<>"))
			run_sub_command = lv != rv;
		else if (cpymo_str_equals_str(op, ">"))
			run_sub_command = lv > rv;
		else if (cpymo_str_equals_str(op, ">="))
			run_sub_command = lv >= rv;
		else if (cpymo_str_equals_str(op, "<"))
			run_sub_command = lv < rv;
		else if (cpymo_str_equals_str(op, "<="))
			run_sub_command = lv <= rv;
		else goto BAD_EXPRESSION;

		if (run_sub_command) {
			while (!interpreter->script_parser.is_line_end) {
				// Skip blanks
				char ch = cpymo_parser_curline_peek(&interpreter->script_parser);
				if (ch == ' ' || ch == '\t')
					cpymo_parser_curline_readchar(&interpreter->script_parser);
				else break;
			}

			cpymo_str sub_command =
				cpymo_parser_curline_readuntil_or(&interpreter->script_parser, ' ', '\t');

			cpymo_str_trim(&sub_command);
			return cpymo_interpreter_dispatch(sub_command, interpreter, engine, cont);
		}
		

		CONT_NEXTLINE;

		BAD_EXPRESSION: {
			char *condition_str = cpymo_str_copy_malloc(condition);
			if (condition_str == NULL) return CPYMO_ERR_OUT_OF_MEM;
			printf( 
				"[Error] Bad if expression \"%s\" in script %s(%u).\n", 
				condition_str,
				interpreter->script->script_name,
				(unsigned)interpreter->script_parser.cur_line);
			free(condition_str);
			return CPYMO_ERR_INVALID_ARG;
		}
	}

	D("call") {
		POP_ARG(script_name);
		ENSURE(script_name);

		cpymo_interpreter *callee = 
			(cpymo_interpreter *)malloc(sizeof(cpymo_interpreter));
		if (callee == NULL) return CPYMO_ERR_OUT_OF_MEM;

		err = cpymo_interpreter_init_script(
			callee, script_name, &engine->assetloader, interpreter);

		if (err != CPYMO_ERR_SUCC) {
			free(callee);
			return err;
		}

		assert(engine->interpreter == interpreter);

		engine->interpreter = callee;

		longjmp(cont, CPYMO_EXEC_CONTVAL_INTERPRETER_UPDATED);
		return CPYMO_ERR_UNKNOWN;
	}

	D("ret") {
		if (interpreter->caller == NULL) return CPYMO_ERR_NO_MORE_CONTENT;

		assert(engine->interpreter == interpreter);

		// cpymo_interpreter *caller = interpreter->caller;

		engine->interpreter = interpreter->caller;
		if (interpreter->own_script)
			cpymo_script_free(interpreter->script);
		free(interpreter);

		longjmp(cont, CPYMO_EXEC_CONTVAL_INTERPRETER_UPDATED);
		return CPYMO_ERR_UNKNOWN;
	}

	D("sel") {
		cpymo_interpreter_checkpoint(interpreter);

		POP_ARG(choices_str); ENSURE(choices_str);
		int choices = cpymo_str_atoi(choices_str);

		POP_ARG(hint_pic);

		error_t err =
			cpymo_select_img_configuare_begin(
				&engine->select_img,
				(size_t)choices,
				cpymo_str_pure(""),
				&engine->assetloader,
				&engine->gameconfig);
		CPYMO_THROW(err);

		if (!(IS_EMPTY(hint_pic))) 
			cpymo_select_img_configuare_select_text_hint_pic(engine, hint_pic);
		
		for (int i = 0; i < choices; ++i) {
			cpymo_parser_next_line(&interpreter->script_parser);
			cpymo_str text =
				cpymo_parser_curline_readuntil(&interpreter->script_parser, '\n');

			uint64_t hash;
			cpymo_str_hash_init(&hash);
			cpymo_str_hash_append_cstr(&hash, "SEL: ");
			cpymo_str_hash_append_cstr(
				&hash, interpreter->script->script_name);
			cpymo_str_hash_append_cstr(&hash, "/");

			char buf[32];
			sprintf(buf, "%u", (unsigned)interpreter->script_parser.cur_line);
			cpymo_str_hash_append_cstr(&hash, buf);
			cpymo_str_hash_append_cstr(&hash, "/");
			sprintf(buf, "%d", i);
			cpymo_str_hash_append_cstr(&hash, buf);
			cpymo_str_hash_append_cstr(&hash, "/");
			cpymo_str_hash_append(&hash, text);

			enum cpymo_select_img_selection_hint_state hint_mode = cpymo_select_img_selection_nohint;
			if (!(IS_EMPTY(hint_pic))) {
				uint32_t first_char = cpymo_str_utf8_try_head_to_utf32(&text);

				if (first_char == 0x25cb) hint_mode = cpymo_select_img_selection_hint01;
				else if (first_char == 0x00D7) hint_mode = cpymo_select_img_selection_hint23;
			}
			
			err = cpymo_select_img_configuare_select_text(
				&engine->select_img, &engine->assetloader, &engine->gameconfig, &engine->flags, 
				text, true, hint_mode, hash, cpymo_gameconfig_font_size(&engine->gameconfig));

			CPYMO_THROW(err);
		}

		cpymo_select_img_configuare_end_select_text(
			&engine->select_img,
			&engine->wait,
			engine,
			0, 0,
			engine->gameconfig.imagesize_w,
			engine->gameconfig.imagesize_h / 2.0f,
			cpymo_color_white,
			-1,
			true);
		return CPYMO_ERR_SUCC;
	}

	D("select_text") { 
		cpymo_interpreter_checkpoint(interpreter);

		POP_ARG(choices_str); ENSURE(choices_str); 
		const int choices = cpymo_str_atoi(choices_str); 
		error_t err = cpymo_select_img_configuare_begin(
			&engine->select_img, (size_t)choices, cpymo_str_pure(""),
			&engine->assetloader, &engine->gameconfig); 
		CPYMO_THROW(err); 
		
		for (int i = 0; i < choices; ++i) { 
			POP_ARG(text); ENSURE(text); 
			uint64_t hash;
			cpymo_str_hash_init(&hash);
			cpymo_str_hash_append_cstr(&hash, "SELECT_TEXT: ");
			cpymo_str_hash_append_cstr(
				&hash, interpreter->script->script_name);
			cpymo_str_hash_append_cstr(&hash, "/");

			char buf[32];
			sprintf(buf, "%u", (unsigned)interpreter->script_parser.cur_line);
			cpymo_str_hash_append_cstr(&hash, buf);
			cpymo_str_hash_append_cstr(&hash, "/");
			sprintf(buf, "%d", i);
			cpymo_str_hash_append_cstr(&hash, buf);
			cpymo_str_hash_append_cstr(&hash, "/");
			cpymo_str_hash_append(&hash, text);

			err = cpymo_select_img_configuare_select_text(
				&engine->select_img, &engine->assetloader, 
				&engine->gameconfig, &engine->flags,
				text, true, cpymo_select_img_selection_nohint, hash, 
				cpymo_gameconfig_font_size(&engine->gameconfig));

			CPYMO_THROW(err); 
		} 
		
		POP_ARG(x1_str); ENSURE(x1_str); 
		POP_ARG(y1_str); ENSURE(y1_str); 
		POP_ARG(x2_str); ENSURE(x2_str); 
		POP_ARG(y2_str); ENSURE(y2_str); 
		POS(x1, y1, x1_str, y1_str); 
		POS(x2, y2, x2_str, y2_str); 
		POP_ARG(col); ENSURE(col); 
		POP_ARG(init_pos); ENSURE(init_pos); 
		
		cpymo_select_img_configuare_end_select_text( 
			&engine->select_img, &engine->wait, engine, x1, y1, x2, y2,  
			cpymo_str_as_color(col), 
			cpymo_str_atoi(init_pos),
			false); 
		
		return CPYMO_ERR_SUCC;
	}

	D("select_var") {
		cpymo_interpreter_checkpoint(interpreter);

		POP_ARG(choices_str); ENSURE(choices_str);
		const int choices = cpymo_str_atoi(choices_str);
		error_t err = cpymo_select_img_configuare_begin(
			&engine->select_img, (size_t)choices, cpymo_str_pure(""),
			&engine->assetloader, &engine->gameconfig);
		CPYMO_THROW(err);

		for (int i = 0; i < choices; ++i) {
			POP_ARG(text); ENSURE(text);
			POP_ARG(expr); ENSURE(expr);

			uint64_t hash;
			cpymo_str_hash_init(&hash);
			cpymo_str_hash_append_cstr(&hash, "SELECT_VAR: ");
			cpymo_str_hash_append_cstr(
				&hash, interpreter->script->script_name);
			cpymo_str_hash_append_cstr(&hash, "/");
			char buf[32];
			sprintf(buf, "%u", (unsigned)interpreter->script_parser.cur_line);
			cpymo_str_hash_append_cstr(&hash, buf);
			cpymo_str_hash_append_cstr(&hash, "/");
			sprintf(buf, "%d", i);
			cpymo_str_hash_append_cstr(&hash, buf);
			cpymo_str_hash_append_cstr(&hash, "/");
			cpymo_str_hash_append(&hash, text);

			err = cpymo_select_img_configuare_select_text(
				&engine->select_img, &engine->assetloader, &engine->gameconfig, &engine->flags,
				text, cpymo_vars_eval(&engine->vars, expr) != 0, cpymo_select_img_selection_nohint, hash,
				cpymo_gameconfig_font_size(&engine->gameconfig));
			CPYMO_THROW(err);
		}

		POP_ARG(x1_str); ENSURE(x1_str);
		POP_ARG(y1_str); ENSURE(y1_str);
		POP_ARG(x2_str); ENSURE(x2_str);
		POP_ARG(y2_str); ENSURE(y2_str);
		POS(x1, y1, x1_str, y1_str);
		POS(x2, y2, x2_str, y2_str);
		POP_ARG(col); ENSURE(col);
		POP_ARG(init_pos); ENSURE(init_pos);

		cpymo_select_img_configuare_end_select_text(
			&engine->select_img, &engine->wait, engine, x1, y1, x2, y2,
			cpymo_str_as_color(col),
			cpymo_str_atoi(init_pos),
			false);

		return CPYMO_ERR_SUCC;
	}

	D("select_img") {
		cpymo_interpreter_checkpoint(interpreter);

		POP_ARG(choices_str); ENSURE(choices_str);
		POP_ARG(filename); ENSURE(filename);

		size_t choices = (size_t)cpymo_str_atoi(choices_str);
		if (choices) {
			error_t err = cpymo_select_img_configuare_begin(
				&engine->select_img, choices, filename, 
				&engine->assetloader, &engine->gameconfig);
			if (err != CPYMO_ERR_SUCC) return err;

			for (size_t i = 0; i < choices; ++i) {
				POP_ARG(x_str); ENSURE(x_str);
				POP_ARG(y_str); ENSURE(y_str);
				POP_ARG(v_str); ENSURE(v_str);
				const bool enabled = cpymo_vars_eval(&engine->vars, v_str) != 0;
				POS(x, y, x_str, y_str);

				uint64_t hash;
				cpymo_str_hash_init(&hash);
				cpymo_str_hash_append_cstr(&hash, "SELECT_IMG: ");
				cpymo_str_hash_append_cstr(
					&hash, interpreter->script->script_name);
				cpymo_str_hash_append_cstr(&hash, "/");
				char buf[32];
				sprintf(buf, "%u", (unsigned)interpreter->script_parser.cur_line);
				cpymo_str_hash_append_cstr(&hash, buf);
				cpymo_str_hash_append_cstr(&hash, "/");
				sprintf(buf, "%d", (int)i);
				cpymo_str_hash_append_cstr(&hash, buf);
				cpymo_str_hash_append_cstr(&hash, "/");
				cpymo_str_hash_append(&hash, filename);

				cpymo_select_img_configuare_select_img_selection(engine, x, y, enabled, hash);
			}

			POP_ARG(init_position);
			int init_position_i = cpymo_str_atoi(init_position);

			cpymo_select_img_configuare_end(&engine->select_img, &engine->wait, engine, init_position_i);
		}
		else return CPYMO_ERR_INVALID_ARG;

		return CPYMO_ERR_SUCC;
	}

	D("select_imgs") {
		cpymo_interpreter_checkpoint(interpreter);

		POP_ARG(choices_str); ENSURE(choices_str);

		size_t choices = (size_t)cpymo_str_atoi(choices_str);
		if (choices) {
			error_t err = cpymo_select_img_configuare_begin(
				&engine->select_img, choices, cpymo_str_pure(""),
				&engine->assetloader, &engine->gameconfig);
			if (err != CPYMO_ERR_SUCC) return err;

			for (size_t i = 0; i < choices; ++i) {
				POP_ARG(filename); ENSURE(filename);
				POP_ARG(x_str); ENSURE(x_str);
				POP_ARG(y_str); ENSURE(y_str);
				POP_ARG(v_str); ENSURE(v_str);
				const bool enabled = cpymo_vars_eval(&engine->vars, v_str);
				POS(x, y, x_str, y_str);

				uint64_t hash;
				cpymo_str_hash_init(&hash);
				cpymo_str_hash_append_cstr(&hash, "SELECT_IMGS: ");
				cpymo_str_hash_append_cstr(&hash, interpreter->script->script_name);
				cpymo_str_hash_append_cstr(&hash, "/");
				
				char buf[32];
				sprintf(buf, "%u", (unsigned)interpreter->script_parser.cur_line);
				cpymo_str_hash_append_cstr(&hash, buf);
				cpymo_str_hash_append_cstr(&hash, "/");
				sprintf(buf, "%d", (int)i);
				cpymo_str_hash_append_cstr(&hash, buf);
				cpymo_str_hash_append_cstr(&hash, "/");
				cpymo_str_hash_append(&hash, filename);


				cpymo_select_img_configuare_select_imgs_selection(engine, filename, x, y, enabled, hash);
			}

			POP_ARG(init_position);
			int init_position_i = cpymo_str_atoi(init_position);

			cpymo_select_img_configuare_end(&engine->select_img, &engine->wait, engine, init_position_i);
		}
		else return CPYMO_ERR_INVALID_ARG;

		return CPYMO_ERR_SUCC;
	}
	
	D("wait") {
		POP_ARG(wait_ms_str);
		ENSURE(wait_ms_str);

		cpymo_engine_request_redraw(engine);
		float wait_sec = (float)cpymo_str_atoi(wait_ms_str) / 1000.0f;
		cpymo_wait_for_seconds(&engine->wait, wait_sec);
		return CPYMO_ERR_SUCC;
	}

	D("wait_se") {
		if (cpymo_audio_enabled(engine)) {
			cpymo_wait_register(&engine->wait, &cpymo_audio_wait_se);
			return CPYMO_ERR_SUCC;
		}
		else {
			CONT_NEXTLINE;
		}
	}

	D("rand") {
		POP_ARG(var_name); ENSURE(var_name);
		POP_ARG(min_val_str); ENSURE(min_val_str);
		POP_ARG(max_val_str); ENSURE(max_val_str);

		int min_val = cpymo_str_atoi(min_val_str);
		int max_val = cpymo_str_atoi(max_val_str);

		if (max_val - min_val <= 0) {
			printf(
				"[Error] In script %s(%u), max value must bigger than min value for rand command.\n",
				interpreter->script->script_name,
				(unsigned)interpreter->script_parser.cur_line);

			return CPYMO_ERR_INVALID_ARG;
		}

		err = cpymo_vars_set(&engine->vars, var_name, min_val + rand() % (max_val - min_val + 1));
		CPYMO_THROW(err);

		CONT_NEXTLINE;
	}

	/*** IV. Audio ***/
	D("bgm") {
		POP_ARG(filename); ENSURE(filename);
		POP_ARG(isloop_s);

		bool isloop = !cpymo_str_equals_str(isloop_s, "0");

		error_t err = cpymo_audio_bgm_play(engine, filename, isloop);
		if (err == CPYMO_ERR_OUT_OF_MEM) {
			cpymo_engine_trim_memory(engine);
			err = cpymo_audio_bgm_play(engine, filename, isloop);
		}

		CPYMO_THROW(err);
		

		CONT_NEXTLINE;
	}

	D("bgm_stop") {
		cpymo_audio_bgm_stop(engine);
		CONT_NEXTLINE;
	}

	D("se") {
		POP_ARG(filename); ENSURE(filename);
		POP_ARG(isloop_s);

		bool isloop = false;
		if (cpymo_str_equals_str(isloop_s, "1"))
			isloop = true;

		cpymo_audio_se_stop(engine);

		float vol = cpymo_audio_get_channel_volume(
			CPYMO_AUDIO_CHANNEL_VO, &engine->audio);

		if (isloop || (!cpymo_engine_skipping(engine) && vol > 0)) {
			error_t err = cpymo_audio_se_play(engine, filename, isloop);
			CPYMO_THROW(err);
		}

		CONT_NEXTLINE;
	}

	D("se_stop") {
		cpymo_audio_se_stop(engine);
		CONT_NEXTLINE;
	}

	D("vo") {
		POP_ARG(filename); ENSURE(filename);

		if (!cpymo_engine_skipping(engine)) {
			float vol = cpymo_audio_get_channel_volume(
				CPYMO_AUDIO_CHANNEL_VO, &engine->audio);

			if (vol > 0) {
				error_t err = cpymo_audio_vo_play(engine, filename);
				CPYMO_THROW(err);
			}
		}
		else {
			cpymo_audio_vo_stop(engine);
		}

		cpymo_backlog_record_write_vo(&engine->backlog, filename);

		CONT_NEXTLINE;
	}

	/*** V. System ***/
	D("load") {
		POP_ARG(save_id_x);

		if (IS_EMPTY(save_id_x)) {
			return cpymo_save_ui_enter(engine, true);
		}
		else {
			unsigned short save_id = (unsigned short)cpymo_str_atoi(save_id_x);
			FILE *file = cpymo_save_open_read(engine, save_id);
			if (file) {
				error_t err = cpymo_save_load_savedata(engine, file);
				if (err != CPYMO_ERR_SUCC) {
					printf("[Error] Bad save data file: %s\n", cpymo_error_message(err));
					return CPYMO_ERR_NO_MORE_CONTENT;
				}

				longjmp(cont, CPYMO_EXEC_CONTVAL_INTERPRETER_UPDATED);
				return CPYMO_ERR_UNKNOWN;
			}
			else {
				CONT_NEXTLINE;
			}
		}
	}

	D("album") {
		POP_ARG(list_name);

		cpymo_str ui_name;

		if (IS_EMPTY(list_name)) {
			list_name = cpymo_str_pure("album_list");
			ui_name = cpymo_str_pure("albumbg");
		} 
		else {
			ui_name = list_name;
		}

		return cpymo_album_enter(engine, list_name, ui_name, 0);
	}

	D("music") {
		return cpymo_music_box_enter(engine);
	}

	D("date") {
		int fmonth = cpymo_vars_get(&engine->vars, cpymo_str_pure("FMONTH"));
		int fdate = cpymo_vars_get(&engine->vars, cpymo_str_pure("FDATE"));
		char *str = NULL;

		const cpymo_localization *l = cpymo_localization_get(engine);
		error_t err = l->date_str(&str, fmonth, fdate);
		CPYMO_THROW(err);
				

		POP_ARG(date_bg); ENSURE(date_bg);
		POP_ARG(x_str); ENSURE(x_str);
		POP_ARG(y_str); ENSURE(y_str);
		POP_ARG(col_str);

		POS(x, y, x_str, y_str);

		cpymo_color col =
			cpymo_str_as_color(col_str);

		err = cpymo_floating_hint_start(
			engine,
			cpymo_str_pure(str),
			date_bg,
			x, y, col, 1.5f);

		free(str);
		return err;
	}

	D("config") {
		return cpymo_config_ui_enter(engine);
	}

#if CPYMO_FEATURE_LEVEL >= 1
	/* CPyMO HD Feature Level I */
	D_FEATURE_LEVEL(1, "lua") {
		cpymo_str lua_code = 
			cpymo_parser_curline_readuntil(&interpreter->script_parser, '\0');

		cpymo_str_trim(&lua_code);
		error_t err = cpymo_lua_context_push_lua_code(&engine->lua, lua_code);
		CPYMO_THROW(err);

		err = cpymo_lua_context_execute(&engine->lua, 0, 0);
		return err;
	}
#endif
	
	else {
		char buf[32];
		cpymo_str_copy(buf, 32, command);

		printf(
			"[Warning] Unknown command \"%s\" in script %s(%u).\n",
			buf,
			interpreter->script->script_name,
			(unsigned)interpreter->script_parser.cur_line + 1);

		if (engine->redraw) {
			return CPYMO_ERR_SUCC;
		}
		else {
			CONT_NEXTLINE;
		}
	}
}
