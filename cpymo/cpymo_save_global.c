#include "cpymo_save_global.h"
#include "cpymo_engine.h"
#include <string.h>
#include <cpymo_backend_save.h>
#include <endianness.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

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
			if (buf != NULL) buf = (char *)realloc(buf, SIZE); \
			else buf = (char *)malloc(SIZE); \
			if(buf == NULL) { \
				fclose(file); \
				return CPYMO_ERR_OUT_OF_MEM; \
			} \
			buf_size = SIZE; \
		}

	// Global Variables
	while (true) {
		char flag = 0;
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
		val_abs = end_le16toh(val_abs);

		cpymo_parser_stream_span var_name;
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

	error_t err = cpymo_hash_flags_reserve(&e->flags, (size_t)hash_flags_count);
	if (err != CPYMO_ERR_SUCC) {
		if (buf) free(buf);
		fclose(file);
		return err;
	}

	e->flags.flag_count = (size_t)hash_flags_count;
	assert(e->flags.flag_count <= e->flags.flag_buf_size);

	for (size_t i = 0; i < e->flags.flag_count; ++i) {
		uint64_t flag;
		READ(&flag, sizeof(flag), 1);
		e->flags.flags[i] = end_le64toh(flag);
	}

	if (buf) free(buf);
	fclose(file);

	return CPYMO_ERR_SUCC;

	#undef READ
	#undef ENSURE_BUF
}

error_t cpymo_save_global_save(const cpymo_engine *e)
{
	FILE *file = cpymo_backend_write_save(e->assetloader.gamedir, "global.csav");
	if (file == NULL) return CPYMO_ERR_CAN_NOT_OPEN_FILE;

	#define WRITE(PTR, UNITSIZE, COUNT) \
		if (fwrite(PTR, UNITSIZE, COUNT, file) != COUNT) { \
			fclose(file); \
			return CPYMO_ERR_BAD_FILE_FORMAT; \
		}

	// Global Variables
	for (struct cpymo_var *var = e->vars.globals; var != NULL; var = var->next) {
		uint16_t var_name_len = (uint16_t)strlen(var->name);
		uint16_t var_name_len_le16 = end_htole16(var_name_len);

		char negative = var->val >= 0 ? 1 : 2;
		WRITE(&negative, sizeof(negative), 1);

		WRITE(&var_name_len_le16, sizeof(var_name_len_le16), 1);
		WRITE(var->name, sizeof(var->name[0]), var_name_len);

		uint32_t val_abs = end_htole32((uint32_t)abs(var->val));
		WRITE(&val_abs, sizeof(val_abs), 1);
	}

	char end_flag = 0;
	WRITE(&end_flag, sizeof(end_flag), 1);

	// Hash Flags
	uint64_t hash_flags_count_le64 = end_htole64((uint64_t)e->flags.flag_count);
	WRITE(&hash_flags_count_le64, sizeof(hash_flags_count_le64), 1);

	for (size_t i = 0; i < e->flags.flag_count; ++i) {
		uint64_t flag = end_htole64(e->flags.flags[i]);
		WRITE(&flag, sizeof(flag), 1);
	}

	fclose(file);

	return CPYMO_ERR_SUCC;

	#undef WRITE
}


