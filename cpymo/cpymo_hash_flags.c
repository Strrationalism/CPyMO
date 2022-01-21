#include "cpymo_hash_flags.h"
#include <stdlib.h>
#include <assert.h>

void cpymo_hash_flags_init(cpymo_hash_flags *f)
{
	f->flags = NULL;
	f->flag_count = 0;
	f->flag_buf_size = 0;
}

void cpymo_hash_flags_free(cpymo_hash_flags *f)
{
	if (f->flags) free(f->flags);
}

error_t cpymo_hash_flags_reserve(cpymo_hash_flags *f, size_t size)
{
	if (size == 0) return CPYMO_ERR_SUCC;
	else if (f->flags == NULL) {
		size *= 2;
		assert(f->flag_count == 0 && f->flag_buf_size == 0);
		f->flags = (cpymo_hash_flag *)malloc(sizeof(cpymo_hash_flag) * size);
		if (f->flags == NULL) return CPYMO_ERR_OUT_OF_MEM;

		f->flag_buf_size = size;
	}
	else if (f->flag_buf_size < size) {
		assert(f->flag_buf_size != 0);
		size *= 2;
		f->flags = (cpymo_hash_flag *)realloc(f->flags, size * sizeof(cpymo_hash_flag));
		if (f->flags == NULL) return CPYMO_ERR_OUT_OF_MEM;

		f->flag_buf_size = size;
	}

	return CPYMO_ERR_SUCC;
}

error_t cpymo_hash_flags_add(cpymo_hash_flags *flags, cpymo_hash_flag f)
{
	if (cpymo_hash_flags_check(flags, f) == true) return CPYMO_ERR_SUCC;

	error_t err = cpymo_hash_flags_reserve(flags, flags->flag_count + 1);
	CPYMO_THROW(err);

	flags->flags[flags->flag_count++] = f;
	return CPYMO_ERR_SUCC;
}

bool cpymo_hash_flags_check(cpymo_hash_flags *flags, cpymo_hash_flag f)
{
	for (size_t i = 0; i < flags->flag_count; ++i)
		if (flags->flags[i] == f) return true;

	return false;
}
