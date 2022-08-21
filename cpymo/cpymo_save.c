#include "cpymo_prelude.h"
#include "cpymo_save.h"
#include "cpymo_save_global.h"
#include "cpymo_engine.h"
#include "cpymo_msgbox_ui.h"
#include <cpymo_backend_save.h>
#include <endianness.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

static inline void cpymo_save_get_filename(char *dst, unsigned short save_id)
{
	sprintf(dst, "save-%02d.csav", save_id);
}

error_t cpymo_save_write(cpymo_engine * e, unsigned short save_id)
{
	FILE *save = NULL;
	const char * const empty = "";

	{
		char save_filename[16];
		cpymo_save_get_filename(save_filename, save_id);
		save = cpymo_backend_write_save(e->assetloader.gamedir, save_filename);

		if (save == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;
	}

	#define WRITE_STR(STR) \
		if (STR) \
		{ \
			const size_t len = strlen(STR); \
			const uint16_t len_le = end_htole16((uint16_t)len); \
			if (fwrite(&len_le, sizeof(len_le), 1, save) != 1) { \
				fclose(save); \
				return CPYMO_ERR_UNKNOWN; \
			} \
			if (len) { \
				if (fwrite(STR, len, 1, save) != 1) { \
					fclose(save); \
					return CPYMO_ERR_UNKNOWN; \
				} \
			} \
		} \
		else { \
			uint16_t zero = end_htole16(0); \
			if (fwrite(&zero, sizeof(zero), 1, save) != 1) { \
				fclose(save); \
				return CPYMO_ERR_UNKNOWN; \
			} \
		}

	WRITE_STR(e->title);

	// LATEST SAY
	WRITE_STR(e->say.current_name);
	WRITE_STR(e->say.current_text);

	// MSGBOX IMAGES
	WRITE_STR(e->say.namebox_name);
	WRITE_STR(e->say.msgbox_name);
	
	// BGM
	const char *bgm_name = cpymo_audio_get_bgm_name(e);
	WRITE_STR(bgm_name);

	// SE
	const char *se_name = cpymo_audio_get_se_name(e);
	WRITE_STR(se_name);

	// FADEOUT
	{
		uint8_t fadeout_state[] = {
			e->fade.state != cpymo_fade_disabled,
			e->fade.col.r,
			e->fade.col.g,
			e->fade.col.b
		};

		if (fwrite(fadeout_state, sizeof(fadeout_state), 1, save) != 1) {
			fclose(save);
			return CPYMO_ERR_UNKNOWN;
		}
	}

	#define PACK32(X) (assert(sizeof(X) == 4), end_htole32(*(uint32_t *)(&X)))

	// BG
	{
		WRITE_STR(e->bg.current_bg_name);
		int32_t bg_x = (int32_t)e->bg.current_bg_x;
		int32_t bg_y = (int32_t)e->bg.current_bg_y;

		if (e->scroll.img) {
			bg_x = (int32_t)e->scroll.ex;
			bg_y = (int32_t)e->scroll.ey;
		}

		uint32_t pos[] = { PACK32(bg_x), PACK32(bg_y) };
		if (fwrite(pos, sizeof(pos), 1, save) != 1) {
			fclose(save);
			return CPYMO_ERR_UNKNOWN;
		}
	}

	// CHARA
	{
		struct cpymo_chara *chara = e->charas.chara;
		while (chara) {
			if (chara->alive) {
				WRITE_STR(chara->chara_name);

				int32_t cid = (int32_t)chara->chara_id;
				int32_t layer = (int32_t)chara->layer;
				int32_t x = (int32_t)chara->pos_x.end_value;
				int32_t y = (int32_t)chara->pos_y.end_value;

				uint32_t chara_params[] = {
					PACK32(cid),
					PACK32(layer),
					PACK32(x),
					PACK32(y)
				};

				if (fwrite(chara_params, sizeof(chara_params), 1, save) != 1) {
					fclose(save);
					return CPYMO_ERR_UNKNOWN;
				}
			}
			chara = chara->next;
		}

		WRITE_STR(empty);
	}

	// ANIME
	{
		if (e->anime.anime_image && e->anime.is_loop) {
			WRITE_STR(e->anime.anime_name);

			int32_t all_frames = (int32_t)e->anime.all_frame;
			int32_t interval = (int32_t)(e->anime.interval * 1000.0f);
			int32_t x = (int32_t)e->anime.draw_x;
			int32_t y = (int32_t)e->anime.draw_y;

			uint32_t anime_params[] = {
				PACK32(all_frames),
				PACK32(interval),
				PACK32(x),
				PACK32(y)
			};

			if (fwrite(anime_params, sizeof(anime_params), 1, save) != 1) {
				fclose(save);
				return CPYMO_ERR_UNKNOWN;
			}
		}
		else {
			WRITE_STR(empty);
		}
	}

	// LOCAL VARS
	{
		struct cpymo_var *var = e->vars.locals;
		while (var) {
			WRITE_STR(var->name);
			int32_t val = (int32_t)var->val;
			uint32_t val_le = PACK32(val);
			if (fwrite(&val_le, sizeof(val_le), 1, save) != 1) {
				fclose(save);
				return CPYMO_ERR_UNKNOWN;
			}

			var = var->next;
		}

		
		WRITE_STR(empty);
	}

	// INTERPRETER
	{
		cpymo_interpreter *interpreter = e->interpreter;
		while (interpreter) {
			WRITE_STR(interpreter->script_name);
			uint32_t cur_pos = (uint32_t)interpreter->script_parser.cur_pos;
			uint32_t cur_line = (uint32_t)interpreter->script_parser.cur_line;
			uint32_t line_end = (uint32_t)interpreter->script_parser.is_line_end;
			uint32_t checkpoint_line = (uint32_t)interpreter->checkpoint.cur_line;
			
			uint32_t interpreter_params[] = {
				PACK32(cur_pos),
				PACK32(cur_line),
				PACK32(line_end),
				PACK32(checkpoint_line)
			};

			if (fwrite(interpreter_params, sizeof(interpreter_params), 1, save) != 1) {
				fclose(save);
				return CPYMO_ERR_UNKNOWN;
			}

			interpreter = interpreter->caller;
		}

		WRITE_STR(empty);
	}


	#undef PACK32
	#undef WRITE_STR

	fclose(save);

#ifdef __EMSCRIPTEN__
	EM_ASM(FS.syncfs(false, function(err) {}););
#endif

	return CPYMO_ERR_SUCC;
}

void cpymo_save_autosave(cpymo_engine *e)
{
	cpymo_save_write(e, 0);
	cpymo_save_global_save(e);
}

FILE * cpymo_save_open_read(struct cpymo_engine *e, unsigned short save_id)
{
	char filename[16];
	cpymo_save_get_filename(filename, save_id);

	return cpymo_backend_read_save(e->assetloader.gamedir, filename);
}

static error_t cpymo_save_read_string(char **str, FILE *save) 
{
	uint16_t len_le;
	if (fread(&len_le, sizeof(len_le), 1, save) != 1) {
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}

	size_t len = end_le16toh(len_le);

	char *dst = (char *)realloc(*str, len + 1);
	if (dst == NULL) return CPYMO_ERR_OUT_OF_MEM;

	*str = dst;

	if (len) {
		if (fread(*str, len, 1, save) != 1) {
			free(*str);
			*str = NULL;
			return CPYMO_ERR_BAD_FILE_FORMAT;
		}
	}

	(*str)[len] = '\0';

	return CPYMO_ERR_SUCC;
}

error_t cpymo_save_load_title(cpymo_save_title *out, FILE *save)
{
	assert(out->say_name == NULL);
	assert(out->say_text == NULL);
	assert(out->title == NULL);

	error_t err = cpymo_save_read_string(&out->title, save);
	CPYMO_THROW(err);

	err = cpymo_save_read_string(&out->say_name, save);
	if (err != CPYMO_ERR_SUCC) {
		free(out->title);
		out->title = NULL;
		return err;
	}

	err = cpymo_save_read_string(&out->say_text, save);
	if (err != CPYMO_ERR_SUCC) {
		free(out->title);
		free(out->say_name);
		out->title = NULL;
		out->say_name = NULL;
	}

	return CPYMO_ERR_SUCC;
}

error_t cpymo_save_load_savedata(cpymo_engine *e, FILE *save)
{
	// reset states
	cpymo_vars_clear_locals(&e->vars);

	cpymo_interpreter_free(e->interpreter);
	free(e->interpreter);
	e->interpreter = NULL;

	cpymo_wait_reset(&e->wait);
	cpymo_flash_reset(&e->flash);
	cpymo_fade_reset(&e->fade);
	cpymo_bg_reset(&e->bg);
	cpymo_anime_off(&e->anime);
	cpymo_select_img_reset(&e->select_img);
	cpymo_charas_free(&e->charas); cpymo_charas_init(&e->charas);
	cpymo_scroll_reset(&e->scroll);
	cpymo_say_free(&e->say); cpymo_say_init(&e->say);
	cpymo_text_clear(&e->text);
	
	cpymo_audio_vo_stop(e);
	cpymo_audio_bgm_stop(e);
	cpymo_audio_se_stop(e);

	cpymo_backlog_free(&e->backlog); cpymo_backlog_init(&e->backlog);


	// load save data
	char *strbuf = NULL;

	#define FAIL if (err != CPYMO_ERR_SUCC)
	#define THROW if (strbuf) free(strbuf); return err;

	// TITLE
	error_t err = cpymo_save_read_string(&strbuf, save);
	FAIL{ THROW; };

	if (e->title) free(e->title);
	e->title = strbuf;
	strbuf = NULL;

	// LATEST SAY
	err = cpymo_save_read_string(&strbuf, save);
	FAIL{ THROW; };
	err = cpymo_save_read_string(&strbuf, save);
	FAIL{ THROW; };

	// MSGBOX IMAGES
	{
		char *msgbox = NULL, *namebox = NULL;
		err = cpymo_save_read_string(&namebox, save);
		FAIL{ THROW; }

		err = cpymo_save_read_string(&msgbox, save);
		FAIL{ free(namebox); THROW; };

		cpymo_say_load_msgbox_and_namebox_image(
			&e->say,
			cpymo_parser_stream_span_pure(msgbox),
			cpymo_parser_stream_span_pure(namebox),
			&e->assetloader);

		free(namebox);
		free(msgbox);
	}

	// BGM
	err = cpymo_save_read_string(&strbuf, save);
	FAIL{ THROW; };
	if (*strbuf) cpymo_audio_bgm_play(e, cpymo_parser_stream_span_pure(strbuf), true);
	
	
	// SE
	err = cpymo_save_read_string(&strbuf, save);
	FAIL{ THROW; };
	if (*strbuf) cpymo_audio_se_play(e, cpymo_parser_stream_span_pure(strbuf), true);
	

	// FADEOUT
	{
		uint8_t fadeout_state[4];
		if (fread(fadeout_state, sizeof(fadeout_state), 1, save) != 1) { THROW; }

		if (fadeout_state[0]) {
			cpymo_color col;
			col.r = fadeout_state[1];
			col.g = fadeout_state[2];
			col.b = fadeout_state[3];

			e->fade.state = cpymo_fade_keep;
			e->fade.col = col;
			cpymo_tween_assign(&e->fade.alpha, 1.0f);
		}
		else {
			e->fade.state = cpymo_fade_disabled;
		}
	}

	#define READ_PARAMS(NAME, SIZE) \
		uint32_t NAME[SIZE]; \
		if (fread(NAME, sizeof(NAME), 1, save) != 1) { \
			THROW; \
		} \
		for (size_t iiiii = 0; iiiii < SIZE; ++iiiii) \
			NAME[iiiii] = end_le32toh(NAME[iiiii]);

	#define CAST(TYPE, X) (*(TYPE *)(&X))

	// BG
	{
		err = cpymo_save_read_string(&strbuf, save);
		FAIL{ THROW; };

		READ_PARAMS(bg_params, 2);

		cpymo_bg_command(
			e,
			&e->bg,
			cpymo_parser_stream_span_pure(strbuf),
			cpymo_parser_stream_span_pure("BG_NOFADE"),
			0, 0, 0);

		e->bg.current_bg_x = (float)CAST(int32_t, bg_params[0]);
		e->bg.current_bg_y = (float)CAST(int32_t, bg_params[1]);
	}

	// CHARA
	while (true) {
		err = cpymo_save_read_string(&strbuf, save);
		FAIL{ THROW; };

		if (*strbuf == '\0') break;

		READ_PARAMS(chara_params, 4);

		int cid = chara_params[0];
		int layer = chara_params[1];
		int32_t x = CAST(int32_t, chara_params[2]);
		int32_t y = CAST(int32_t, chara_params[3]);

		struct cpymo_chara *c = NULL;
		cpymo_charas_new_chara(
			e,
			&c,
			cpymo_parser_stream_span_pure(strbuf),
			cid,
			layer,
			0,
			(float)x,
			(float)y,
			1.0f,
			0);
	}

	// ANIME
	err = cpymo_save_read_string(&strbuf, save);
	FAIL{ THROW; };

	if (*strbuf) {
		READ_PARAMS(anime_params, 4);
		int32_t all_frames = CAST(int32_t, anime_params[0]);
		int32_t interval_x = CAST(int32_t, anime_params[1]);
		float interval = 0.001f * (float)interval_x;
		int32_t x = CAST(int32_t, anime_params[2]);
		int32_t y = CAST(int32_t, anime_params[3]);

		cpymo_anime_on(e, all_frames, cpymo_parser_stream_span_pure(strbuf), (float)x, (float)y, interval, true);
	}

	// LOCAL VARS
	while (true) {
		err = cpymo_save_read_string(&strbuf, save);
		FAIL{ THROW; };
		if (*strbuf == '\0') break;

		READ_PARAMS(val, 1);
		cpymo_vars_set(
			&e->vars,
			cpymo_parser_stream_span_pure(strbuf),
			(int)CAST(int32_t, val[0]));
	}

	// INTERPRETER
	cpymo_interpreter **slot = &e->interpreter;
	while (true) {
		err = cpymo_save_read_string(&strbuf, save);
		FAIL{ THROW; };

		if (*strbuf == '\0') break;

		*slot = (cpymo_interpreter *)malloc(sizeof(cpymo_interpreter));
		if (*slot == NULL) {
			err = CPYMO_ERR_OUT_OF_MEM;
			THROW;
		}

		err = cpymo_interpreter_init_script(*slot, strbuf, &e->assetloader);
		FAIL{ THROW; };

		READ_PARAMS(interpreter_params, 4);
		(*slot)->script_parser.cur_pos = interpreter_params[0];
		(*slot)->script_parser.cur_line = interpreter_params[1];
		(*slot)->script_parser.is_line_end = interpreter_params[2] != 0;
		(*slot)->checkpoint.cur_line = interpreter_params[3];

		slot = &(*slot)->caller;
	}

	if (strbuf) free(strbuf);
	strbuf = NULL;

	if (e->interpreter == NULL) return CPYMO_ERR_BAD_FILE_FORMAT;

	cpymo_interpreter_goto_line(e->interpreter, (uint64_t)e->interpreter->checkpoint.cur_line);

	cpymo_engine_request_redraw(e);

	return CPYMO_ERR_SUCC;
}

