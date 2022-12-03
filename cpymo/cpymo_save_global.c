#include "cpymo_prelude.h"
#include "cpymo_save_global.h"
#include "cpymo_engine.h"
#include <string.h>
#include <cpymo_backend_save.h>
#include <endianness.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

error_t cpymo_save_global_load(cpymo_engine *e)
{
	FILE *file = cpymo_backend_read_save(e->assetloader.gamedir, "global.csav");
	if (file == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	#define READ(PTR, UNITSIZE, COUNT) \
		if (fread(PTR, UNITSIZE, COUNT, file) != COUNT) { \
			if(buf) free(buf); \
			fclose(file); \
			return CPYMO_ERR_BAD_FILE_FORMAT; \
		}

	char *buf = NULL;
	size_t buf_size = 0;

	#define ENSURE_BUF(SIZE) \
		if (buf_size < SIZE) { \
			if (buf != NULL) { \
				char *new_buf = (char *)realloc(buf, SIZE); \
				if (new_buf) { \
					buf = new_buf; \
				} \
				else { \
					free(buf); \
					fclose(file); \
					return CPYMO_ERR_OUT_OF_MEM; \
				} \
			} \
			else buf = (char *)malloc(SIZE); \
			if(buf == NULL) { \
				fclose(file); \
				return CPYMO_ERR_OUT_OF_MEM; \
			} \
			buf_size = SIZE; \
		}

	// Global Variables
	while (true) {
		uint8_t flag = 0;
		READ(&flag, sizeof(flag), 1);

		if (flag == 0) break;

		if (flag != 1 && flag != 2) {
			fclose(file);
			return CPYMO_ERR_BAD_FILE_FORMAT;
		}

		uint16_t var_name_len;
		READ(&var_name_len, sizeof(var_name_len), 1);
		var_name_len = end_le16toh(var_name_len);

		ENSURE_BUF(var_name_len);
		READ(buf, sizeof(char), var_name_len);

		uint32_t val_abs;
		READ(&val_abs, sizeof(val_abs), 1);
		val_abs = end_le32toh(val_abs);

		cpymo_str var_name;
		var_name.begin = buf;
		var_name.len = var_name_len;

		error_t err = cpymo_vars_set(&e->vars, var_name,
			flag == 1 ? (int)val_abs : -(int)val_abs);

		if (err != CPYMO_ERR_SUCC) {
			if (buf) free(buf);
			fclose(file);
			return err;
		}
	}

	// Hash Flags
	uint64_t hash_flags_count;
	READ(&hash_flags_count, sizeof(hash_flags_count), 1);
	hash_flags_count = end_le64toh(hash_flags_count);

	for (size_t i = 0; i < hash_flags_count; ++i) {
		uint64_t flag;
		READ(&flag, sizeof(flag), 1);
		flag = end_le64toh(flag);
		cpymo_hash_flags_add(&e->flags, flag);
	}

	if (buf) free(buf);
	fclose(file);

	e->flags.dirty = false;
	e->vars.globals_dirty = false;

	return CPYMO_ERR_SUCC;

	#undef READ
	#undef ENSURE_BUF
}

error_t cpymo_save_global_save(cpymo_engine *e)
{
	if (e->vars.globals_dirty == false && e->flags.dirty == false)
		return CPYMO_ERR_SUCC;

	e->vars.globals_dirty = false;
	e->flags.dirty = false;

	FILE *file = cpymo_backend_write_save(e->assetloader.gamedir, "global.csav");
	if (file == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	#define WRITE(PTR, UNITSIZE, COUNT) \
		if (fwrite(PTR, UNITSIZE, COUNT, file) != COUNT) { \
			fclose(file); \
			return CPYMO_ERR_BAD_FILE_FORMAT; \
		}

	// Global Variables
	size_t global_vars = cpymo_vars_count(&e->vars.globals);
	for (size_t i = 0; i < global_vars; ++i) {
		cpymo_val val;
		const char *var_name = 
			cpymo_vars_get_by_index(e->vars.globals, i, &val);
		uint16_t var_name_len = (uint16_t)strlen(var_name);
		uint16_t var_name_len_le16 = end_htole16(var_name_len);

		uint8_t negative = val >= 0 ? 1 : 2;
		WRITE(&negative, sizeof(negative), 1);

		WRITE(&var_name_len_le16, sizeof(var_name_len_le16), 1);
		WRITE(var_name, sizeof(var_name[0]), var_name_len);

		uint32_t val_abs = end_htole32((uint32_t)abs(val));
		WRITE(&val_abs, sizeof(val_abs), 1);
	}

	uint8_t end_flag = 0;
	WRITE(&end_flag, sizeof(end_flag), 1);

	// Hash Flags
	size_t hash_flags_count = cpymo_hash_flags_count(&e->flags);
	uint64_t hash_flags_count_le64 = end_htole64((uint64_t)hash_flags_count);
	WRITE(&hash_flags_count_le64, sizeof(hash_flags_count_le64), 1);

	for (size_t i = 0; i < hash_flags_count; ++i) {
		uint64_t flag = end_htole64(cpymo_hash_flags_get(&e->flags, i));
		WRITE(&flag, sizeof(flag), 1);
	}

	fclose(file);

#ifdef __EMSCRIPTEN__
	EM_ASM(FS.syncfs(false, function(err) {}););
#endif

	return CPYMO_ERR_SUCC;

	#undef WRITE
}

error_t cpymo_save_config_save(const cpymo_engine *e)
{
	uint16_t config[] = {
		(uint16_t)(cpymo_audio_get_channel_volume(CPYMO_AUDIO_CHANNEL_BGM, &e->audio) * UINT16_MAX),
		(uint16_t)(cpymo_audio_get_channel_volume(CPYMO_AUDIO_CHANNEL_SE, &e->audio) * UINT16_MAX),
		(uint16_t)(cpymo_audio_get_channel_volume(CPYMO_AUDIO_CHANNEL_VO, &e->audio) * UINT16_MAX),
		(uint16_t)e->gameconfig.fontsize,
		(uint16_t)e->gameconfig.textspeed,
		(uint16_t)e->config_skip_already_read_only
	};

	for (size_t i = 0; i < sizeof(config) / sizeof(config[0]); ++i)
		config[i] = end_htole16(config[i]);
	
	FILE *config_file = cpymo_backend_write_save(e->assetloader.gamedir, "config.csav");
	if (config_file == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	const size_t written = fwrite(config, sizeof(config), 1, config_file);
	fclose(config_file);
	if (written != 1) return CPYMO_ERR_UNKNOWN;

#ifdef __EMSCRIPTEN__
	EM_ASM(FS.syncfs(false, function(err) {}););
#endif
		
	return CPYMO_ERR_SUCC;
}

error_t cpymo_save_config_load(cpymo_engine *e)
{
	uint16_t config[6];
	FILE *file = cpymo_backend_read_save(e->assetloader.gamedir, "config.csav");
	if (file == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	const size_t read = fread(config, sizeof(config), 1, file);
	fclose(file);
	if (read != 1) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	for (size_t i = 0; i < sizeof(config) / sizeof(config[0]); ++i)
		config[i] = end_le16toh(config[i]);

	cpymo_audio_set_channel_volume(CPYMO_AUDIO_CHANNEL_BGM, &e->audio, config[0] / (float)UINT16_MAX);
	cpymo_audio_set_channel_volume(CPYMO_AUDIO_CHANNEL_SE, &e->audio, config[1] / (float)UINT16_MAX);
	cpymo_audio_set_channel_volume(CPYMO_AUDIO_CHANNEL_VO, &e->audio, config[2] / (float)UINT16_MAX);
	e->gameconfig.fontsize = config[3];
	e->gameconfig.textspeed = config[4];
	e->config_skip_already_read_only = config[5] > 0;

	return CPYMO_ERR_SUCC;
}


