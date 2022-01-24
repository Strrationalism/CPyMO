#include "cpymo_interpreter.h"
#include "cpymo_engine.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <setjmp.h>
#include <assert.h>
#include <ctype.h>

error_t cpymo_interpreter_init_boot(cpymo_interpreter * out, const char * start_script_name)
{
	const char *script_format =
		"#bg logo1\n"
		"#bg logo2\n"
		"#change %s";

	out->script_name[0] = '\0';
	out->caller = NULL;
	
	size_t script_len = strlen(script_format) + 64;
	out->script_content = (char *)malloc(script_len);

	if (out->script_content == NULL || strlen(start_script_name) >= 63)
		return CPYMO_ERR_OUT_OF_MEM;

	if (sprintf(out->script_content, script_format, start_script_name) < 0) {
		free(out->script_content);
		return CPYMO_ERR_UNKNOWN;
	}

	cpymo_parser_init(&out->script_parser, out->script_content, strlen(out->script_content));
	return CPYMO_ERR_SUCC;
}

error_t cpymo_interpreter_init_script(cpymo_interpreter * out, const char * script_name, const cpymo_assetloader *loader)
{
	if (strlen(script_name) >= 63) return CPYMO_ERR_OUT_OF_MEM;
	strcpy(out->script_name, script_name);

	out->caller = NULL;

	out->script_content = NULL;
	size_t script_len = 0;

	error_t err =
		cpymo_assetloader_load_script(&out->script_content, &script_len, script_name, loader);

	if (err != CPYMO_ERR_SUCC) return err;

	cpymo_parser_init(&out->script_parser, out->script_content, script_len);
	return CPYMO_ERR_SUCC;
}

error_t cpymo_interpreter_init_snapshot(cpymo_interpreter * out, const cpymo_interpreter_snapshot * snapshot, const cpymo_assetloader * loader)
{
	error_t err = cpymo_interpreter_init_script(out, snapshot->script_name, loader);
	if (err != CPYMO_ERR_SUCC) return err;

	return cpymo_interpreter_goto_line(out, snapshot->cur_line);
}

void cpymo_interpreter_free(cpymo_interpreter * interpreter)
{
	cpymo_interpreter *caller = interpreter->caller;
	while (caller) {
		cpymo_interpreter *to_free = caller;
		caller = caller->caller;

		free(to_free->script_content);
		free(to_free);
	}

	free(interpreter->script_content);
}

error_t cpymo_interpreter_goto_line(cpymo_interpreter * interpreter, uint64_t line)
{
	while (line != interpreter->script_parser.cur_line)
		if (!cpymo_parser_next_line(&interpreter->script_parser))
			return CPYMO_ERR_BAD_FILE_FORMAT;
	return CPYMO_ERR_SUCC;
}

error_t cpymo_interpreter_goto_label(cpymo_interpreter * interpreter, cpymo_parser_stream_span label)
{
	cpymo_parser_reset(&interpreter->script_parser);

	while (1) {
		cpymo_parser_stream_span command = 
			cpymo_parser_curline_pop_command(&interpreter->script_parser);

		if (cpymo_parser_stream_span_equals_str(command, "label")) {
			cpymo_parser_stream_span cur_label = 
				cpymo_parser_curline_pop_commacell(&interpreter->script_parser);

			if (cpymo_parser_stream_span_equals(cur_label, label)) {
				cpymo_parser_next_line(&interpreter->script_parser);
				return CPYMO_ERR_SUCC;
			}
		}
		else {
			if (!cpymo_parser_next_line(&interpreter->script_parser)) {
				char label_name[32];
				cpymo_parser_stream_span_copy(label_name, sizeof(label_name), label);
				fprintf(stderr, "[Error] Can not find label %s in script %s.", label_name, interpreter->script_name);
				return CPYMO_ERR_NOT_FOUND;
			}
		}
	}
}

cpymo_interpreter_snapshot cpymo_interpreter_get_snapshot_current_callstack(const cpymo_interpreter * interpreter)
{
	cpymo_interpreter_snapshot out;
	strcpy(out.script_name, interpreter->script_name);
	out.cur_line = interpreter->script_parser.cur_line;
	return out;
}

static error_t cpymo_interpreter_dispatch(cpymo_parser_stream_span command, cpymo_interpreter *interpreter, cpymo_engine *engine, jmp_buf cont);

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

	cpymo_parser_stream_span command =
		cpymo_parser_curline_pop_command(&interpreter->script_parser);

	error_t err = cpymo_interpreter_dispatch(command, interpreter, engine, cont);
	if (err != CPYMO_ERR_SUCC && err != CPYMO_ERR_INVALID_ARG) {
		cpymo_parser_next_line(&interpreter->script_parser);
		return err;
	}

	if (err == CPYMO_ERR_INVALID_ARG) {
		fprintf(stderr,
			"[Error] Invalid arguments in script %s(%d).", 
			interpreter->script_name, 
			(int)interpreter->script_parser.cur_line);
	}

	if (!cpymo_parser_next_line(&interpreter->script_parser))
		return CPYMO_ERR_NO_MORE_CONTENT;

	return CPYMO_ERR_SUCC;
}

#define D(CMD) \
	else if (cpymo_parser_stream_span_equals_str(command, CMD))

#define POP_ARG(X) \
	cpymo_parser_stream_span X = cpymo_parser_curline_pop_commacell(&interpreter->script_parser); \
	cpymo_parser_stream_span_trim(&X)

#define IS_EMPTY(X) \
	cpymo_parser_stream_span_equals_str(X, "")
	
#define ENSURE(X) \
	{ if (IS_EMPTY(X)) return CPYMO_ERR_INVALID_ARG; }

#define POS(OUT_X, OUT_Y, IN_X, IN_Y) \
	float OUT_X = cpymo_parser_stream_span_atof(IN_X) / 100.0f * engine->gameconfig.imagesize_w; \
	float OUT_Y = cpymo_parser_stream_span_atof(IN_Y) / 100.0f * engine->gameconfig.imagesize_h;

#define CONT_WITH_CURRENT_CONTEXT { longjmp(cont, CPYMO_EXEC_CONTVAL_OK); return CPYMO_ERR_UNKNOWN; }

#define CONT_NEXTLINE { \
	if (cpymo_parser_next_line(&interpreter->script_parser))	\
		{ longjmp(cont, CPYMO_EXEC_CONTVAL_OK); return CPYMO_ERR_UNKNOWN; }	\
	else return CPYMO_ERR_NO_MORE_CONTENT; }

static error_t cpymo_interpreter_dispatch(cpymo_parser_stream_span command, cpymo_interpreter *interpreter, cpymo_engine *engine, jmp_buf cont)
{
	error_t err;

	if (IS_EMPTY(command)) {
		CONT_NEXTLINE;
	}

	/*** I. Text ***/
	D("say") {
		POP_ARG(name_or_text); ENSURE(name_or_text);
		POP_ARG(text);

		if (IS_EMPTY(text)) {
			text = name_or_text;
			name_or_text.len = 0;
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
		cpymo_color col = cpymo_parser_stream_span_as_color(col_str);
		POP_ARG(fontsize_str); ENSURE(fontsize_str);
		float fontsize = 
			cpymo_parser_stream_span_atof(fontsize_str) * 
			engine->gameconfig.imagesize_h / 240.0f * 1.2f;
		POP_ARG(show_immediately_str);
		bool show_immediately = cpymo_parser_stream_span_atoi(show_immediately_str) != 0;

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

		char *buf = (char *)malloc(title.len + 1);
		if (buf == NULL) return CPYMO_ERR_OUT_OF_MEM;

		free(engine->title);
		engine->title = buf;
		cpymo_parser_stream_span_copy(engine->title, title.len + 1, title);
		
		CONT_NEXTLINE;
	}

	D("title_dsp") {
		if (strlen(engine->title) <= 0)
			CONT_NEXTLINE;

		return cpymo_floating_hint_start(
			engine,
			cpymo_parser_stream_span_pure(engine->title),
			cpymo_parser_stream_span_pure(""),
			2 * cpymo_gameconfig_font_size(&engine->gameconfig),
			cpymo_gameconfig_font_size(&engine->gameconfig),
			cpymo_color_white,
			1.0f);
	}

	/*** II. Video ***/
	D("chara") {
		int chara_ids[16];
		int layers[16];
		float pos_x_s[16];
		cpymo_parser_stream_span filenames[16];
		size_t command_buffer_size = 0;

		float time = 0.3f;
		while (true) {
			POP_ARG(chara_id_or_time_str);
			POP_ARG(filename);
			POP_ARG(pos_x_str);
			POP_ARG(layer_str);

			int chara_id_or_time = cpymo_parser_stream_span_atoi(chara_id_or_time_str);
			
			if (IS_EMPTY(filename) && IS_EMPTY(pos_x_str) && IS_EMPTY(layer_str)) {
				time = (float)chara_id_or_time / 1000.0f;
				break;
			}

			int chara_id = chara_id_or_time;

			ENSURE(filename);
			ENSURE(pos_x_str);
			ENSURE(layer_str);

			if (command_buffer_size >= 16) {
				fprintf(stderr, "[Warning] chara command buffer was overflow.");
			} 
			else {
				chara_ids[command_buffer_size] = chara_id;
				layers[command_buffer_size] = cpymo_parser_stream_span_atoi(layer_str);
				pos_x_s[command_buffer_size] =
					(float)cpymo_parser_stream_span_atoi(pos_x_str) / 100.0f * (float)engine->gameconfig.imagesize_w;
				filenames[command_buffer_size] = filename;
				command_buffer_size++;
			}
		}

		for (size_t i = 0; i < command_buffer_size; ++i) {
			if (cpymo_parser_stream_span_equals_str(filenames[i], "NULL")) {
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

		float time = IS_EMPTY(time_str) ? 0.3f : (float)cpymo_parser_stream_span_atoi(time_str) / 1000.0f;
		if (cpymo_parser_stream_span_equals_str(id_str, "a"))
			cpymo_charas_kill_all(engine, time);
		else cpymo_charas_kill(engine, cpymo_parser_stream_span_atoi(id_str), time);

		cpymo_charas_wait(engine);
		return CPYMO_ERR_SUCC;
	}

	D("chara_pos") {
		POP_ARG(id_str); ENSURE(id_str);
		POP_ARG(x_str); ENSURE(x_str);
		POP_ARG(y_str); ENSURE(y_str);
		POP_ARG(coord_mode_str);

		int id = cpymo_parser_stream_span_atoi(id_str);
		POS(x, y, x_str, y_str);

		int coord_mode = IS_EMPTY(coord_mode_str) ? 5 : cpymo_parser_stream_span_atoi(coord_mode_str);

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

		if (cpymo_parser_stream_span_equals_str(time_str, "BG_VERYFAST")) time = 0.1f;
		if (cpymo_parser_stream_span_equals_str(time_str, "BG_FAST")) time = 0.175f;
		else if (cpymo_parser_stream_span_equals_str(time_str, "BG_NORMAL")) time = 0.25f;
		else if (cpymo_parser_stream_span_equals_str(time_str, "BG_SLOW")) time = 0.5f;
		else if (cpymo_parser_stream_span_equals_str(time_str, "BG_VERYSLOW")) time = 1.0f;
		else if (!IS_EMPTY(time_str))
			time = (float)cpymo_parser_stream_span_atoi(time_str) / 1000.0f;
		else time = 0.3f;
		

		if (IS_EMPTY(x_str)) x = 0.0f; else x = (float)cpymo_parser_stream_span_atoi(x_str);
		if (IS_EMPTY(y_str)) y = 0.0f; else y = (float)cpymo_parser_stream_span_atoi(y_str);

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

		cpymo_color col = cpymo_parser_stream_span_as_color(col_str);
		float time = cpymo_parser_stream_span_atoi(time_str) / 1000.0f;

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

		cpymo_color col = cpymo_parser_stream_span_as_color(col_str);
		float time = cpymo_parser_stream_span_atoi(time_str) / 1000.0f;

		cpymo_fade_start_fadeout(engine, time, col);
		return CPYMO_ERR_SUCC;
	}

	D("fade_in") {
		POP_ARG(time_str); ENSURE(time_str);
		float time = cpymo_parser_stream_span_atoi(time_str) / 1000.0f;
		cpymo_fade_start_fadein(engine, time);
		return CPYMO_ERR_SUCC;
	}

	D("textbox") {
		POP_ARG(msg); ENSURE(msg);
		POP_ARG(name); ENSURE(name);

		cpymo_engine_request_redraw(engine);

		error_t err = cpymo_say_load_msgbox_image(&engine->say, msg, &engine->assetloader);
		CPYMO_THROW(err);

		err = cpymo_say_load_namebox_image(&engine->say, name, &engine->assetloader);
		return err;
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
				cpymo_charas_set_play_anime(&engine->charas, cpymo_parser_stream_span_atoi(id)); \
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

		int loops = cpymo_parser_stream_span_atoi(loop_str);

		float *buffer = (float *)malloc(64 * sizeof(float));
		if (buffer == NULL) return CPYMO_ERR_OUT_OF_MEM;
		size_t offsets = 0;

		while (offsets < 32) {
			POP_ARG(x_str);
			if (IS_EMPTY(x_str)) break;

			POP_ARG(y_str); 
			if (IS_EMPTY(y_str)) break;

			float x = (float)cpymo_parser_stream_span_atoi(x_str);
			float y = (float)cpymo_parser_stream_span_atoi(y_str);
			buffer[offsets * 2] = x;
			buffer[offsets * 2 + 1] = y;
			offsets++;
		}

		if (offsets > 0 || loops <= 0) {
			cpymo_charas_play_anime(
				engine,
				(float)cpymo_parser_stream_span_atoi(peroid_str) / 1000.0f,
				loops,
				buffer,
				offsets,
				true);

			cpymo_charas_set_play_anime(&engine->charas, cpymo_parser_stream_span_atoi(id_str));

			return CPYMO_ERR_SUCC;
		}
		else {
			free(buffer);
			fprintf(
				stderr,
				"[Error] chara_anime has invalid argument in script %s(%u)",
				interpreter->script_name,
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

		float sx = (float)cpymo_parser_stream_span_atoi(sx_str);
		float sy = (float)cpymo_parser_stream_span_atoi(sy_str);
		float ex = (float)cpymo_parser_stream_span_atoi(ex_str);
		float ey = (float)cpymo_parser_stream_span_atoi(ey_str);
		float time = (float)cpymo_parser_stream_span_atoi(time_str) / 1000.0f;

		return cpymo_scroll_start(engine, filename, sx, sy, ex, ey, time);
	}

	D("chara_y") {
		int chara_ids[16];
		int layers[16];
		float pos_x_s[16];
		float pos_y_s[16];
		cpymo_parser_stream_span filenames[16];
		size_t command_buffer_size = 0;

		POP_ARG(coord_mode_str); ENSURE(coord_mode_str);
		int coord_mode = cpymo_parser_stream_span_atoi(coord_mode_str);

		float time = 0.3f;
		while (true) {
			POP_ARG(chara_id_or_time_str);
			POP_ARG(filename);
			POP_ARG(pos_x_str);
			POP_ARG(pos_y_str);
			POP_ARG(layer_str);

			int chara_id_or_time = cpymo_parser_stream_span_atoi(chara_id_or_time_str);

			if (IS_EMPTY(filename) && IS_EMPTY(pos_x_str) && IS_EMPTY(pos_y_str) && IS_EMPTY(layer_str)) {
				time = (float)chara_id_or_time / 1000.0f;
				break;
			}

			int chara_id = chara_id_or_time;

			ENSURE(filename);
			ENSURE(pos_x_str);
			ENSURE(pos_y_str);
			ENSURE(layer_str);

			if (command_buffer_size >= 16) {
				fprintf(stderr, "[Warning] chara command buffer was overflow.");
			}
			else {
				chara_ids[command_buffer_size] = chara_id;
				layers[command_buffer_size] = cpymo_parser_stream_span_atoi(layer_str);
				POS(pos_x, pos_y, pos_x_str, pos_y_str);
				pos_x_s[command_buffer_size] = pos_x;
				pos_y_s[command_buffer_size] = pos_y;
				filenames[command_buffer_size] = filename;
				command_buffer_size++;
			}
		}

		for (size_t i = 0; i < command_buffer_size; ++i) {
			if (cpymo_parser_stream_span_equals_str(filenames[i], "NULL")) {
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

		int coord_mode = cpymo_parser_stream_span_atoi(coord_mode_str);
		int chara_id = cpymo_parser_stream_span_atoi(chara_id_str);

		if (!IS_EMPTY(endx_str)) {

			POP_ARG(endy_str); ENSURE(endy_str);
			POP_ARG(begin_alpha_str); ENSURE(begin_alpha_str);
			POP_ARG(layer_str); ENSURE(layer_str);
			POP_ARG(time_str); ENSURE(time_str);

			POS(startx, starty, startx_str_or_endy, starty_str_or_time);
			POS(endx, endy, endx_str, endy_str);
			int layer = cpymo_parser_stream_span_atoi(layer_str);
			float begin_alpha = 1.0f - (float)cpymo_parser_stream_span_atoi(begin_alpha_str) / 255.0f;
			float time = (float)cpymo_parser_stream_span_atoi(time_str) / 1000.0f;

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
			float time = (float)cpymo_parser_stream_span_atoi(starty_str_or_time) / 1000.0f;

			struct cpymo_chara *c = NULL;
			err = cpymo_charas_find(
				&engine->charas,
				&c,
				chara_id);
			if (err == CPYMO_ERR_NOT_FOUND) {
				fprintf(stderr, "[Error] Character %d not found.\n", chara_id);
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
		POP_ARG(frames_str); ENSURE(frames_str);
		POP_ARG(filename); ENSURE(filename);
		POP_ARG(x_str); ENSURE(x_str);
		POP_ARG(y_str); ENSURE(y_str);
		POP_ARG(interval_str); ENSURE(interval_str);
		POP_ARG(is_loop_s); ENSURE(is_loop_s);

		int frames = cpymo_parser_stream_span_atoi(frames_str);
		
		float interval = cpymo_parser_stream_span_atoi(interval_str) / 1000.0f;
		bool is_loop = cpymo_parser_stream_span_atoi(is_loop_s) != 0;

		POS(x, y, x_str, y_str);

		err = cpymo_anime_on(engine, frames, filename, x, y, interval, is_loop);
		if (err != CPYMO_ERR_SUCC) {
			char anime_name[16];
			cpymo_parser_stream_span_copy(anime_name, sizeof(anime_name), filename);
			fprintf(stderr, "[Warning] Can not load anime %s in script %s(%u).", 
				anime_name, interpreter->script_name, (unsigned)interpreter->script_parser.cur_line);
		}

		CONT_NEXTLINE;
	}
	
	D("anime_off") {
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

		int *v = NULL;
		err = cpymo_vars_access_create(&engine->vars, name, &v);
		CPYMO_THROW(err);

		*v += cpymo_vars_eval(&engine->vars, value);

		CONT_NEXTLINE;
	}

	D("sub") {
		POP_ARG(name); ENSURE(name);
		POP_ARG(value); ENSURE(value);

		int *v = NULL;
		err = cpymo_vars_access_create(&engine->vars, name, &v);
		CPYMO_THROW(err);

		*v -= cpymo_vars_eval(&engine->vars, value);

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
		POP_ARG(script_name_span);
		ENSURE(script_name_span);

		char script_name[sizeof(interpreter->script_name)];
		cpymo_parser_stream_span_copy(script_name, sizeof(script_name), script_name_span);

		cpymo_interpreter_free(interpreter);
		err = cpymo_interpreter_init_script(interpreter, script_name, &engine->assetloader);
		CPYMO_THROW(err);

		CONT_WITH_CURRENT_CONTEXT;
	}

	D("if") {
		POP_ARG(condition); ENSURE(condition);

		cpymo_parser parser;
		cpymo_parser_init(&parser, condition.begin, condition.len);

		cpymo_parser_stream_span left;
		left.begin = condition.begin;
		left.len = 0;

		while (left.len < condition.len) {
			char ch = left.begin[left.len];
			if (ch == '>' || ch == '<' || ch == '=' || ch == '!') 
				break;
			left.len++;
		}

		if (left.len >= condition.len) goto BAD_EXPRESSION;

		cpymo_parser_stream_span op;
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

		cpymo_parser_stream_span right;
		right.begin = op.begin + op.len;
		right.len = condition.len - op.len - left.len;

		cpymo_parser_stream_span_trim(&left);
		cpymo_parser_stream_span_trim(&op);
		cpymo_parser_stream_span_trim(&right);

		if (IS_EMPTY(left) || IS_EMPTY(right) || IS_EMPTY(op)) 
			goto BAD_EXPRESSION;

		const int lv = cpymo_vars_eval(&engine->vars, left);
		const int rv = cpymo_vars_eval(&engine->vars, right);

		bool run_sub_command;
		if (cpymo_parser_stream_span_equals_str(op, "="))
			run_sub_command = lv == rv;
		else if (cpymo_parser_stream_span_equals_str(op, "!=") || cpymo_parser_stream_span_equals_str(op, "<>"))
			run_sub_command = lv != rv;
		else if (cpymo_parser_stream_span_equals_str(op, ">"))
			run_sub_command = lv > rv;
		else if (cpymo_parser_stream_span_equals_str(op, ">="))
			run_sub_command = lv >= rv;
		else if (cpymo_parser_stream_span_equals_str(op, "<"))
			run_sub_command = lv < rv;
		else if (cpymo_parser_stream_span_equals_str(op, "<="))
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

			cpymo_parser_stream_span sub_command =
				cpymo_parser_curline_readuntil_or(&interpreter->script_parser, ' ', '\t');

			cpymo_parser_stream_span_trim(&sub_command);
			return cpymo_interpreter_dispatch(sub_command, interpreter, engine, cont);
		}
		

		CONT_NEXTLINE;

		BAD_EXPRESSION: {
			char *condition_str = (char *)malloc(condition.len + 1);
			if (condition_str == NULL) return CPYMO_ERR_OUT_OF_MEM;
			cpymo_parser_stream_span_copy(condition_str, condition.len + 1, condition);
			fprintf(
				stderr, 
				"[Error] Bad if expression \"%s\" in script %s(%u).", 
				condition_str,
				interpreter->script_name,
				(unsigned)interpreter->script_parser.cur_line);
			free(condition_str);
			return CPYMO_ERR_INVALID_ARG;
		}
	}

	D("call") {
		POP_ARG(script_name_span);
		ENSURE(script_name_span);
		//cpymo_parser_next_line(&interpreter->script_parser);

		char script_name[sizeof(interpreter->script_name)];
		cpymo_parser_stream_span_copy(script_name, sizeof(script_name), script_name_span);

		cpymo_interpreter *callee = (cpymo_interpreter *)malloc(sizeof(cpymo_interpreter));
		if (callee == NULL) return CPYMO_ERR_OUT_OF_MEM;

		if ((err = cpymo_interpreter_init_script(callee, script_name, &engine->assetloader)) != CPYMO_ERR_SUCC) {
			free(callee);
			return err;
		}

		assert(engine->interpreter == interpreter);

		engine->interpreter = callee;
		callee->caller = interpreter;

		longjmp(cont, CPYMO_EXEC_CONTVAL_INTERPRETER_UPDATED);
		return CPYMO_ERR_UNKNOWN;
	}

	D("ret") {
		if (interpreter->caller == NULL) return CPYMO_ERR_NO_MORE_CONTENT;

		assert(engine->interpreter == interpreter);

		// cpymo_interpreter *caller = interpreter->caller;

		engine->interpreter = interpreter->caller;
		free(interpreter->script_content);
		free(interpreter);

		longjmp(cont, CPYMO_EXEC_CONTVAL_INTERPRETER_UPDATED);
		return CPYMO_ERR_UNKNOWN;
	}

	D("sel") {
		POP_ARG(choices_str); ENSURE(choices_str);
		int choices = cpymo_parser_stream_span_atoi(choices_str);

		POP_ARG(hint_pic);

		error_t err =
			cpymo_select_img_configuare_begin(
				engine,
				(size_t)choices,
				cpymo_parser_stream_span_pure(""));
		CPYMO_THROW(err);

		if (!(IS_EMPTY(hint_pic))) 
			cpymo_select_img_configuare_select_text_hint_pic(engine, hint_pic);
		
		for (int i = 0; i < choices; ++i) {
			cpymo_parser_next_line(&interpreter->script_parser);
			cpymo_parser_stream_span text =
				cpymo_parser_curline_readuntil(&interpreter->script_parser, '\n');

			char hash_str[128];
			sprintf(hash_str, "SEL: %s/%u/%d/", 
				interpreter->script_name, 
				(unsigned)interpreter->script_parser.cur_line, 
				i);

			size_t len = strlen(hash_str);
			cpymo_parser_stream_span_copy(hash_str + len, sizeof(hash_str) - len - 2, text);

			uint64_t sel_hash = cpymo_parser_stream_span_hash(cpymo_parser_stream_span_pure(hash_str));

			int hint_mode = cpymo_select_img_selection_nohint;
			if (!(IS_EMPTY(hint_pic))) {
				uint32_t first_char = cpymo_parser_stream_span_utf8_try_head_to_utf32(&text);

				if (first_char == L'○') hint_mode = cpymo_select_img_selection_hint01;
				else if (first_char == L'×') hint_mode = cpymo_select_img_selection_hint23;
			}

			err = cpymo_select_img_configuare_select_text(engine, text, true, hint_mode, sel_hash);
			CPYMO_THROW(err);
		}

		cpymo_select_img_configuare_end_select_text(
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
		POP_ARG(choices_str); ENSURE(choices_str); 
		const int choices = cpymo_parser_stream_span_atoi(choices_str); 
		error_t err = cpymo_select_img_configuare_begin(engine, (size_t)choices, cpymo_parser_stream_span_pure("")); 
		CPYMO_THROW(err); 
		
		for (int i = 0; i < choices; ++i) { 
			POP_ARG(text); ENSURE(text); 
			char hash_str[128];
			sprintf(hash_str, "SELECT_TEXT: %s/%u/%d/",
				interpreter->script_name,
				(unsigned)interpreter->script_parser.cur_line,
				i);

			size_t len = strlen(hash_str);
			cpymo_parser_stream_span_copy(hash_str + len, sizeof(hash_str) - len - 2, text);

			uint64_t hash = cpymo_parser_stream_span_hash(cpymo_parser_stream_span_pure(hash_str));

			err = cpymo_select_img_configuare_select_text(engine, text, true, cpymo_select_img_selection_nohint, hash);
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
			engine, x1, y1, x2, y2,  
			cpymo_parser_stream_span_as_color(col), 
			cpymo_parser_stream_span_atoi(init_pos),
			false); 
		
		return CPYMO_ERR_SUCC;
	}

	D("select_var") {
		POP_ARG(choices_str); ENSURE(choices_str);
		const int choices = cpymo_parser_stream_span_atoi(choices_str);
		error_t err = cpymo_select_img_configuare_begin(engine, (size_t)choices, cpymo_parser_stream_span_pure(""));
		CPYMO_THROW(err);

		for (int i = 0; i < choices; ++i) {
			POP_ARG(text); ENSURE(text);
			POP_ARG(expr); ENSURE(expr);

			char hash_str[128];
			sprintf(hash_str, "SELECT_VAR: %s/%u/%d/",
				interpreter->script_name,
				(unsigned)interpreter->script_parser.cur_line,
				i);
			size_t len = strlen(hash_str);
			cpymo_parser_stream_span_copy(hash_str + len, sizeof(hash_str) - len - 2, text);

			uint64_t hash = cpymo_parser_stream_span_hash(cpymo_parser_stream_span_pure(hash_str));

			err = cpymo_select_img_configuare_select_text(
				engine, 
				text, 
				cpymo_vars_eval(&engine->vars, expr) != 0,
				cpymo_select_img_selection_nohint,
				hash);
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
			engine, x1, y1, x2, y2,
			cpymo_parser_stream_span_as_color(col),
			cpymo_parser_stream_span_atoi(init_pos),
			false);

		return CPYMO_ERR_SUCC;
	}

	D("select_img") {
		POP_ARG(choices_str); ENSURE(choices_str);
		POP_ARG(filename); ENSURE(filename);

		size_t choices = (size_t)cpymo_parser_stream_span_atoi(choices_str);
		if (choices) {
			error_t err = cpymo_select_img_configuare_begin(engine, choices, filename);
			if (err != CPYMO_ERR_SUCC) return err;

			for (size_t i = 0; i < choices; ++i) {
				POP_ARG(x_str); ENSURE(x_str);
				POP_ARG(y_str); ENSURE(y_str);
				POP_ARG(v_str); ENSURE(v_str);
				const bool enabled = cpymo_vars_eval(&engine->vars, v_str) != 0;
				POS(x, y, x_str, y_str);

				char hash_str[128];
				sprintf(hash_str, "SELECT_IMG: %s/%u/%d/",
					interpreter->script_name,
					(unsigned)interpreter->script_parser.cur_line,
					(int)i);
				size_t len = strlen(hash_str);
				cpymo_parser_stream_span_copy(hash_str + len, sizeof(hash_str) - len - 2, filename);
				uint64_t hash = cpymo_parser_stream_span_hash(cpymo_parser_stream_span_pure(hash_str));

				cpymo_select_img_configuare_select_img_selection(engine, x, y, enabled, hash);
			}

			POP_ARG(init_position);
			int init_position_i = cpymo_parser_stream_span_atoi(init_position);

			cpymo_select_img_configuare_end(engine, init_position_i);
		}
		else return CPYMO_ERR_INVALID_ARG;

		return CPYMO_ERR_SUCC;
	}

	D("select_imgs") {
		POP_ARG(choices_str); ENSURE(choices_str);

		size_t choices = (size_t)cpymo_parser_stream_span_atoi(choices_str);
		if (choices) {
			error_t err = cpymo_select_img_configuare_begin(engine, choices, cpymo_parser_stream_span_pure(""));
			if (err != CPYMO_ERR_SUCC) return err;

			for (size_t i = 0; i < choices; ++i) {
				POP_ARG(filename); ENSURE(filename);
				POP_ARG(x_str); ENSURE(x_str);
				POP_ARG(y_str); ENSURE(y_str);
				POP_ARG(v_str); ENSURE(v_str);
				const bool enabled = cpymo_vars_eval(&engine->vars, v_str);
				POS(x, y, x_str, y_str);

				char hash_str[128];
				sprintf(hash_str, "SELECT_IMGS: %s/%u/%d/",
					interpreter->script_name,
					(unsigned)interpreter->script_parser.cur_line,
					(int)i);

				size_t len = strlen(hash_str);
				cpymo_parser_stream_span_copy(hash_str + len, sizeof(hash_str) - len - 2, filename);

				uint64_t hash = cpymo_parser_stream_span_hash(cpymo_parser_stream_span_pure(hash_str));

				cpymo_select_img_configuare_select_imgs_selection(engine, filename, x, y, enabled, hash);
			}

			POP_ARG(init_position);
			int init_position_i = cpymo_parser_stream_span_atoi(init_position);

			cpymo_select_img_configuare_end(engine, init_position_i);
		}
		else return CPYMO_ERR_INVALID_ARG;

		return CPYMO_ERR_SUCC;
	}
	
	D("wait") {
		POP_ARG(wait_ms_str);
		ENSURE(wait_ms_str);

		cpymo_engine_request_redraw(engine);
		float wait_sec = (float)cpymo_parser_stream_span_atoi(wait_ms_str) / 1000.0f;
		cpymo_wait_for_seconds(&engine->wait, wait_sec);
		return CPYMO_ERR_SUCC;
	}

	D("rand") {
		POP_ARG(var_name); ENSURE(var_name);
		POP_ARG(min_val_str); ENSURE(min_val_str);
		POP_ARG(max_val_str); ENSURE(max_val_str);

		int min_val = cpymo_parser_stream_span_atoi(min_val_str);
		int max_val = cpymo_parser_stream_span_atoi(max_val_str);

		if (max_val - min_val <= 0) {
			fprintf(
				stderr,
				"[Error] In script %s(%u), max value must bigger than min value for rand command.",
				interpreter->script_name,
				(unsigned)interpreter->script_parser.cur_line);

			return CPYMO_ERR_INVALID_ARG;
		}

		err = cpymo_vars_set(&engine->vars, var_name, min_val + rand() % (max_val - min_val + 1));
		CPYMO_THROW(err);

		CONT_NEXTLINE;
	}

	/*** IV. System ***/
	D("date") {
		int fmonth = cpymo_vars_get(&engine->vars, cpymo_parser_stream_span_pure("FMONTH"));
		int fdate = cpymo_vars_get(&engine->vars, cpymo_parser_stream_span_pure("FDATE"));
		char str[16];
		sprintf(str, "%d/%d",
			cpymo_utils_clamp(fmonth, 1, 12),
			cpymo_utils_clamp(fdate, 1, 31));

		POP_ARG(date_bg); ENSURE(date_bg);
		POP_ARG(x_str); ENSURE(x_str);
		POP_ARG(y_str); ENSURE(y_str);
		POP_ARG(col_str);

		POS(x, y, x_str, y_str);

		cpymo_color col =
			cpymo_parser_stream_span_as_color(col_str);

		return cpymo_floating_hint_start(
			engine,
			cpymo_parser_stream_span_pure(str),
			date_bg,
			x, y, col, 1.5f);
	}
	
	else {
		char buf[32];
		cpymo_parser_stream_span_copy(buf, 32, command);

		fprintf(
			stderr,
			"[Warning] Unknown command \"%s\" in script %s(%u).\n",
			buf,
			interpreter->script_name,
			(unsigned)interpreter->script_parser.cur_line + 1);

		CONT_NEXTLINE;
	}
}