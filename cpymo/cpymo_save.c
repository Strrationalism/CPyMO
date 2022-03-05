#include "cpymo_save.h"
#include "cpymo_engine.h"
#include <cpymo_backend_save.h>
#include <endianness.h>
#include <assert.h>

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
			if (fwrite(STR, len, 1, save) != 1) { \
				fclose(save); \
				return CPYMO_ERR_UNKNOWN; \
			} \
		} \
		else { \
			uint16_t zero = end_htole16(0); \
			if (fwrite(&zero, sizeof(zero), 1, save) != 1) { \
				fclose(save); \
				return CPYMO_ERR_UNKNOWN; \
			} \
		}

	// TITLE
	WRITE_STR(e->title);

	// LATEST SAY
	WRITE_STR(e->say.current_name);
	WRITE_STR(e->say.current_text);

	// MSGBOX IMAGES
	WRITE_STR(e->say.namebox_name);
	WRITE_STR(e->say.msgbox_name);
	
	// BGM
	WRITE_STR(e->audio.bgm_name);

	// SE
	WRITE_STR(e->audio.se_name);

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
			int32_t x = (int32_t)e->anime.draw_x;
			int32_t y = (int32_t)e->anime.draw_y;

			uint32_t anime_params[] = {
				PACK32(all_frames),
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
			uint32_t checkpoint_line = (uint32_t)interpreter->checkpoint.cur_pos;
			
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
	return CPYMO_ERR_SUCC;
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

	if (fread(*str, len, 1, save) != 1) {
		free(*str);
		*str = NULL;
		return CPYMO_ERR_BAD_FILE_FORMAT;
	}

	*str[len] = '\0';

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
